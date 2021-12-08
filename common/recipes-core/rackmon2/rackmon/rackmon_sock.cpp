#include "rackmon_svc_unix.hpp"

std::tuple<struct sockaddr_un, size_t> RackmonSock::get_service_addr() {
  struct sockaddr_un ret{};
  ret.sun_family = AF_UNIX;
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

void RackmonSock::sendchunk(const char *buf, uint16_t buf_len)
{
  if (::send(sock, &buf_len, sizeof(buf_len), 0) < 0) {
    throw std::system_error(
      std::error_code(errno, std::generic_category()), "send header");
  }
  if (buf_len == 0)
    return;
  uint16_t sent_size = 0;
  while (sent_size < buf_len) {
    int chunk_size = ::send(sock, buf, buf_len - sent_size, 0);
    if (chunk_size < 0) {
      throw std::system_error(
        std::error_code(errno, std::generic_category()), "send body");
    }
    sent_size += (uint16_t)chunk_size;
    buf += chunk_size;
  }
}

void RackmonSock::send(const char* buf, size_t len) {
  const size_t max_chunk_size = 0xffff;
  // off == len is a special condition where we send a dummy
  // buf. This special condition of buf_len = 0 is handled
  // in sendchunk. Hence the condition of off <= len
  for (size_t off = 0; off <= len; off += max_chunk_size) {
    size_t rem = len - off;
    size_t csize = rem > max_chunk_size ? max_chunk_size : rem;
    sendchunk(buf + off, csize);
  }
}

bool RackmonSock::recvchunk(std::vector<char>& resp)
{
  uint16_t recv_len;
  if (::recv(sock, &recv_len, sizeof(recv_len), 0) < 0) {
    throw std::system_error(
      std::error_code(errno, std::generic_category()), "recv header");
  }
  // Received dummy, that was our last chunk!
  if (recv_len == 0)
    return false;
  size_t off = resp.size();
  resp.resize(off + recv_len);
  size_t received = 0;
  char *recv_buf = resp.data() + off;
  while (received < recv_len) {
    int chunk_size = ::recv(sock, recv_buf, recv_len - received, 0);
    if (chunk_size < 0) {
      throw std::system_error(
        std::error_code(errno, std::generic_category()), "recv body");
    }
    received += (size_t)chunk_size;
    recv_buf += (size_t)chunk_size;
  }
  // If we received the max size, we need to receive another
  // chunk. Return true, so recv() does another iteration.
  return recv_len == 0xffff;
}

void RackmonSock::recv(std::vector<char>& resp) {
  resp.clear();
  // recvchunk returns true if there are more chunks to receive.
  // iterate over it. recvchunk will resize resp as needed.
  while (recvchunk(resp) == true);
}
