/*
 * ftdi_mdio_daemon.c
 * Holds the FTDI USB connection and serves CLI commands over a Linux socket.
 *
 * Usage: ftdi-mdio-daemon -d <devaddr> -i <interface> [--debug]
 */

#include "ftdi_mdio_daemon.h"
#include "ftdi_mdio_core.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>

/* ── mutex: protects ftdic against concurrent frame writes from multiple
 * clients ── */
static pthread_mutex_t g_ftdi_lock = PTHREAD_MUTEX_INITIALIZER;

/* ── USB reconnect parameters ── */
static int g_device = -1;
static int g_interface = -1;

static int reconnect(void) {
  log_print(LOG_WARN, "[daemon] USB lost, reconnecting...\n");
  device_close();
  int r = device_open(g_device, g_interface);
  if (r < 0)
    log_print(LOG_ERROR, "[daemon] reconnect failed\n");
  else
    log_print(LOG_INFO, "[daemon] reconnect ok\n");
  return r;
}

/* ── handle a single client connection ── */
static void handle_client(int cli_fd) {
  mdio_request_t req;
  mdio_response_t rsp;
  ssize_t n;

  memset(&rsp, 0, sizeof(rsp));

  n = recv(cli_fd, &req, sizeof(req), MSG_WAITALL);
  if (n == 0) {
    /* client closed connection without sending data, ignore silently */
    rsp.status = 0;
    goto done;
  }
  if (n != (ssize_t)sizeof(req)) {
    log_print(LOG_WARN, "[daemon] short recv (%zd)\n", n);
    rsp.status = -1;
    goto done;
  }

  pthread_mutex_lock(&g_ftdi_lock);
  uint16_t mdio_data;
  switch ((ipc_action_t)req.action) {
    case IPC_MDIO_READ:
      rsp.status = mdio_read_c22(req.phyid, req.regaddr, &mdio_data);
      rsp.value = mdio_data;
      break;

    case IPC_MDIO_WRITE:
      rsp.status = mdio_write_c22(req.phyid, req.regaddr, req.value);
      break;

    case IPC_MDIO_READ_C45:
      rsp.status = mdio_read_c45(req.phyid, req.devad, req.regaddr, &mdio_data);
      rsp.value = mdio_data;
      break;

    case IPC_MDIO_WRITE_C45:
      rsp.status = mdio_write_c45(req.phyid, req.devad, req.regaddr, req.value);
      break;

    case IPC_GPIO_GET:
      rsp.status = gpio_get();
      rsp.value = gpio_s.level;
      break;

    case IPC_GPIO_SET:
      rsp.status =
          gpio_set((uint8_t)req.regaddr, req.gpio_dir, (uint8_t)req.value);
      rsp.value = gpio_s.level;
      break;

    default:
      log_print(LOG_WARN, "[daemon] unknown action %d\n", req.action);
      rsp.status = -2;
      break;
  }

  /* If the FTDI operation failed, attempt one reconnect and retry. */
  if (rsp.status < 0) {
    if (reconnect() == 0) {
      log_print(LOG_INFO, "[daemon] retry after reconnect\n");
      /* Retry once (simplified: only retries read operations). */
      switch ((ipc_action_t)req.action) {
        case IPC_MDIO_READ:
          rsp.status = mdio_read_c22(req.phyid, req.regaddr, &mdio_data);
          rsp.value = mdio_data;
          break;
        case IPC_MDIO_READ_C45:
          rsp.status =
              mdio_read_c45(req.phyid, req.devad, req.regaddr, &mdio_data);
          rsp.value = mdio_data;
          break;
        default:
          break;
      }
    }
  }

  pthread_mutex_unlock(&g_ftdi_lock);

done:
  send(cli_fd, &rsp, sizeof(rsp), MSG_NOSIGNAL);
  close(cli_fd);
}

/* ── signal handler: graceful shutdown ── */
static volatile int g_running = 1;
static void sig_handler(int s) {
  g_running = 0;
  (void)s;
}

static void print_usage(void) {
  fprintf(
      stderr,
      "Usage: ftdi-mdio-daemon -d <devaddr> -i <1-4> [--debug] [--quiet]\n"
      "  -d : USB device address (from 'ftdi_mdio list')\n"
      "  -i : FTDI interface (1~4)\n");
}

int main(int argc, char* argv[]) {
  prctl(PR_SET_PDEATHSIG, SIGTERM);
  int opt_device = -1, opt_intf = -1;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0 && i + 1 < argc)
      opt_device = (int)strtol(argv[++i], NULL, 0);
    else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc)
      opt_intf = (int)strtol(argv[++i], NULL, 0);
    else if (strcmp(argv[i], "--debug") == 0)
      g_log_level = LOG_DEBUG;
    else if (strcmp(argv[i], "--quiet") == 0)
      g_log_level = LOG_ERROR;
  }

  if (opt_device < 0 || opt_intf < 0) {
    print_usage();
    return 2;
  }

  g_device = opt_device;
  g_interface = opt_intf;

  if (device_open(g_device, g_interface) < 0) {
    fprintf(stderr, "[daemon] device_open failed\n");
    return 1;
  }
  log_print(
      LOG_INFO,
      "[daemon] FTDI opened (dev=%d intf=%d)\n",
      g_device,
      g_interface);

  /* ── Unix socket setup ── */
  int srv = socket(AF_UNIX, SOCK_STREAM, 0);
  if (srv < 0) {
    perror("socket");
    return 1;
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, FTDI_MDIO_SOCK_PATH, sizeof(addr.sun_path) - 1);

  unlink(FTDI_MDIO_SOCK_PATH);
  if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return 1;
  }
  chmod(FTDI_MDIO_SOCK_PATH, 0666); /* allow non-root users to connect */
  listen(srv, 8);

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = sig_handler;
  /* Do NOT set SA_RESTART — we need SIGINT to interrupt accept() with EINTR */
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  log_print(LOG_INFO, "[daemon] listening on %s\n", FTDI_MDIO_SOCK_PATH);

  while (g_running) {
    int cli = accept(srv, NULL, NULL);
    if (cli < 0) {
      if (errno == EINTR)
        break;
      perror("accept");
      continue;
    }
    handle_client(cli); /* single-threaded: serve one client at a time */
  }

  log_print(LOG_INFO, "[daemon] shutting down\n");
  close(srv);
  unlink(FTDI_MDIO_SOCK_PATH);
  device_close();
  return 0;
}
