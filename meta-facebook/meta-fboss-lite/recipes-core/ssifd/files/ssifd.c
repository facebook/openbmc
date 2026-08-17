/*
 * Copyright (c) 2021 Ampere Computing LLC
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This is a daemon that forwards requests and receive responses from SSIF over
 * the D-Bus IPMI Interface.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <openbmc/ipmi.h>
#include <openbmc/log.h>

//#define DEBUG

#define DEFAULT_SSIF_DEV "/dev/ipmi-ssif-host"

/* Max SSIF payload per spec */
#define SSIF_PAYLOAD_MAX 254

/* The device read/write buffer: 4-byte length prefix + payload */
#define SSIF_BUF_SIZE (SSIF_PAYLOAD_MAX + sizeof(unsigned int))

/* Payload ID for single-host systems */
#define FRU_SERVER 0x01

/*
 * SSIF driver(ipmi_ssif.c) timeout is 15 seconds; we must respond before that.
 * Use 14 seconds as our internal deadline.
 */
#define HOST_RSP_TIMEOUT_SEC 14

/* Completion code when response is not available in time */
#define CC_TIMEOUT 0xCE

static int ssif_fd = -1;
static int verbose_logging = 0;
static volatile sig_atomic_t g_running = 1;

#define SSIFD_VERBOSE(fmt, args...) \
  do {                              \
    if (verbose_logging)            \
      OBMC_INFO(fmt, ##args);       \
  } while (0)

/*
 * Timeout handling:
 * When a request comes in, we spawn a detached timer thread.
 * If lib_ipmi_handle() returns before the deadline, we cancel
 * the timer. If the timer fires first, it sends a 0xCE response
 * and sets a flag so the main thread knows to discard its late
 * response.
 *
 * A generation counter (g_timer_gen) prevents stale timer threads
 * from acting on newer requests.
 */
static pthread_mutex_t g_timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_timer_cond;
static bool g_timer_active = false;
static bool g_timed_out = false;
static unsigned int g_timer_gen = 0;

/* Saved request header for building timeout response */
static uint8_t g_req_netfn = 0;
static uint8_t g_req_lun = 0;
static uint8_t g_req_cmd = 0;

static void signal_handler(int sig) {
  (void)sig;

  g_running = 0;
}

static void setup_signals(void) {
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);
}

static void init_timer_cond(void) {
  pthread_condattr_t attr;

  pthread_condattr_init(&attr);
  pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
  pthread_cond_init(&g_timer_cond, &attr);
  pthread_condattr_destroy(&attr);
}

static int ssif_write_response(const uint8_t *buf, size_t len) {
  ssize_t wlen;

  wlen = write(ssif_fd, buf, len);
  if (wlen < 0 || (size_t)wlen != len) {
    OBMC_ERROR(errno, "ssif write failed: expect=%zu wrote=%zd", len, wlen);
    return -1;
  }
  return 0;
}

static void send_timeout_response(void) {
  uint8_t rsp[sizeof(unsigned int) + 3];
  unsigned int *plen;
  size_t idx;

  plen = (unsigned int *)rsp;
  idx = sizeof(unsigned int);

  *plen = 3; /* netfn/lun + cmd + cc */
  rsp[idx++] = (uint8_t)(((g_req_netfn + 1) << 2) | (g_req_lun & 0x03));
  rsp[idx++] = g_req_cmd;
  rsp[idx++] = CC_TIMEOUT;

  OBMC_INFO(
      "SSIF timeout: netfn=0x%02x lun=%u cmd=0x%02x, sending cc=0x%02x",
      g_req_netfn, g_req_lun, g_req_cmd, CC_TIMEOUT);

  ssif_write_response(rsp, sizeof(rsp));
}

struct timer_arg {
  unsigned int gen;
};

static void *timer_thread_fn(void *arg) {
  struct timer_arg *targ;
  unsigned int my_gen;
  struct timespec ts;
  int rc;

  targ = (struct timer_arg *)arg;
  my_gen = targ->gen;

  free(targ);

  pthread_mutex_lock(&g_timer_mutex);
  clock_gettime(CLOCK_MONOTONIC, &ts);
  ts.tv_sec += HOST_RSP_TIMEOUT_SEC;

  rc = pthread_cond_timedwait(&g_timer_cond, &g_timer_mutex, &ts);
  if (rc == ETIMEDOUT && my_gen == g_timer_gen) {
    /* Timer expired and we are still the current generation */
    g_timed_out = true;
    pthread_mutex_unlock(&g_timer_mutex);
    send_timeout_response();
  } else {
    /* Cancelled by main thread, or stale generation */
    pthread_mutex_unlock(&g_timer_mutex);
  }

  return NULL;
}

static void start_timer(void) {
  pthread_t tid;
  struct timer_arg *targ;

  pthread_mutex_lock(&g_timer_mutex);
  g_timer_gen++;
  g_timer_active = true;
  g_timed_out = false;

  targ = malloc(sizeof(*targ));
  targ->gen = g_timer_gen;
  pthread_mutex_unlock(&g_timer_mutex);

  pthread_create(&tid, NULL, timer_thread_fn, targ);
  pthread_detach(tid);
}

static void cancel_timer(void) {
  pthread_mutex_lock(&g_timer_mutex);
  if (g_timer_active) {
    g_timer_active = false;
    pthread_cond_broadcast(&g_timer_cond);
  }
  pthread_mutex_unlock(&g_timer_mutex);
}

/*
 * Main SSIF processing loop.
 *
 * Device read format:
 *   [0..3]  unsigned int: payload length (N)
 *   [4]     netfn_lun byte (netfn<<2 | lun)
 *   [5]     cmd
 *   [6..N+3] request data
 *
 * lib_ipmi_handle() expects:
 *   [0]     payload_id
 *   [1]     netfn_lun
 *   [2]     cmd
 *   [3..M]  request data
 *
 * lib_ipmi_handle() returns:
 *   [0]     netfn_lun (response)
 *   [1]     cmd
 *   [2]     completion code
 *   [3..M]  response data
 *
 * Device write format:
 *   [0..3]  unsigned int: payload length
 *   [4..]   response payload (netfn_lun + cmd + cc + data)
 */
static void ssif_loop(void) {
  struct pollfd pfd;
  uint8_t read_buf[SSIF_BUF_SIZE];
  uint8_t ipmi_req[MAX_IPMI_MSG_SIZE];
  uint8_t ipmi_res[MAX_IPMI_MSG_SIZE];
  uint8_t err_rsp[sizeof(unsigned int) + 3];
  uint8_t write_buf[SSIF_BUF_SIZE];
  uint16_t ipmi_res_len;
  ssize_t rlen;
  unsigned int payload_len;
  unsigned int *wlen_field;
  unsigned int *ep;
  uint8_t netfn, lun, cmd;
  bool timed_out;
  int pret;

#ifdef DEBUG
  struct timespec req_tv, res_tv;
  double elapsed;
  char hexdump[512];
  int i;
#endif

  pfd.fd = ssif_fd;
  pfd.events = POLLIN;

  SSIFD_VERBOSE("ssif processing loop started");

  while (g_running) {
    pret = poll(&pfd, 1, 1000); /* 1s timeout to check g_running */
    if (pret < 0) {
      if (errno == EINTR)
        continue;
      OBMC_ERROR(errno, "poll failed");
      break;
    }
    if (pret == 0)
      continue; /* timeout, re-check g_running */

    rlen = read(ssif_fd, read_buf, sizeof(read_buf));
    if (rlen <= 0) {
      if (rlen < 0 && errno != EAGAIN)
        OBMC_ERROR(errno, "ssif read error");
      continue;
    }

    if (rlen < (ssize_t)(sizeof(unsigned int) + 2)) {
      OBMC_WARN("SSIF short read: %zd bytes", rlen);
      continue;
    }

#ifdef DEBUG
    clock_gettime(CLOCK_REALTIME, &req_tv);
    hexdump[0] = '\0';
    for (i = 0; i < (int)rlen && i < 64; i++)
      snprintf(hexdump + strlen(hexdump), sizeof(hexdump) - strlen(hexdump),
               " %02x", read_buf[i]);
    OBMC_WARN("[%ld.%ld] SSIF Req:%s len=%zd",
              req_tv.tv_sec, req_tv.tv_nsec, hexdump, rlen);
#endif

    /* Parse the raw device buffer */
    memcpy(&payload_len, read_buf, sizeof(unsigned int));

    /* Validate payload_len bounds */
    if (payload_len > SSIF_PAYLOAD_MAX ||
        payload_len > (unsigned int)(rlen - sizeof(unsigned int))) {
      OBMC_WARN("SSIF: invalid payload_len=%u (read %zd bytes), dropping",
                payload_len, rlen);
      continue;
    }

    if (payload_len < 2) {
      OBMC_WARN("SSIF: payload too small (%u bytes), dropping", payload_len);
      continue;
    }

    netfn = read_buf[sizeof(unsigned int)] >> 2;
    lun = read_buf[sizeof(unsigned int)] & 0x03;
    cmd = read_buf[sizeof(unsigned int) + 1];

    SSIFD_VERBOSE("SSIF Req: netfn=0x%02x lun=%u cmd=0x%02x data_len=%u",
                  netfn, lun, cmd, payload_len - 2);

    /* Save for timeout handler */
    pthread_mutex_lock(&g_timer_mutex);
    g_req_netfn = netfn;
    g_req_lun = lun;
    g_req_cmd = cmd;
    pthread_mutex_unlock(&g_timer_mutex);

    /* Start response deadline timer */
    start_timer();

    /*
     * Build ipmid request buffer:
     *   [0] = payload_id
     *   [1] = netfn_lun (original byte from device)
     *   [2] = cmd
     *   [3..] = data
     */
    ipmi_req[0] = FRU_SERVER;
    memcpy(&ipmi_req[1], &read_buf[sizeof(unsigned int)], payload_len);

    /* Call ipmid synchronously */
    memset(ipmi_res, 0, sizeof(ipmi_res));
    ipmi_res_len = 0;
    lib_ipmi_handle(ipmi_req, (uint16_t)(payload_len + 1),
                    ipmi_res, &ipmi_res_len);

    /* Cancel the timer - we have a response */
    cancel_timer();

    /* Check if timeout already fired */
    pthread_mutex_lock(&g_timer_mutex);
    timed_out = g_timed_out;
    pthread_mutex_unlock(&g_timer_mutex);

    if (timed_out) {
      SSIFD_VERBOSE("SSIF: dropping late response for netfn=0x%02x cmd=0x%02x",
                    netfn, cmd);
      continue;
    }

    /* Build the response to write to the device */
    if (ipmi_res_len < 3) {
      OBMC_WARN("SSIF: ipmid response too short (%u bytes)", ipmi_res_len);
      /* Fabricate an error response */
      ep = (unsigned int *)err_rsp;
      *ep = 3;
      err_rsp[sizeof(unsigned int)] =
          (uint8_t)(((netfn + 1) << 2) | (lun & 0x03));
      err_rsp[sizeof(unsigned int) + 1] = cmd;
      err_rsp[sizeof(unsigned int) + 2] = 0xFF;
      ssif_write_response(err_rsp, sizeof(err_rsp));
      continue;
    }

    wlen_field = (unsigned int *)write_buf;
    *wlen_field = (unsigned int)ipmi_res_len;
    memcpy(&write_buf[sizeof(unsigned int)], ipmi_res, ipmi_res_len);

    SSIFD_VERBOSE(
        "SSIF Rsp: netfn=0x%02x cmd=0x%02x cc=0x%02x len=%u",
        ipmi_res[0] >> 2, ipmi_res[1], ipmi_res[2], ipmi_res_len);

    ssif_write_response(write_buf, sizeof(unsigned int) + ipmi_res_len);

#ifdef DEBUG
    clock_gettime(CLOCK_REALTIME, &res_tv);
    hexdump[0] = '\0';
    for (i = 0; i < (int)ipmi_res_len && i < 64; i++)
      snprintf(hexdump + strlen(hexdump), sizeof(hexdump) - strlen(hexdump),
               " %02x", ipmi_res[i]);
    OBMC_WARN("[%ld.%ld] SSIF Res:%s", res_tv.tv_sec, res_tv.tv_nsec, hexdump);
    elapsed = (res_tv.tv_sec - req_tv.tv_sec - 1) * 1000.0 +
              (1000.0 + (double)(res_tv.tv_nsec - req_tv.tv_nsec) / 1000000.0);
    OBMC_WARN("SSIF transaction time: %.1f ms", elapsed);
#endif
  } /* while (g_running) */
}

/*
 * Open the SSIF device (upstream kernel style: /dev/ipmi-ssif-host)
 */
static int ssif_open(const char *path) {
  int fd;

  fd = open(path, O_RDWR);
  if (fd < 0) {
    OBMC_ERROR(errno, "failed to open ssif device %s", path);
  } else {
    OBMC_INFO("opened ssif device %s, fd=%d", path, fd);
  }
  return fd;
}

static void dump_usage(const char *prog) {
  printf("Usage: %s [options] [device_path]\n", prog);
  printf("  -h|--help      print this help message\n");
  printf("  -v|--verbose   enable verbose logging\n");
  printf("  device_path    SSIF device (default: %s)\n", DEFAULT_SSIF_DEV);
}

int main(int argc, char *const argv[]) {
  int ret;
  int opt_index;
  const char *dev_path;

  struct option long_opts[] = {
      {"help", no_argument, NULL, 'h'},
      {"verbose", no_argument, NULL, 'v'},
      {NULL, 0, NULL, 0},
  };

  dev_path = DEFAULT_SSIF_DEV;

  while (1) {
    opt_index = 0;
    ret = getopt_long(argc, argv, "hv", long_opts, &opt_index);
    if (ret == -1)
      break;

    switch (ret) {
      case 'h':
        dump_usage(argv[0]);
        return 0;
      case 'v':
        verbose_logging = 1;
        break;
      default:
        return -1;
    }
  }

  /* Optional positional argument: device path */
  if (optind < argc) {
    dev_path = argv[optind];
  }

  obmc_log_init("ssifd", LOG_INFO, 0);
  obmc_log_set_syslog(LOG_CONS, LOG_DAEMON);
  obmc_log_unset_std_stream();

  setup_signals();
  init_timer_cond();

  OBMC_INFO("ssifd started: device=%s", dev_path);

  ssif_fd = ssif_open(dev_path);
  if (ssif_fd < 0) {
    return -1;
  }

  /* Run the main processing loop in foreground (managed by systemd) */
  ssif_loop();

  close(ssif_fd);
  OBMC_INFO("ssifd terminated");
  return 0;
}
