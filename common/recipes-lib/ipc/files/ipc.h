#ifndef _IPC_CLIENT_H_
#define _IPC_CLIENT_H_
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_ENDPOINT_LEN 32

struct client_s;
typedef struct client_s client_t;

typedef int (*ipc_handle_req_t)(client_t *cli);

struct service_s;
typedef struct service_s service_t;

struct client_s {
  char endpoint[MAX_ENDPOINT_LEN];
  void *svc_cookie;
  int fd;
  service_t *svc;
};

int ipc_send_req(const char *endpoint, uint8_t *req, size_t req_len, uint8_t *resp, size_t *resp_len, int timeout);
int ipc_recv_req(client_t *cli, uint8_t *req, size_t *req_len, int timeout);
int ipc_send_resp(client_t *cli, uint8_t *resp, size_t resp_len);
int ipc_start_svc(const char *endpoint, ipc_handle_req_t handle_req, int max_active, void *cookie, pthread_t *waiter);

#endif
