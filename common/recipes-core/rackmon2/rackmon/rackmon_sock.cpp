#include "rackmon_svc_unix.hpp"

std::tuple<struct sockaddr_un, size_t> RackmonSock::get_service_addr() {
  struct sockaddr_un ret = {
      .sun_family = AF_UNIX,
  };
  std::strcpy(ret.sun_path, sock_path.data());
  return std::make_tuple(ret, sock_path.size() + sizeof(ret.sun_family));
}

int RackmonSock::get_service_sock() {
  int ret = socket(AF_UNIX, SOCK_STREAM, 0);
  if (ret < 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "socket_creation");
  }
  return ret;
}

int RackmonSock::create_service() {
  int sock = get_service_sock();
  unlink(sock_path.data());
  auto [local, sock_len] = get_service_addr();
  if (bind(sock, (struct sockaddr*)&local, sock_len) != 0) {
    close(sock);
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "socket_bind");
  }
  if (listen(sock, 20) != 0) {
    close(sock);
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "socket_listen");
  }
  return sock;
}

int RackmonSock::create_client() {
  int sock = get_service_sock();
  auto [rackmond_addr, addr_len] = get_service_addr();
  if (connect(sock, (struct sockaddr*)&rackmond_addr, addr_len)) {
    close(sock);
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "socket_listen");
  }
  return sock;
}

RackmonSock::~RackmonSock() {
  if (sock != -1)
    close(sock);
}

void RackmonSock::send(const char* buf, size_t len) {
  uint16_t buf_len = uint16_t(len);
  if (::send(sock, &buf_len, sizeof(buf_len), 0) < 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "send header");
  }
  if (::send(sock, buf, len, 0) < 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "send body");
  }
}

void RackmonSock::recv(std::vector<char>& resp) {
  uint16_t recv_len;
  if (::recv(sock, &recv_len, sizeof(recv_len), 0) < 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "recv header");
  }
  resp.resize(recv_len);
  if (::recv(sock, resp.data(), recv_len, 0) < 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "recv body");
  }
}
