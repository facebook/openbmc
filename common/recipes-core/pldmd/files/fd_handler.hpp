#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <sys/poll.h>
#include "vector_handler.hpp"

class fd_handler
{
  public:
    // pollfds handle
    int  polling();
    void update_pollfds();

    int check_pollfds(int index); // check pollfd if event happen return fd, else return 0
    struct pollfd* get_pollfd(int index);

    // send and recv
    int send_data(int fd, uint8_t * buf, size_t size);
    int recv_data(int fd, uint8_t ** buf, size_t &size);

    // client handle
    void add_client(int fd);
    void pop_client(int index);
    int get_client_count(){ return get_count(client_fds); };


  protected:
    /*
     * pollfds =
     *   {
     *     tx server fd,
     *         .
     *         .
     *     server fd,
     *         .
     *         .
     *     client fd,
     *         .
     *         .
     *   }
     */
    std::vector<int> tx_server_fds = {};
    std::vector<int> server_fds = {};
    std::vector<int> client_fds = {};
    struct pollfd *pollfds = nullptr;

    // flag for whether pollfds need to updated
    bool client_change = false;

    void init_tx_server_fd(int fd);
    void init_server_fd(int& fd, const std::string& path);
    void init_pollfds();

    int  do_polling(size_t fd_count);
    // void do_update_pollfd(size_t fd_count);
};
