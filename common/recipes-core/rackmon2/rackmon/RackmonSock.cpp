// Copyright 2021-present Facebook. All Rights Reserved.
#include "RackmonSvcUnix.hpp"

using namespace rackmonsvc;

std::tuple<struct sockaddr_un, size_t> RackmonSock::getServiceAddr() {
  struct sockaddr_un ret {};
  ret.sun_family = AF_UNIX;
  std::strcpy(ret.sun_path, kSockPath.data());
  return std::make_tuple(ret, kSockPath.size() + sizeof(ret.sun_family));
}

int RackmonSock::getServiceSock() {
  int ret = socket(AF_UNIX, SOCK_STREAM, 0);
  if (ret < 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "socket_creation");
  }
  return ret;
}

int RackmonSock::createService() {
  int sock = getServiceSock();
  unlink(kSockPath.data());
  auto [local, sockLen] = getServiceAddr();
  if (bind(sock, (struct sockaddr*)&local, sockLen) != 0) {
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

int RackmonSock::createClient() {
  int sock = getServiceSock();
  auto [rackmondAddr, addrLen] = getServiceAddr();
  if (connect(sock, (struct sockaddr*)&rackmondAddr, addrLen)) {
    close(sock);
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "socket_listen");
  }
  return sock;
}

RackmonSock::~RackmonSock() {
  if (sock_ != -1)
    close(sock_);
}

void RackmonSock::sendChunk(const char* buf, uint16_t bufLen) {
  if (::send(sock_, &bufLen, sizeof(bufLen), 0) < 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "send header");
  }
  if (bufLen == 0)
    return;
  uint16_t sentSize = 0;
  while (sentSize < bufLen) {
    int chunkSize = ::send(sock_, buf, bufLen - sentSize, 0);
    if (chunkSize < 0) {
      throw std::system_error(
          std::error_code(errno, std::generic_category()), "send body");
    }
    sentSize += (uint16_t)chunkSize;
    buf += chunkSize;
  }
}

void RackmonSock::send(const char* buf, size_t len) {
  const size_t kMaxChunkSize = 0xffff;
  // off == len is a special condition where we send a dummy
  // buf. This special condition of buf_len = 0 is handled
  // in sendChunk. Hence the condition of off <= len
  for (size_t off = 0; off <= len; off += kMaxChunkSize) {
    size_t rem = len - off;
    size_t csize = rem > kMaxChunkSize ? kMaxChunkSize : rem;
    sendChunk(buf + off, csize);
  }
}

bool RackmonSock::recvChunk(std::vector<char>& resp) {
  uint16_t recvLen;
  if (::recv(sock_, &recvLen, sizeof(recvLen), 0) < 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "recv header");
  }
  // Received dummy, that was our last chunk!
  if (recvLen == 0)
    return false;
  size_t off = resp.size();
  resp.resize(off + recvLen);
  size_t received = 0;
  char* recvBuf = resp.data() + off;
  while (received < recvLen) {
    int chunkSize = ::recv(sock_, recvBuf, recvLen - received, 0);
    if (chunkSize < 0) {
      throw std::system_error(
          std::error_code(errno, std::generic_category()), "recv body");
    }
    received += (size_t)chunkSize;
    recvBuf += (size_t)chunkSize;
  }
  // If we received the max size, we need to receive another
  // chunk. Return true, so recv() does another iteration.
  return recvLen == 0xffff;
}

void RackmonSock::recv(std::vector<char>& resp) {
  resp.clear();
  // recvchunk returns true if there are more chunks to receive.
  // iterate over it. recvchunk will resize resp as needed.
  while (recvChunk(resp) == true)
    ;
}
