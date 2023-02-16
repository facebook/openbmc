#pragma once

#include "fd_handler.hpp"
#include "instance_id.hpp"

#define PLDMD_MSG_HDR_LEN   2  // 2 bytes (eid + mctp message type)
#define PLDM_COMMON_REQ_LEN 3  // 3 bytes common field for PLDM requests
#define PLDM_COMMON_RES_LEN 4  // 4 bytes for PLDM Responses

typedef struct pldm_client
{
  int fd;
  uint8_t instance_id;
  uint8_t client_type;
  bool active;
} pldm_client;

typedef struct pldm_client_finder
{
  uint8_t iid;
  explicit pldm_client_finder(uint8_t iid):iid(iid){}
  bool operator() (const pldm_client& target) const { return target.instance_id==iid; }
} pldm_client_finder;

enum {
  PLDM_MSG_SUCCESS = 0,
  ERR_END_OF_FILE  = -1,
  ERR_SIZE_TOO_SHORT = -2,
};

enum
{
  MCTP_FD = 0,
  SERV_FD,
  SERV_FWUP_FD,
  FIRST_CLIENT_INDEX,
  DEFAULT_FD_NUM = FIRST_CLIENT_INDEX,
};

enum {
  NORMAL_CLIENT = 0,
  UPDATE_CLIENT,
};

class pldm_fd_handler : public fd_handler
{
  public:
    std::string bus=0;
    int mctpd_fd=0;
    int pldmd_fd=0;
    int pldmd_fwupdate_fd = 0;

    pldm_fd_handler(int mctpd_fd, const std::string &bus) :
    bus(bus), mctpd_fd(mctpd_fd) { do_init(); }

    // pollfds handle
    int  polling();
    void update_pollfds();

    // send and recv
    int client_send_data(int index, uint8_t * buf, size_t size);
    int send_fw_client_data(uint8_t * buf, size_t size);
    // int send_data(int fd, uint8_t * buf, size_t buf_size);
    int recv_data(int fd, uint8_t ** buf, size_t &size);

    // client handle
    bool add_client(int fd, uint8_t type);
    void pop_client(int index);
    int get_client_fd(uint8_t iid);
    int get_client_count(){ return get_count(clients_data); }
    void show_clients();

    // check sockets if there is an event then return fd else return 0
    int check_mctpd_socket();
    int check_pldmd_socket();
    int check_pldmd_fwupdate_socket();
    int check_clients(int index);

  protected:
    // to distinguish client connect pldmd with
    // whether pldm-mux or pldm-fwup-mux
    // replace fd_handler::client_fds
    std::vector<pldm_client> clients_data = {};
    // pldmd only support update one device at a time.
    bool is_fw_update_active = false;
    int fw_update_client = 0;
    pldm::InstanceId ids;
    void do_init();
    void do_pop_client(int index);

  private:
    const std::string pldmd_socket = "pldm-mux";
    const std::string pldmd_fwupdate_socket = "pldm-fwup-mux";
};
