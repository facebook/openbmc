#pragma once
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <string_view>
#include <system_error>
#include <tuple>
#include <vector>

class RackmonSock {
 protected:
  int sock = -1;
  static constexpr std::string_view sock_path{"/var/run/rackmond.sock"};
  std::tuple<struct sockaddr_un, size_t> get_service_addr();
  int get_service_sock();
  int create_service();
  int create_client();

  void sendchunk(const char *buf, uint16_t buf_len);
  bool recvchunk(std::vector<char>& resp);
 public:
  RackmonSock(int fd) : sock(fd) {}
  int get_sock() {
    return sock;
  }
  virtual ~RackmonSock();
  void send(const char* buf, size_t len);
  void recv(std::vector<char>& resp);
};

class RackmonService : public RackmonSock {
 public:
  RackmonService() : RackmonSock(create_service()) {}
};

class RackmonClient : public RackmonSock {
 public:
  RackmonClient() : RackmonSock(create_client()) {}
};
