/*
 * Copyright 2022-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <unistd.h>
#include <getopt.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include "CLI/CLI.hpp"
#include <glog/logging.h>
#include <libpldm/base.h>
#include <libpldm-oem/pldm.h>
#include "instance_id.hpp"
#include "pldm_fd_handler.hpp"

// #define PLDMD_MSG_HDR_LEN     2  // 2 bytes (eid + mctp message type)
// #define PLDM_COMMON_REQ_LEN   3  // 3 bytes common field for PLDM requests
// #define PLDM_COMMON_RES_LEN   4  // 4 bytes for PLDM Responses
/**
 * PLDM Daemon Data Format :
 * index 0   : mctp destination eid
 * index 1   : mctp message type
 * index 2   : pldm package
 */
const uint8_t MCTP_MSG_TYPE_PLDM = 1;
bool verbose = false;
pldm_fd_handler *handler;

static int
connect_to_socket (const char * path, int length)
{
  int returnCode = 0;
  int sockfd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if (-1 == sockfd)
  {
    returnCode = -errno;
    std::cerr << "Failed to create the socket, RC= " << returnCode << "\n";
    close(sockfd);
    return -1;
  }

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  memcpy(addr.sun_path, "\0", 1);
  memcpy(addr.sun_path+1, path, length++);
  if (connect(sockfd, (struct sockaddr *)&addr, length + sizeof(addr.sun_family)) < 0)
  {
    returnCode = -errno;
    std::cerr << "Failed to connect the socket, RC= " << returnCode << "\n";
    close(sockfd);
    return -1;
  }

  return sockfd;
}

static int 
connect_mctp_socket (const std::string &bus)
{
  std::string path = "mctp-mux" + bus;
  int sockfd = connect_to_socket(path.c_str(), path.length());
  if (sockfd < 0)
    return -1;

  // Send a byte of MSG_TYPE (0x01) for registration
  if (send(sockfd , &MCTP_MSG_TYPE_PLDM , 1 , 0) < 0)
  {
    int returnCode = -errno;
    std::cerr << "Failed to send message type as pldm to mctp, RC= "
              << returnCode << "\n";
    return -1;
  }

  return sockfd;
}

static void
freeBuf (uint8_t* buf)
{
  if (buf != nullptr) {
    free(buf);
  }
}

static void 
mctpd_msg_handle (int fd, uint8_t *buf, size_t size)
{
  int rc = handler->recv_data(fd, &buf, size);
  LOG(INFO) << "mctpd_msg_handle rc = " << rc;

  // MCTP daemon close socket, then PLDM daemon is useless
  if (rc == ERR_END_OF_FILE) {
    LOG(ERROR) << "Disconnected with MCTP daemon.";
    LOG(ERROR) << "Close proccess...";
    exit(EXIT_FAILURE);

  // Handle message
  } else {
    auto msg = reinterpret_cast<pldm_msg*>(buf+PLDMD_MSG_HDR_LEN);

    // For firmware update socket
    if (msg->hdr.type == PLDM_FWUP && (msg->hdr.request == PLDM_REQUEST ||
      msg->hdr.request == PLDM_ASYNC_REQUEST_NOTIFY)) {
      LOG(INFO) << "Forward to fwupdate client.";
      handler->send_fw_client_data(buf, size);

    // For request handle
    } else if (msg->hdr.request == PLDM_REQUEST ||
      msg->hdr.request == PLDM_ASYNC_REQUEST_NOTIFY) {
      LOG(INFO) << "Request handle.";

      uint8_t *resp;
      int resp_len;

      pldm_msg_handle(buf + PLDMD_MSG_HDR_LEN, size - PLDMD_MSG_HDR_LEN, &resp, &resp_len);
      std::cout << std::hex;
      for (int i = 0; i < resp_len ; i++) {
        if(i < PLDM_COMMON_RES_LEN) {
          std::cout << "PLDM data[" << i << "] = 0x" << unsigned(resp[i]) << "\n";
        }
        else {
          std::cout << "IPMI data[" << i << "] = 0x" << unsigned(resp[i]) << "\n";
        }
      }
      std::cout << std::dec;
      handler->client_send_data(0, resp, resp_len);
    // For response handle
    } else if (msg->hdr.request == PLDM_RESPONSE) {
      uint8_t iid = msg->hdr.instance_id;
      int cfd = handler->get_client_fd(iid);
      if (cfd < 0) {
        LOG(ERROR) << "can't find client with instance id = " << (int)iid;
      } else {
        // Send back to client, so remove buf[0] & buf[1]
        LOG(INFO) << "Forward to client iid:" << (int)iid;
        handler->send_data(cfd, buf, size);
      }
    }
  }
  freeBuf(buf);
}

static void
client_msg_handle (int fd, int index, uint8_t *buf, size_t size)
{
  int rc = handler->recv_data(fd, &buf, size);

  // client disconnected.
  if (rc == ERR_END_OF_FILE) {
    LOG(INFO) << "client = "
              << index
              << " disconnect.";
    handler->pop_client(index);

  // client message handle
  } else {
    handler->client_send_data(index, buf, size);
  }

  freeBuf(buf);
}

static void
new_client_handle(int fd, uint8_t type)
{
  // accept new connection and record fd.
  int new_fd = accept4(fd, NULL, 0, SOCK_NONBLOCK);
  if (new_fd < 0)
    return;

  if (!handler->add_client(new_fd, type)) {
    close(new_fd);
    return;
  }
}

static int
run_daemon ()
{
  LOG(INFO) << "Starting loop...";
  int fd;
  uint8_t* buf = nullptr;
  size_t size = 0;

  for (;;) {

    handler->update_pollfds();
    handler->show_clients();
    handler->polling();

    // check if there are messages from mctpd
    fd = handler->check_mctpd_socket();
    if (fd) {
      mctpd_msg_handle(fd, buf, size);
    }

    // check if there are messages from connected clients
    int client_count = handler->get_client_count();
    LOG(INFO) << "client_count : " << client_count;
    for (int i = 0; i < client_count; ++i) {
      fd = handler->check_clients(i);
      LOG(INFO) << "index : " << i << " fd : " << fd;

      if (fd) {
        client_msg_handle(fd, i, buf, size);
      }
    }

    fd = handler->check_pldmd_socket();
    if (fd) {
      new_client_handle(fd, NORMAL_CLIENT);
    }

    fd = handler->check_pldmd_fwupdate_socket();
    if (fd) {
      new_client_handle(fd, UPDATE_CLIENT);
    }
  }
}

class myFormatter : public CLI::Formatter
{
  protected:
    std::string get_option_name (std::string str) const
    {
        std::map<std::string, std::string> cmd_table;
        cmd_table["--help"] = "";
        cmd_table["--bus"] = " <bus number>";
        cmd_table["--log"] = " <log level>";
        return cmd_table[str];
    }

  public:
    myFormatter() : Formatter() {}
    std::string make_option_opts (const CLI::Option * opt) const override
    {
        return get_option_name(opt->get_name());
    }
};

int main (int argc, char** argv)
{
  // init CLI app
  CLI::App app("PLDM daemon");

  // init "--help" message
  auto fmt = std::make_shared<myFormatter>();
  fmt->column_width(30);
  app.failure_message(CLI::FailureMessage::simple);
  app.formatter(fmt);

  // init options
  std::string optionDescription;

  // init <bus number> option
  std::string busNumber = "";
  optionDescription = "Setting bus number. (required)";
  auto option = app.add_option("-b, --bus", busNumber, optionDescription);
  option->required();

  // log level
  int level = 1;
  optionDescription = "Setting log level. (default = WARNING(1))";
  option = app.add_option("-l, --log", level, optionDescription);
  option->check(CLI::Range(0, 3));

  // parse and execute
  CLI11_PARSE(app, argc, argv);
  
  // init log
  std::string log_path = "/tmp/glog";
  if (mkdir(log_path.c_str(), 0777) == -1) {
    if (errno == EEXIST) {
      std::cerr << "Directory already exist." << std::endl;
      errno = 0;
    }
  }
  FLAGS_log_dir = log_path.c_str();
  FLAGS_minloglevel = level;
  ::google::InitGoogleLogging(argv[0]);

  LOG(INFO) << "Bus number = " << busNumber;

  // Connect to mctpd
  int mctpfd = connect_mctp_socket(busNumber);
  if (mctpfd < 0)
    exit(EXIT_FAILURE);

  LOG(INFO) << "Connected to mctpd_" << busNumber;

  // Init sock for pldm clients & run daemon
  handler = new pldm_fd_handler(mctpfd, busNumber);
  if(run_daemon() < 0)
    exit(EXIT_FAILURE);

  if(handler)
    free(handler);
  exit(EXIT_SUCCESS);
}
