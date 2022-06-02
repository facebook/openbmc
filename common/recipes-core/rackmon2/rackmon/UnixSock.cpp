// Copyright 2021-present Facebook. All Rights Reserved.
#include "UnixSock.h"
#include <Log.h>
#include <poll.h>
#include <unistd.h>
#include <csignal>
#include <thread>

namespace rackmonsvc {

std::list<UnixService*> UnixService::activeServiceList{};
std::mutex UnixService::activeServiceListLock{};

std::tuple<struct sockaddr_un, size_t> UnixSock::getServiceAddr(
    const std::string& sockPath) {
  struct sockaddr_un ret {};
  ret.sun_family = AF_UNIX;
  std::strncpy(ret.sun_path, sockPath.c_str(), sizeof(ret.sun_path) - 1);
  return std::make_tuple(ret, sockPath.size() + sizeof(ret.sun_family));
}

int UnixSock::getServiceSock() {
  int ret = socket(AF_UNIX, SOCK_STREAM, 0);
  if (ret < 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "socket_creation");
  }
  return ret;
}

int UnixSock::createService(const std::string& sockPath) {
  int sock = getServiceSock();
  if (access(sockPath.c_str(), F_OK) != -1) {
    logWarn << "WARNING: " << sockPath << " is being recreated" << std::endl;
    unlink(sockPath.c_str());
  }
  auto [local, sockLen] = getServiceAddr(sockPath);
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

int UnixSock::createClient(const std::string& sockPath) {
  int sock = getServiceSock();
  auto [rackmondAddr, addrLen] = getServiceAddr(sockPath);
  if (connect(sock, (struct sockaddr*)&rackmondAddr, addrLen)) {
    close(sock);
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "socket_connect");
  }
  return sock;
}

UnixSock::~UnixSock() {
  if (sock_ != -1)
    close(sock_);
}

void UnixSock::sendChunk(const char* buf, uint16_t bufLen) {
  const int maxRetries = 3;
  int retries = 0;
  while (::send(sock_, &bufLen, sizeof(bufLen), 0) < 0) {
    retries++;
    if (retries == maxRetries) {
      throw std::system_error(
          std::error_code(errno, std::generic_category()), "send header");
    }
  }
  if (bufLen == 0)
    return;
  uint16_t sentSize = 0;
  retries = 0;
  while (sentSize < bufLen) {
    int chunkSize = ::send(sock_, buf, bufLen - sentSize, 0);
    if (chunkSize < 0) {
      if (retries == maxRetries) {
        throw std::system_error(
            std::error_code(errno, std::generic_category()), "send body");
      }
      continue;
    }
    retries = 0;
    sentSize += (uint16_t)chunkSize;
    buf += chunkSize;
  }
}

void UnixSock::send(const char* buf, size_t len) {
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

bool UnixSock::recvChunk(std::vector<char>& resp) {
  uint16_t recvLen;
  const int maxRetries = 3;
  int retries = 0;
  while (::recv(sock_, &recvLen, sizeof(recvLen), 0) < 0) {
    retries++;
    if (retries == maxRetries) {
      throw std::system_error(
          std::error_code(errno, std::generic_category()), "recv header");
    }
  }
  // Received dummy, that was our last chunk!
  if (recvLen == 0)
    return false;
  size_t off = resp.size();
  resp.resize(off + recvLen);
  size_t received = 0;
  char* recvBuf = resp.data() + off;
  retries = 0;
  while (received < recvLen) {
    int chunkSize = ::recv(sock_, recvBuf, recvLen - received, 0);
    if (chunkSize < 0) {
      retries++;
      if (retries == maxRetries) {
        throw std::system_error(
            std::error_code(errno, std::generic_category()), "recv body");
      }
      continue;
    }
    retries = 0;
    received += (size_t)chunkSize;
    recvBuf += (size_t)chunkSize;
  }
  // If we received the max size, we need to receive another
  // chunk. Return true, so recv() does another iteration.
  return recvLen == 0xffff;
}

void UnixSock::recv(std::vector<char>& resp) {
  resp.clear();
  // recvchunk returns true if there are more chunks to receive.
  // iterate over it. recvchunk will resize resp as needed.
  while (recvChunk(resp) == true)
    ;
}

void UnixService::triggerExit(int /* unused */) {
  std::unique_lock lk(activeServiceListLock);
  for (auto& ent : activeServiceList) {
    ent->requestExit();
  }
}

void UnixService::registerStaticExitHandler() {
  struct sigaction ign_action, exit_action;
  ign_action.sa_flags = 0;
  sigemptyset(&ign_action.sa_mask);
  ign_action.sa_handler = SIG_IGN;
  if (sigaction(SIGPIPE, &ign_action, NULL) != 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "SIGPIPE handler");
  }

  exit_action.sa_flags = 0;
  sigemptyset(&exit_action.sa_mask);
  exit_action.sa_handler = UnixService::triggerExit;
  if (sigaction(SIGTERM, &exit_action, NULL) != 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "SIGTERM handler");
  }
  if (sigaction(SIGINT, &exit_action, NULL) != 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "SIGINT handler");
  }
}

void UnixService::unregisterStaticExitHandler() {
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);
}

void UnixService::registerExitHandler() {
  if (pipe(backChannelFDs_) != 0) {
    std::system_error(
        std::error_code(errno, std::generic_category()),
        "Backchannel Pipe creation");
  }
  std::unique_lock lk(activeServiceListLock);
  activeServiceList.push_back(this);
  if (activeServiceList.size() == 1) {
    registerStaticExitHandler();
  }
}

void UnixService::unregisterExitHandler() {
  std::unique_lock lk(activeServiceListLock);
  for (auto it = activeServiceList.begin(); it != activeServiceList.end();
       ++it) {
    if (*it == this) {
      activeServiceList.erase(it);
      break;
    }
  }
  if (activeServiceList.size() == 0) {
    unregisterStaticExitHandler();
  }
  if (backChannelRequestor_ != -1) {
    close(backChannelRequestor_);
    backChannelRequestor_ = -1;
  }
  if (backChannelHandler_ != -1) {
    close(backChannelHandler_);
    backChannelHandler_ = -1;
  }
}

void UnixService::initialize(int /* argc */, char** /* argv */) {
  registerExitHandler();
  sock_ = std::make_unique<UnixServiceSock>(sockPath_);
}

void UnixService::deinitialize() {
  unregisterExitHandler();
  unlink(sockPath_.c_str());
}

void UnixService::requestExit() {
  char c = 'c';
  logInfo << "Got request exit" << std::endl;
  if (write(backChannelRequestor_, &c, 1) != 1) {
    logError << "Could not request rackmon svc to exit!" << std::endl;
  }
}

void UnixService::handleConnection(std::unique_ptr<UnixSock> sock) {
  std::vector<char> buf;
  try {
    sock->recv(buf);
  } catch (...) {
    logError << "Failed to receive message" << std::endl;
    return;
  }
  handleRequest(buf, std::move(sock));
}

void UnixService::doLoop() {
  struct sockaddr_un client;
  struct pollfd pfd[2] = {
      {sock_->getSock(), POLLIN, 0},
      {backChannelHandler_, POLLIN, 0},
  };

  while (1) {
    int ret;
    socklen_t clisocklen = sizeof(struct sockaddr_un);

    ret = poll(pfd, 2, -1);
    if (ret <= 0) {
      // This should be the common case. The entire thing
      // with the pipe is to handle the race condition when
      // we get a signal when we are actively handling a
      // previous request.
      logInfo << "Handling termination signal" << std::endl;
      break;
    }
    if (pfd[1].revents & POLLIN) {
      char c;
      if (read(pfd[1].fd, &c, 1) != 1) {
        logError << "Got something but no data!" << std::endl;
      } else if (c == 'c') {
        logInfo << "Handling termination request" << std::endl;
        break;
      } else {
        logError << "Got unknown command: " << c << std::endl;
      }
    }
    if (pfd[0].revents & POLLIN) {
      int clifd =
          accept(sock_->getSock(), (struct sockaddr*)&client, &clisocklen);
      if (clifd < 0) {
        logError << "Failed to accept new connection" << std::endl;
        continue;
      }
      auto clisock = std::make_unique<UnixSock>(clifd);
      handleConnection(std::move(clisock));
    }
  }
}

} // namespace rackmonsvc
