#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "ipc.h"

char *svc_cookie = "test_cookie";

int test_handle_req(client_t *cli)
{
  uint8_t req[32] = {0};
  uint8_t resp[32] = {1,2,3,4};
  size_t len = 32;
  static int req_num = 0;
  int reqno = req_num++;

  assert(strcmp(cli->endpoint, "test_svc") == 0);
  assert(cli->svc_cookie == svc_cookie);
  if (ipc_recv_req(cli, req, &len, 1) != 0) {
    assert(0);
    return -1;
  }
  if (reqno < 2)
    sleep(10);
  assert(memcmp(req, resp, 4) == 0);

  if (ipc_send_resp(cli, resp, 4) != 0) {
    if (reqno != 0)
      assert(0);
    return -1;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  int rc;
  rc = ipc_start_svc("test_svc", test_handle_req, 1, svc_cookie, NULL);
  assert(rc == 0);
  sleep(1);
  uint8_t req[32] = {1,2,3,4};
  uint8_t resp[32] = {0};
  size_t resp_len = 32;
  rc = ipc_send_req("test_svc", req, 4, resp, &resp_len, 2);
  assert(rc != 0);
  printf("PASSED: Test request timeout is honored\n");
  rc = ipc_send_req("test_svc", req, 4, resp, &resp_len, 20);
  assert(rc == 0);
  assert(memcmp(req, resp, 4) == 0);
  printf("PASSED: Test single request is sent and received correctly\n");
  for (int i = 0; i < 10; i++) {
    memset(resp, 0, sizeof(resp));
    rc = ipc_send_req("test_svc", req, 4, resp, &resp_len, 1);
    assert(rc == 0);
    assert(memcmp(req, resp, 4) == 0);
  }
  printf("PASSED: Multiple request\n");
  return 0;
}
