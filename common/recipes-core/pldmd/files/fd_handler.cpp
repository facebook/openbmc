#include "fd_handler.hpp"

#include <stdio.h>
#include <cstring>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <glog/logging.h>
#include <iomanip>

using namespace std;

int
fd_handler::polling() {
  size_t fd_count = get_count(tx_server_fds, server_fds, client_fds);
  return do_polling(fd_count);
};

void
fd_handler::update_pollfds() {

  if (!client_change) 
    return;

  size_t index;
  size_t fd_count = get_count(tx_server_fds, server_fds, client_fds);

  void* result = realloc(pollfds, fd_count * sizeof(struct pollfd));
  if (result == NULL)
    return;
  else
    pollfds = (struct pollfd*)result;

  index = get_count(tx_server_fds, server_fds);
  for (auto& fd : client_fds) {
    pollfds[index].fd = fd;
    pollfds[index].events = POLLIN;
    ++index;
  }

  client_change = false;
};

int 
fd_handler::check_pollfds (int index)
{
  struct pollfd* pfd = get_pollfd(index);
  if (pfd->revents)
    return pfd->fd;
  else
    return 0;
}

struct pollfd* fd_handler::get_pollfd (int index) {
  return pollfds + index;
}

int fd_handler::send_data (int fd, uint8_t * buf, size_t size)
{
  return send (fd, buf, size, 0);
}

int fd_handler::recv_data (int fd, uint8_t **buf, size_t &size) 
{
  int returnCode = 0;
  ssize_t peekedLength = recv(fd, nullptr, 0, MSG_PEEK | MSG_TRUNC);

  if (peekedLength <= 0) {
    // stream socket peer has performed an orderly shutdown.
    if (peekedLength == 0 && errno == 0)
      returnCode = -1;
    else {
      returnCode = -errno;
      LOG(ERROR) << "recv system call failed, RC = " 
                << (int)peekedLength
                << ", -errno = "
                << returnCode;
    }
    return returnCode;
  }

  size = peekedLength;
  *buf = (uint8_t *) malloc (size);
  returnCode = recv(fd, *buf, size, 0);
  if (returnCode < 0) {
    if (errno != ECONNRESET)
      LOG(ERROR) << "can't receive from client";
    returnCode = -1;
    return returnCode;
  }

  return returnCode;
}

void fd_handler::add_client(int fd)
{
  add_element(client_fds, fd);
  client_change = true;
}

void fd_handler::pop_client(int index)
{
  pop_element(client_fds, index);
  client_change = true;
}

void
fd_handler::init_tx_server_fd (int fd) {
  tx_server_fds.push_back(fd);
}

/* init server socket for clients */
void 
fd_handler::init_server_fd (int& fd, const std::string &path)
{
  int returnCode = 0;
  if ((fd = socket(AF_UNIX, SOCK_SEQPACKET, 0)) == 0)
  {
    returnCode = -errno;
    LOG(ERROR) << "Failed to create the socket = "
               << path.c_str()
               << "RC = "
               << returnCode;
    exit(EXIT_FAILURE);
  }

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  memcpy(addr.sun_path, "\0", 1);
  memcpy(addr.sun_path+1, path.c_str(), path.length());
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr.sun_family)+path.length()+1))
  {
    returnCode = -errno;
    LOG(ERROR) << "Failed to bind the socket = " << path.c_str()
               << "RC = "
               << returnCode;
    exit(EXIT_FAILURE);
  }

  if (listen(fd, 1))
  {
    returnCode = -errno;
    LOG(ERROR) << "Failed to listen the socket = " << path.c_str()
               << "RC = "
               << returnCode;
    exit(EXIT_FAILURE);
  }

  server_fds.push_back(fd);
}

void
fd_handler::init_pollfds ()
{
  // init poll fds to monitor all socket
  size_t default_fd_counts = server_fds.size() + tx_server_fds.size();
  pollfds = (struct pollfd*) malloc (default_fd_counts * sizeof(struct pollfd));

  size_t index = 0;
  for (size_t i = 0; i < tx_server_fds.size(); ++i, ++index) {
    pollfds[index].fd = tx_server_fds[i];
    pollfds[index].events = POLLIN;
  }
  for (size_t i = 0; i < server_fds.size(); ++i, ++index) {
    pollfds[index].fd = server_fds[i];
    pollfds[index].events = POLLIN;
  }
}

int
fd_handler::do_polling (size_t fd_count)
{
  int returnCode = poll(pollfds, fd_count, -1);
  if (returnCode < 0) {
    LOG(ERROR) << "Failed to poll fds, RC = " << returnCode;
  }
  return returnCode;
}
