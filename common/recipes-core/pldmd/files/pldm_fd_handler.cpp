#include "pldm_fd_handler.hpp"

#include <algorithm>
#include <stdio.h>
#include <unistd.h>
#include <libpldm/base.h>
#include <libpldm-oem/pldm.h>
#include <glog/logging.h>

int
pldm_fd_handler::polling()
{
  size_t fd_count = get_count(tx_server_fds, server_fds, clients_data);
  return do_polling(fd_count);
};

void
pldm_fd_handler::update_pollfds()
{
  if (!client_change)
    return;

  std::vector<pldm_client> tmp_clients_data = {};
  for (auto&client:clients_data) {
    if (client.active == true)
      tmp_clients_data.push_back(client);
    else {
      close(client.fd);
      ids.markFree(client.instance_id);
      if (client.client_type == UPDATE_CLIENT) {
        fw_update_client = 0;
        is_fw_update_active = false;
      }
    }
  }
  clients_data = tmp_clients_data;

  size_t index;
  size_t fd_count = get_count(tx_server_fds, server_fds, clients_data);
  LOG(INFO) << "update_pollfds : fd_count = " << (int)fd_count;

  void* result = realloc(pollfds, fd_count * sizeof(struct pollfd));
  if( result == NULL )
    return;
  else
    pollfds = (struct pollfd*)result;


  index = get_count(tx_server_fds, server_fds);
  for (auto& data : clients_data) {
    pollfds[index].fd = data.fd;
    pollfds[index].events = POLLIN;
    ++index;
  }
  client_change = false;
}

bool
pldm_fd_handler::add_client(int fd, uint8_t type)
{
  if (type == UPDATE_CLIENT && is_fw_update_active) {
    LOG(INFO) << "fw update on going...";
    return false;
  }

  int iid = ids.next();
  if (iid < 0) {
    LOG(INFO) << "add client fail. run out of instance id.";
    return false;
  }

  pldm_client data = {
    fd,
    (uint8_t)iid,
    type,
    true,
  };

  add_element(clients_data, data);
  client_change = true;
  if (type == UPDATE_CLIENT) {
    fw_update_client = fd;
    is_fw_update_active = true;
  }

  std::string msg;
  if(type == NORMAL_CLIENT) msg = "NORMAL_CLIENT";
  if(type == UPDATE_CLIENT) msg = "UPDATE_CLIENT";
  LOG(INFO) << "Add client " << msg.c_str() << " successflly.";

  return true;
}

void
pldm_fd_handler::pop_client(int index)
{
  // pop_element(clients_data, index);
  // if (clients_data[index].type == FWUPDATE_CLIENT)
  //   is_fw_update_active = false;
  clients_data[index].active = false;
  client_change = true;
}

void 
pldm_fd_handler::do_pop_client(int index)
{
  pop_element(clients_data, index);
  // client_change = true;
}

int
pldm_fd_handler::get_client_fd(uint8_t iid)
{
  auto it = find_if(clients_data.begin(), clients_data.end(),
    pldm_client_finder(iid));

  if (it != clients_data.end())
    return it->fd;
  else
    return -1;
}

int
pldm_fd_handler::get_client_index(uint8_t iid) const
{
  auto i = std::find_if(clients_data.cbegin(), clients_data.cend(),
                        [=](auto c) { return c.instance_id == iid; });

  if (i == clients_data.cend()) return -1;
  return std::distance(clients_data.cbegin(), i);
}

void
pldm_fd_handler::show_clients()
{
  int index = 0;

  for (auto&c:clients_data) {
    LOG(INFO) << "client[" << index++ << "]:";
    LOG(INFO) << "instance id : " << std::hex << c.instance_id;
    LOG(INFO) << "client type : " << std::hex << c.client_type;
    LOG(INFO) << "fd          : " << c.fd;
  }
}

int
pldm_fd_handler::client_send_data(int index, uint8_t *buf, size_t size)
{
  auto msg = reinterpret_cast<pldm_msg*>(buf+PLDMD_MSG_HDR_LEN);

  // If message is request, then assign instance id.
  if (msg->hdr.request == PLDM_REQUEST ||
      msg->hdr.request == PLDM_ASYNC_REQUEST_NOTIFY) {
    msg->hdr.instance_id = clients_data[index].instance_id;
  }

  LOG(INFO) << "Forward to mctp daemon.";
  return fd_handler::send_data(mctpd_fd, buf, size);
}

int 
pldm_fd_handler::send_fw_client_data(uint8_t * buf, size_t size)
{
  return fd_handler::send_data(fw_update_client, buf, size);
}

int 
pldm_fd_handler::recv_data(int fd, uint8_t** buf, size_t &size)
{
  int returnCode = fd_handler::recv_data(fd, buf, size);

  if (returnCode == 0) {
    if (size < PLDM_HEADER_SIZE + PLDMD_MSG_HDR_LEN) {
      LOG(INFO) << "recv pldm package length too short.";
      returnCode = ERR_SIZE_TOO_SHORT;
      return returnCode;
    }
  }

  return returnCode;
}

int
pldm_fd_handler::check_mctpd_socket()
{
  int fd = check_pollfds(MCTP_FD);
  if (fd)
    LOG(INFO) << "Receive data from mctpd.";
  return fd;
}

int
pldm_fd_handler::check_pldmd_socket()
{
  int fd = check_pollfds(SERV_FD);
  if (fd)
    LOG(INFO) << "Receive new client's connection.";
  return fd;
}

int
pldm_fd_handler::check_pldmd_fwupdate_socket()
{
  int fd = check_pollfds(SERV_FWUP_FD);
  if (fd)
    LOG(INFO) << "Receive new fw-update client's connection.";
  return fd;
}

int
pldm_fd_handler::check_clients(int index)
{
  int pollfds_index = (int)get_count(tx_server_fds, server_fds) + index;
  int fd = check_pollfds(pollfds_index);
  LOG(INFO) << "pollfds_index : " << pollfds_index;

  if (fd)
    LOG(INFO) << "Receive data from client = " << index;

  return fd;
}

void
pldm_fd_handler::do_init()
{
  // init tx server fd
  init_tx_server_fd(mctpd_fd);

  // init server fd
  std::string socketName;
  socketName = "pldm-mux" + bus;
  init_server_fd(pldmd_fd, socketName);
  LOG(INFO) << "Create socket for clients = "
            << socketName.c_str()
            << " successfully.";

  socketName = "pldm-fwup-mux" + bus;
  init_server_fd(pldmd_fwupdate_fd, socketName);
  LOG(INFO) << "Create socket for firmware update client = "
            << socketName.c_str()
            << " successfully.";
  is_fw_update_active = false;

  init_pollfds();
  LOG(INFO) << "Create monitor for all socket successfully.";
}
