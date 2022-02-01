// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <string_view>
#include <system_error>
#include <tuple>
#include <vector>

namespace rackmonsvc {

class RackmonSock {
 protected:
  int sock_ = -1;
  static constexpr std::string_view kSockPath{"/var/run/rackmond.sock"};
  std::tuple<struct sockaddr_un, size_t> getServiceAddr();
  int getServiceSock();
  int createService();
  int createClient();

  void sendChunk(const char* buf, uint16_t bufLen);
  bool recvChunk(std::vector<char>& resp);

 public:
  explicit RackmonSock(int fd) : sock_(fd) {}
  int getSock() const {
    return sock_;
  }
  virtual ~RackmonSock();
  void send(const char* buf, size_t len);
  void recv(std::vector<char>& resp);
};

class RackmonService : public RackmonSock {
 public:
  RackmonService() : RackmonSock(createService()) {}
};

class RackmonClient : public RackmonSock {
 public:
  RackmonClient() : RackmonSock(createClient()) {}
};

} // namespace rackmonsvc
