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
#include "CLI/CLI.hpp"

#include <libpldm/base.h>
#include <libpldm-oem/pldm.h>
#include "instance_id.hpp"

#define showMsg(fmt, args...)    if(verbose){fprintf(stderr, fmt, ##args);}
#define PLDMD_MSG_HDR_LEN     2  // 2 bytes (eid + mctp message type)
#define PLDM_COMMON_REQ_LEN   3  // 3 bytes common field for PLDM requests
#define PLDM_COMMON_RES_LEN   4  // 4 bytes for PLDM Responses
/**
 * PLDM Daemon Data Format :
 * index 0   : mctp destination eid
 * index 1   : mctp message type
 * index 2   : pldm package
 */
uint8_t MCTP_MSG_TYPE_PLDM = 1;
bool verbose = false;

struct pldmClients
{
  // bool active;
  uint8_t instance_id;
  int client_type;
  int fd;
};

enum
{
  MCTP_FD = 0,
  SERV_FD,
  SERV_FWUP_FD,
  FIRST_CLIENT_INDEX,
  DEFAULT_FD_NUM = FIRST_CLIENT_INDEX,
};

enum
{
  NORMAL_CLIENT = 0,
  FWUPDATE_CLIENT,
};

static int connect_to_socket (const char * path, int length)
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

class _ctx
{
public:
  bool clients_changed = false;
  uint8_t *buf;
  ssize_t buf_cur_size = 0;
  ssize_t buf_max_size = 0;
  int update_fd;
  std::vector<struct pldmClients> clients;

  /* init for mctpd socket fd */
  _ctx (int sockfd, int bus) : mctp_fd (sockfd)
  {
      clients.clear();
      std::string socketName;

      socketName = "pldm-mux" + std::to_string(bus);
      init_socket(serv_fd, socketName.c_str());
      showMsg("Create socket for clients(%s) successfully.\n", socketName.c_str());

      socketName = "pldm-fwup-mux" + std::to_string(bus);
      init_socket(serv_fwup_fd, socketName.c_str());
      is_fw_update_active = false;
      showMsg("Create socket for firmware update client(%s) successfully.\n", socketName.c_str());

      init_pollfds();
      showMsg("Create monitor for all socket successfully.\n");
  }

  struct pollfd* get_pollfd (int index) {
      return pollfds + index;
  }

  int get_client_fd (uint8_t instance_id)
  {
      int index = 0;
      for (auto&client:clients) {
        if (client.instance_id == instance_id) {
          showMsg("Send recieve data to client[%d]\n", index);
          showMsg("client[%d] fd = %d\n", index, client.fd);
          return client.fd;
        }
        ++index;
      }
      return -1;
  }

  bool add_client (int fd, int client_type)
  {
      int instance_id = ids.next();
      if (instance_id < 0) {
        std::cout << "Run out of instance id." << std::endl;
        return false;
      }

      if (client_type == FWUPDATE_CLIENT) {
        if (is_fw_update_active) {
          std::cout << "There is another fw-update." << std::endl;
          return false;
        } else {
          is_fw_update_active = true;
          update_fd = fd;
        }
      }

      // store client data
      struct pldmClients newClient = {
        (uint8_t)instance_id, // instance id
        client_type,          // client type
        fd,                   // file descriptor
      };
      clients.push_back(newClient);
      return true;
  }

  void update_pollfds ()
  {
      pollfds = (struct pollfd*)realloc(pollfds,
                (clients.size() + DEFAULT_FD_NUM) * sizeof(struct pollfd));

      int index = FIRST_CLIENT_INDEX;
      for (auto & client : clients) {
        pollfds[index].fd = client.fd;
        pollfds[index].events = POLLIN;
        ++index;
      }
  }

  void remove_client (int index)
  {
      if (clients[index].client_type == FWUPDATE_CLIENT) {
        update_fd = 0;
        is_fw_update_active = false;
      }
      ids.markFree(clients[index].instance_id);
      close(clients[index].fd);
      clients.erase(clients.begin()+index);
  }

  void showClient () {
    int index = 0;
    for (auto&c:clients) {
      showMsg("client[%d]:\n", index++);
      showMsg("instance id : %02X\n", c.instance_id);
      showMsg("client type : %d\n", c.client_type);
      showMsg("fd          : %d\n", c.fd);
    }
  }

private:

  /* init socket for clients */
  void init_socket (int& fd, std::string path)
  {
      int returnCode = 0;
      if ((fd = socket(AF_UNIX, SOCK_SEQPACKET, 0)) == 0)
      {
        returnCode = -errno;
        showMsg("Failed to create the socket(%s), RC = %d\n", path.c_str(), returnCode);
        exit(EXIT_FAILURE);
      }

      struct sockaddr_un addr;
      addr.sun_family = AF_UNIX;
      memcpy(addr.sun_path, "\0", 1);
      memcpy(addr.sun_path+1, path.c_str(), path.length());
      if (bind(fd, (struct sockaddr*)&addr, sizeof(addr.sun_family)+path.length()+1))
      {
        returnCode = -errno;
        showMsg("Failed to bind the socket(%s), RC = %d\n", path.c_str(), returnCode);
        exit(EXIT_FAILURE);
      }

      if (listen(fd, 1))
      {
        returnCode = -errno;
        showMsg("Failed to listen the socket(%s), RC = %d\n", path.c_str(), returnCode);
        exit(EXIT_FAILURE);
      }
  }

  void init_pollfds ()
  {
      // init poll fds to monitor all socket
      pollfds = (struct pollfd*) malloc (DEFAULT_FD_NUM * sizeof(struct pollfd));
      pollfds[MCTP_FD].fd = mctp_fd;
      pollfds[SERV_FD].fd = serv_fd;
      pollfds[SERV_FWUP_FD].fd = serv_fwup_fd;

      pollfds[MCTP_FD].events = POLLIN;
      pollfds[SERV_FD].events = POLLIN;
      pollfds[SERV_FWUP_FD].events = POLLIN;
  }

  int mctp_fd, serv_fd, serv_fwup_fd;
  bool is_fw_update_active;
  struct pollfd *pollfds;
  pldm::InstanceId ids;
};

static void tx_message (int fd, uint8_t* buf, ssize_t length)
{
  if (verbose)
    for (int i = 0; i < (int)length; ++i)
      showMsg("tx : data[%02d] : %02X\n", i, *(buf+i));
  auto msg = reinterpret_cast<pldm_msg*>(buf+PLDMD_MSG_HDR_LEN);
  bool req = (msg->hdr.request == PLDM_REQUEST ||
              msg->hdr.request == PLDM_ASYNC_REQUEST_NOTIFY);
  showMsg("Tx IID (%s): %02X\n", ((req)?"Request ":"Response"), msg->hdr.instance_id);
  send (fd, buf, length, 0);
}

static int rx_message (int fd, _ctx &ctx)
{
  int returnCode = 0;
  ssize_t peekedLength = recv(fd, nullptr, 0, MSG_PEEK | MSG_TRUNC);

  if (peekedLength <= 0) {
    std::cerr << "recv system call failed, RC= " << returnCode << "\n";
    returnCode = -1;
    return returnCode;
  }

  // buf[0] = MCTP destination EID
  // buf[1] = MCTP message type (PLDM:0x01)
  // buf[2] = PLDM package (Header 3 bytes)
  if (peekedLength < PLDM_HEADER_SIZE + PLDMD_MSG_HDR_LEN) {
    std::cerr << "recv pldm package length too short.\n";
    returnCode = -1;
    return returnCode;
  }

  if (peekedLength > ctx.buf_max_size) {
    auto tmp = (uint8_t *)realloc(ctx.buf, peekedLength);
    if (!tmp) {
      std::cerr << "can't allocate for incoming message\n";
      returnCode = -1;
      return returnCode;
    } else {
      ctx.buf = tmp;
      ctx.buf_max_size = peekedLength;
    }
  }
  ctx.buf_cur_size = peekedLength;

  returnCode = recv(fd, ctx.buf, ctx.buf_cur_size, 0);
  if (returnCode < 0) {
    if (errno != ECONNRESET)
      std::cerr << "can't receive from client\n";
    returnCode = -1;
    return returnCode;
  }

  if (verbose) {
    showMsg("pakage length = %d\n", (int)ctx.buf_cur_size);
    auto msg = reinterpret_cast<pldm_msg*>(ctx.buf+PLDMD_MSG_HDR_LEN);
    bool req = (msg->hdr.request == PLDM_REQUEST ||
                msg->hdr.request == PLDM_ASYNC_REQUEST_NOTIFY);
    showMsg("Rx IID (%s): %02X\n", ((req)?"Request ":"Response"), msg->hdr.instance_id);
    for (int i = 0; i < (int)ctx.buf_cur_size; ++i) {
      showMsg("rx : data[%02d] : %02X\n", i, *(ctx.buf+i));
    }
  }

  return returnCode;
}

int recv_from_client (_ctx &ctx, int index)
{
  auto mctp = ctx.get_pollfd(MCTP_FD);
  int fd = ctx.clients[index].fd;

  if (rx_message(fd, ctx) < 0) {
    showMsg("client[%2d] disconnect.\n", index);
    ctx.remove_client(index);
    ctx.clients_changed = true;
    return 1;

  } else {
    // buf[0] = MCTP destination EID
    // buf[1] = MCTP message type (PLDM:0x01)
    // buf[2] = PLDM package
    auto msg = reinterpret_cast<pldm_msg*>(ctx.buf+PLDMD_MSG_HDR_LEN);

    // If message is request, then assign instance id.
    if (msg->hdr.request == PLDM_REQUEST ||
        msg->hdr.request == PLDM_ASYNC_REQUEST_NOTIFY) {
      msg->hdr.instance_id = ctx.clients[index].instance_id;
    }
    tx_message(mctp->fd, ctx.buf, ctx.buf_cur_size);
    return 0;
  }
}

/* Return Instance id */
int process_data_from_mctpd (int mctp_fd, _ctx &ctx)
{
  if (rx_message(mctp_fd, ctx) < 0) {
    /* MCTP daemon close socket, then PLDM daemon is useless */
    exit(EXIT_FAILURE);
  } else {
    // buf[0] = MCTP destination EID
    // buf[1] = MCTP message type (PLDM:0x01)
    // buf[2] = PLDM package
    auto request = reinterpret_cast<pldm_msg*>(ctx.buf+PLDMD_MSG_HDR_LEN);
    return request->hdr.instance_id;
  }
}

void polling (_ctx &ctx)
{
  // index : 2 for new client.
  // -1 means no timeout, function will keep polling.
  int returnCode = poll(ctx.get_pollfd(0), FIRST_CLIENT_INDEX + ctx.clients.size(), -1);
  if (returnCode < 0) {
    std::cerr << "Failed to poll fds, RC= " << returnCode << "\n";
    exit(EXIT_FAILURE);
  }
}

void check_mctp_socket (_ctx &ctx)
{
  auto mctp = ctx.get_pollfd(MCTP_FD);

  if (mctp->revents) {
    showMsg("Recieve data from mctpd.\n");
    uint8_t iid = process_data_from_mctpd(mctp->fd, ctx);
    auto msg = reinterpret_cast<pldm_msg*>(ctx.buf+PLDMD_MSG_HDR_LEN);

    // For firmware update socket
    if (msg->hdr.type == PLDM_FWUP && (msg->hdr.request == PLDM_REQUEST ||
      msg->hdr.request == PLDM_ASYNC_REQUEST_NOTIFY)) {
      showMsg("Forward to fwupdate client.\n");
      tx_message(ctx.update_fd, ctx.buf, (ssize_t)ctx.buf_cur_size);

    // For request handle
    } else if (msg->hdr.request == PLDM_REQUEST ||
      msg->hdr.request == PLDM_ASYNC_REQUEST_NOTIFY) {
      uint8_t *resp = nullptr;
      ssize_t resp_bytes = 0;
      pldm_msg_handle(ctx.buf + PLDMD_MSG_HDR_LEN,
                      ctx.buf_cur_size - PLDMD_MSG_HDR_LEN,
                      &resp, &resp_bytes);

      auto mctp = ctx.get_pollfd(MCTP_FD);
      uint8_t eid = *(ctx.buf);
      if (resp_bytes+PLDMD_MSG_HDR_LEN > ctx.buf_max_size) {
        auto tmp = (uint8_t *)realloc(ctx.buf, resp_bytes+PLDMD_MSG_HDR_LEN);
        if (!tmp) {
          std::cerr << "can't allocate for response message\n";
          return;
        } else if (resp_bytes > 0){
          ctx.buf_max_size = resp_bytes+PLDMD_MSG_HDR_LEN;
          ctx.buf_cur_size = resp_bytes+PLDMD_MSG_HDR_LEN;
          ctx.buf = tmp;
          ctx.buf[0] = eid;
          ctx.buf[1] = MCTP_PLDM_TYPE;
          memcpy(ctx.buf+PLDMD_MSG_HDR_LEN, resp, resp_bytes);
        }
      }
      if (resp)
        free(resp);
      tx_message(mctp->fd, ctx.buf, (ssize_t)ctx.buf_cur_size);

    // For response handle
    } else {
      int fd = ctx.get_client_fd(iid);
      if (fd < 0) {
        std::cerr << "can't find client with instance id = " << (int)iid << "\n";
      } else {
        // Send back to client, so remove buf[0] & buf[1]
        tx_message(fd, ctx.buf, (ssize_t)ctx.buf_cur_size);
      }
    }
  }
}

void check_clients (_ctx &ctx)
{
  int client_num = ctx.clients.size();

  for (int i = 0; i < client_num; ++i) {
    auto pollfd = ctx.get_pollfd(i + FIRST_CLIENT_INDEX);
    if (!pollfd->revents)
      continue;

    showMsg("Recieve data from client[%d].\n", i);

    if (recv_from_client(ctx, i)) {
      // if client disconnect, update index
      client_num = ctx.clients.size();
      --i;
    }
  }
}

void check_pldm_socket (_ctx &ctx)
{
  auto pldm = ctx.get_pollfd(SERV_FD);

  if (pldm->revents) {

    // accept new connection and record fd.
    int fd = accept4(pldm->fd, NULL, 0, SOCK_NONBLOCK);
    if (fd < 0) 
      return;

    if (!ctx.add_client(fd, NORMAL_CLIENT)) {
      close(fd);
      return;
    }

    ctx.clients_changed = true;
    showMsg ("Recieve new connection.\n");
  }
}

void check_pldm_fwupdate_socket(_ctx &ctx)
{
  auto fwupdate = ctx.get_pollfd(SERV_FWUP_FD);

  if (fwupdate->revents) {

    // accept new connection and record fd.
    int fd = accept4(fwupdate->fd, NULL, 0, SOCK_NONBLOCK);
    if (fd < 0) 
      return;

    if (!ctx.add_client(fd, FWUPDATE_CLIENT)) {
      close(fd);
      return;
    }

    ctx.clients_changed = true;
    showMsg ("Recieve new connection for fw-update.\n");
  }
}

int connect_mctp_socket (int bus)
{
  std::string path = "mctp-mux" + std::to_string(bus);
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

static void run_daemon (_ctx &ctx)
{
  showMsg("Starting loop...\n");

  for (;;) {

    if (ctx.clients_changed) {
      ctx.update_pollfds();
      ctx.clients_changed = false;
    }

    // listen all the fds with no timeout
    ctx.showClient();
    polling(ctx);

    // check mctpd
    check_mctp_socket(ctx);

    // check clients
    check_clients(ctx);

    // check pldmd socket for new client
    check_pldm_socket(ctx);

    // check pldmd fwupdate socket for new client
    check_pldm_fwupdate_socket(ctx);
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
        cmd_table["--verbose"] = "";
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
  int busNumber = -1;
  optionDescription = "Setting bus number. (required)";
  auto busOption = app.add_option("-b, --bus", busNumber, optionDescription);
  busOption->required();
  busOption->check(CLI::Range(0, 32));

  // init <verbose> flag
  optionDescription = "Setting verbose. (default:disable)";
  app.add_flag("-v, --verbose", verbose, optionDescription);

  // parse and execute
  CLI11_PARSE(app, argc, argv);
  showMsg("Bus number = %d\n", busNumber);
  showMsg("Verbose = %s\n", ((verbose) ? "Enable" : "Disable"));

  // Connect to mctpd
  int sockfd = connect_mctp_socket(busNumber);
  if (sockfd < 0)
    exit(EXIT_FAILURE);

  showMsg("Connected to mctpd_%02d.\n", busNumber);

  // Init sock for pldm clients & run daemon
  _ctx* ctx = new _ctx(sockfd, busNumber);
  run_daemon(*ctx);

  if(ctx)
    free(ctx);
  exit(EXIT_SUCCESS);
}
