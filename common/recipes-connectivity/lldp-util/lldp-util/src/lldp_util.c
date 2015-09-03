/*
 * Copyright 2004-present Facebook. All Rights Reserved.
 *
 * A simple utility for printing LLDP and CDP packets received on an interface.
 * Eventually it might be worth replacing this with lldpd or lldpad,
 * but for now this provides a simple standalone script that can be dropped
 * onto a system and run on an as-needed basis.
 */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/filter.h>
#include <linux/if_ether.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// The minimum ethernet frame length, without the 4-byte frame check sequence
#define MIN_FRAME_LENGTH 60
#define MAX_FRAME_LENGTH 9000

#define ETHERTYPE_VLAN 0x8100
#define ETHERTYPE_LLDP 0x88CC

#define LLDP_CHASSIS_CDP 0xff
#define LLDP_CHASSIS_MAC_ADDRESS 4
#define LLDP_CHASSIS_NET_ADDRESS 5

#define LLDP_PORT_CDP 0xff
#define LLDP_PORT_MAC_ADDRESS 3
#define LLDP_PORT_NET_ADDRESS 4

#define LLDP_TYPE_END 0
#define LLDP_TYPE_CHASSIS_ID 1
#define LLDP_TYPE_PORT_ID 2
#define LLDP_TYPE_SYSTEM_NAME 5

#define CDP_TYPE_DEVICE_ID 1
#define CDP_TYPE_PORT_ID 3
#define CDP_TYPE_SYSTEM_NAME 20

static const unsigned char MAC_LLDP_NEAREST_BRIDGE[] =
  { 0x01, 0x80, 0xc2, 0x00, 0x00,0x0e };
static const unsigned char MAC_CDP[] =
  { 0x01, 0x00, 0x0c, 0xcc, 0xcc, 0xcc };

static bool verbose = false;

void errmsg(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void debugmsg(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

void errmsg(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  va_end(ap);
}

void debugmsg(const char* fmt, ...) {
  if (!verbose) {
    return;
  }

  va_list ap;
  va_start(ap, fmt);
  fputs("DEBUG: ", stderr);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  va_end(ap);
}

#define FAIL_ERRNO(msg, ...) \
  do { \
    returnCode = -errno; \
    errmsg((msg), ##__VA_ARGS__); \
    goto error; \
  } while (0)

#define CHECK_ERROR(rc, msg, ...) \
  do { if ((rc) == -1) { FAIL_ERRNO((msg), ##__VA_ARGS__); } } while (0)

typedef struct {
  const char* protocol;
  const char* local_interface;
  const uint8_t* src_mac;
  const uint8_t* chassis_id;
  const uint8_t* port_id;
  const uint8_t* system_name;
  uint32_t chassis_id_length;
  uint32_t port_id_length;
  uint32_t system_name_length;
  uint8_t chassis_id_type;
  uint8_t port_id_type;
} lldp_neighbor_t;


#define DFLT_MAX_SAV_CDP  10

typedef struct {
  uint8_t *cdp_buf;
  size_t  length;
} cdp_frame_t;

typedef struct {
  int max_sav_cdp;     // max # cdp frames to save
  int num_sav_cdp;     // number currently saved
  cdp_frame_t* frame;  // saved CDP frames
} cdp_sav_data_t;

cdp_sav_data_t cdp_sav_data;

volatile sig_atomic_t g_interrupt;   // set in exit handler

void lldp_neighbor_init(lldp_neighbor_t* neighbor,
                        const char* protocol,
                        const char* interface,
                        const uint8_t* src_mac) {
  memset(neighbor, 0, sizeof(*neighbor));
  neighbor->protocol = protocol;
  neighbor->local_interface = interface;
  neighbor->src_mac = src_mac;
}

int lldp_open(const char* interface) {
  int fd = -1;
  int returnCode = -EINVAL;

  {
    fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0) {
      FAIL_ERRNO("failed to create socket for local interface %s", interface);
    }

    // Look up the interface index
    struct ifreq ifr;
    snprintf(ifr.ifr_name, IFNAMSIZ, interface);
    int rc = ioctl(fd, SIOCGIFINDEX, &ifr);
    CHECK_ERROR(rc, "failed to get interface index for %s", interface);

    // Bind the socket
    struct sockaddr_ll addr;
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = ifr.ifr_ifindex;
    addr.sll_protocol = htons(ETH_P_ALL);
    rc = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    CHECK_ERROR(rc, "failed to bind socket for %s", interface);

    // Ask linux to also send us packets sent to LLDP "nearest bridge"
    // multicast MAC address
    struct packet_mreq mr;
    memset(&mr, 0, sizeof(mr));
    mr.mr_ifindex = ifr.ifr_ifindex;
    mr.mr_alen = ETH_ALEN;
    memcpy(mr.mr_address, MAC_LLDP_NEAREST_BRIDGE, ETH_ALEN);
    mr.mr_type = PACKET_MR_MULTICAST;
    rc = setsockopt(fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr));
    CHECK_ERROR(rc, "failed to add LLDP packet membership for %s", interface);

    // Ask linux to also send us packets sent to the CDP multicast MAC address
    memcpy(mr.mr_address, MAC_CDP, ETH_ALEN);
    rc = setsockopt(fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr));
    CHECK_ERROR(rc, "failed to add CDP packet membership for %s", interface);

    // Set a filter to ignore non-multicast packets, so we don't get a flood
    // of meaningless packets that we don't care about.
    //
    // tcpdump -i eth0 -dd ether multicast
    static struct sock_filter PACKET_FILTER[] = {
      { 0x30, 0, 0, 0x00000000 },
      { 0x45, 0, 1, 0x00000001 },
      { 0x6, 0, 0, 0x0000ffff },
      { 0x6, 0, 0, 0x00000000 },
    };
    struct sock_fprog bpf;
    bpf.len = sizeof(PACKET_FILTER) / sizeof(PACKET_FILTER[0]);
    bpf.filter = PACKET_FILTER;
    rc = setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf));
    CHECK_ERROR(rc, "failed to set socket packet filter for %s", interface);

    returnCode = fd;
    goto cleanup;
  }

error:
  assert(returnCode < 0);
  if (fd != -1) {
    close(fd);
  }

cleanup:
  return returnCode;
}

void lldp_print_value(const uint8_t* buf, uint32_t length) {
  int n;

  bool add_quotes = false;
  for (uint32_t n = 0; n < length; ++n) {
    uint8_t value = buf[n];
    if (value < 0x20 || value >= 0x7f || isspace(value)) {
      add_quotes = true;
      break;
    }
  }

  if (add_quotes) {
    putchar('"');
  }
  for (uint32_t n = 0; n < length; ++n) {
    uint8_t value = buf[n];

    // Ignore trailing NUL characters
    if (value == 0 && n + 1 == length) {
      break;
    }

    if (value == '"' || value == '\\') {
      putchar('\\');
      putchar(value);
    } else if (value >= 0x20 && value < 0x7f) {
      putchar(value);
    } else {
      printf("\\x%02x", value);
    }
  }
  if (add_quotes) {
    putchar('"');
  }
}

void lldp_print_mac(const uint8_t* buf, uint32_t length) {
  if (length != 6) {
    fputs("<bad_mac:", stdout);
    lldp_print_value(buf, length);
    putchar('>');
    return;
  }

  printf("%02x:%02x:%02x:%02x:%02x:%02x",
         buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
}

void lldp_print_net_addr(const uint8_t* buf, uint32_t length) {
  if (length < 1) {
    return;
  }

  if (buf[0] == 1 && length == 5) {
    // IPv4
    struct in_addr addr;
    memcpy(&addr.s_addr, buf + 1, length - 1);

    char str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr, str, sizeof(str)) != NULL) {
      fputs(str, stdout);
      return;
    }
  } else if (buf[0] == 2 && length == 17) {
    struct in6_addr addr;
    memcpy(&addr.s6_addr, buf + 1, length - 1);

    char str[INET6_ADDRSTRLEN];
    if (inet_ntop(AF_INET6, &addr, str, sizeof(str)) != NULL) {
      fputs(str, stdout);
      return;
    }
  }

  // If we are still here, this wasn't IPv4 or IPv6, or we failed to parse it.
  printf("<net_addr:%d:", buf[0]);
  lldp_print_value(buf + 1, length - 1);
  putchar('>');
}

void lldp_print_neighbor(lldp_neighbor_t* neighbor) {
  fprintf(stdout, "%s: local_port=%s",
          neighbor->protocol, neighbor->local_interface);
  fputs(" remote_system=", stdout);
  if (neighbor->system_name) {
    lldp_print_value(neighbor->system_name, neighbor->system_name_length);
  } else if (neighbor->chassis_id_type == LLDP_CHASSIS_MAC_ADDRESS) {
    lldp_print_mac(neighbor->chassis_id, neighbor->chassis_id_length);
  } else if (neighbor->chassis_id_type == LLDP_CHASSIS_NET_ADDRESS) {
    lldp_print_net_addr(neighbor->chassis_id, neighbor->chassis_id_length);
  } else {
    lldp_print_value(neighbor->chassis_id, neighbor->chassis_id_length);
  }

  fputs(" remote_port=", stdout);
  if (neighbor->port_id_type == LLDP_PORT_MAC_ADDRESS) {
    lldp_print_mac(neighbor->port_id, neighbor->port_id_length);
  } else if (neighbor->port_id_type == LLDP_PORT_NET_ADDRESS) {
    lldp_print_net_addr(neighbor->port_id, neighbor->port_id_length);
  } else {
    lldp_print_value(neighbor->port_id, neighbor->port_id_length);
  }
  putchar('\n');
}

uint16_t lldp_read_u16(const uint8_t** buf) {
  // Use memcpy() to comply with strict-aliasing rules.
  // The compiler will optimize this away anyway.
  uint16_t result;
  memcpy(&result, *buf, 2);
  (*buf) += 2;
  return htons(result);
}

int lldp_parse_lldp(lldp_neighbor_t* neighbor,
                    const uint8_t* buf,
                    const uint8_t* end) {
  while (buf < end) {
    // Parse the type (7 bits) and length (9 bits)
    if ((end - buf) < 2) {
      errmsg("LLDPDU truncated in tag/length header");
      return -EINVAL;
    }
    uint16_t length = lldp_read_u16(&buf);
    uint16_t type = (length >> 9);
    length &= 0x01ff;

    if ((end - buf) < length) {
      errmsg("LLDPDU truncated inside field of type %d: "
             "field_length=%d, remaining=%d", type, length, end - buf);
      return -EINVAL;
    }

    if (type == LLDP_TYPE_CHASSIS_ID) {
      if (length < 1) {
        errmsg("LLDPDU with invalid chassis ID length %d", length);
        return -EINVAL;
      }
      neighbor->chassis_id_type = buf[0];
      neighbor->chassis_id = buf + 1;
      neighbor->chassis_id_length = length - 1;
    } else if (type == LLDP_TYPE_PORT_ID) {
      if (length < 1) {
        errmsg("LLDPDU with invalid port ID length %d", length);
        return -EINVAL;
      }
      neighbor->port_id_type = buf[0];
      neighbor->port_id = buf + 1;
      neighbor->port_id_length = length - 1;
    } else if (type == LLDP_TYPE_SYSTEM_NAME) {
      neighbor->system_name = buf;
      neighbor->system_name_length = length;
    }

    buf += length;

    if (type == LLDP_TYPE_END) {
      break;
    }
  }

  if (!neighbor->chassis_id) {
    errmsg("received LLDPDU with missing chassis ID");
    return -EINVAL;
  }
  if (!neighbor->port_id) {
    errmsg("received LLDPDU with missing port ID");
    return -EINVAL;
  }

  return 0;
}

int lldp_parse_cdp(lldp_neighbor_t* neighbor,
                   const uint8_t* buf,
                   const uint8_t* end) {
  // Skip over some initial headers:
  // 1 byte DSAP
  // 1 byte SSAP
  // 1 byte LLC control field
  // 3 byte SNAP vendor code
  // 2 byte SNAP local code
  // 1 byte CDP version
  // 1 byte TTL
  // 2 byte checksum
  if ((end - buf) < 12) {
    errmsg("CDP PDU truncated in initial headers: length=%d", end - buf);
    return -EINVAL;
  }
  buf += 8;
  uint8_t cdp_version = *buf;
  if (cdp_version != 2) {
    // We only support CDP version 2 for now.
    debugmsg("ignoring unsupported CDP PDU with version %d", cdp_version);
    return -EPFNOSUPPORT;
  }
  buf += 4;

  while (buf < end) {
    if ((end - buf) < 4) {
      errmsg("CDP PDU truncated in tag/length header");
      return -EINVAL;
    }
    uint16_t type = lldp_read_u16(&buf);
    uint16_t length = lldp_read_u16(&buf);

    if (length < 4) {
      errmsg("CDP PDU with bad length=%d for field of type %d",
             length, type);
      return -EINVAL;
    }
    length -= 4;
    if ((end - buf) < length) {
      errmsg("CDP PDU truncated inside field of type %d: "
             "field_length=%d, remaining=%d", type, length, end - buf);
      return -EINVAL;
    }

    if (type == CDP_TYPE_DEVICE_ID) {
      neighbor->chassis_id_type = LLDP_CHASSIS_CDP;
      neighbor->chassis_id = buf;
      neighbor->chassis_id_length = length;
    } else if (type == CDP_TYPE_PORT_ID) {
      neighbor->port_id_type = LLDP_PORT_CDP;
      neighbor->port_id = buf;
      neighbor->port_id_length = length;
    } else if (type == CDP_TYPE_SYSTEM_NAME) {
      neighbor->system_name = buf;
      neighbor->system_name_length = length;
    }
    buf += length;
  }

  if (!neighbor->chassis_id) {
    errmsg("received CDP PDU with missing device ID");
    return -EINVAL;
  }
  if (!neighbor->port_id) {
    errmsg("received CDP PDU with missing port ID");
    return -EINVAL;
  }

  return 0;
}

int lldp_process_packet(const uint8_t* buf,
                        size_t length,
                        const char* interface,
                        bool process_cdp,
                        bool* processed) {
  int returnCode = -EINVAL;
  *processed = false;

  {
    // Bail if the minimum ethernet frame size is not met
    if (length < MIN_FRAME_LENGTH) {
      errmsg("received too-short frame on interface %s: length=%d",
             interface, length);
      returnCode = -EINVAL;
      goto error;
    }

    const uint8_t* buf_start = buf;
    const uint8_t* end = buf + length;

    const uint8_t* dest_mac = buf;
    buf += 6;
    const uint8_t* src_mac = buf;
    buf += 6;
    uint16_t ethertype = lldp_read_u16(&buf);

    if (ethertype == ETHERTYPE_VLAN) {
      uint16_t vlan = lldp_read_u16(&buf);
      ethertype = lldp_read_u16(&buf);
    }

    if (ethertype == ETHERTYPE_LLDP) {
      lldp_neighbor_t neighbor;
      lldp_neighbor_init(&neighbor, "LLDP", interface, src_mac);
      returnCode = lldp_parse_lldp(&neighbor, buf, end);
      if (returnCode != 0) {
        goto error;
      }
      lldp_print_neighbor(&neighbor);
      fflush(stdout);
      *processed = true;
    } else if ((memcmp(dest_mac, MAC_CDP, 6) == 0) && (ethertype < 0x600)) {
      if (process_cdp) {
        lldp_neighbor_t neighbor;
        lldp_neighbor_init(&neighbor, "CDP", interface, src_mac);
        returnCode = lldp_parse_cdp(&neighbor, buf, end);
        if (returnCode != 0) {
          goto error;
        }
        lldp_print_neighbor(&neighbor);
        fflush(stdout);
        *processed = true;
      } else if (cdp_sav_data.num_sav_cdp < cdp_sav_data.max_sav_cdp) {
        cdp_sav_data.frame[cdp_sav_data.num_sav_cdp].cdp_buf = malloc(length);
        assert(cdp_sav_data.frame[cdp_sav_data.num_sav_cdp].cdp_buf);
        memcpy(cdp_sav_data.frame[cdp_sav_data.num_sav_cdp].cdp_buf, buf_start,
               length);
        cdp_sav_data.frame[cdp_sav_data.num_sav_cdp].length = length;
        cdp_sav_data.num_sav_cdp++;
      }
    } else {
      // Some other packet type that we don't care about.
      debugmsg("Ignored packet on %s: ethertype=0x%04x",
               interface, ethertype);
    }

    returnCode = 0;
    goto cleanup;
  }

error:
  assert(returnCode < 0);
cleanup:
  return returnCode;
}

int lldp_receive(int fd, const char* interface, bool cdp, bool* processed) {
  int returnCode = -EINVAL;
  *processed = false;
  {
    uint8_t buf[MAX_FRAME_LENGTH];

    struct sockaddr_ll src_addr;
    socklen_t addr_len;
    ssize_t len = recvfrom(fd, buf, MAX_FRAME_LENGTH, 0,
                           (struct sockaddr*)&src_addr, &addr_len);
    if (len == -1) {
      if (errno != EINTR) {
        errmsg("error reading packet from %s", interface);
      }
      returnCode = -errno;
      goto error;
    }

    returnCode = lldp_process_packet(buf, len, interface, cdp, processed);
    goto cleanup;
  }

error:
  assert(returnCode < 0);
cleanup:
  return returnCode;
}

void exit_handler(int signum) {
  g_interrupt = true;
}

void usage_short(FILE* out) {
  fprintf(out, "lldp-util [-h] [-c] [-i <interface>] "
          "[-n <count>] [-t <timeout>]\n");
}

void usage_full(FILE* out) {
  usage_short(out);
  fprintf(out,
          "\n"
          "  -c              Report CDP packets as they arrive in addition "
          "to LLDP\n"
          "  -h              Show this help message and exit\n"
          "  -i <interface>  Specify the interface to listen on\n"
          "  -n <count>      Exit after receiving <count> LLDP packets\n"
          "  -t <timeout>    Exit after <timeout> seconds\n"
          "                  Negative or 0 means no timeout\n"
          "\n"
          "If neither -n nor -t are specified, lldp-util uses a 65 second\n"
          "timeout by default\n"
          );
}

int main(int argc, char* argv[]) {
  const char* interface = "eth0";
  long timeout = 0;
  bool has_timeout = false;
  long max_count = 0;
  bool has_max_count = false;
  bool inline_cdp = false;

  while (true) {
    int opt = getopt(argc, argv, "chi:n:t:");
    if (opt == -1) {
      break;
    }
    switch (opt) {
      case 'c':
        inline_cdp = true;
        break;
      case 'h':
        usage_full(stdout);
        return 0;
      case 'i':
        interface = optarg;
        break;
      case 'n': {
        char* end;
        max_count = strtol(optarg, &end, 10);
        if (end == optarg || *end != '\0') {
          fprintf(stderr, "error: bad count value: must be an integer\n");
          return 1;
        }
        has_max_count = true;
        break;
      }
      case 't': {
        char* end;
        timeout = strtol(optarg, &end, 10);
        if (end == optarg || *end != '\0') {
          fprintf(stderr, "error: bad timeout value: must be an integer\n");
          return 1;
        }
        has_timeout = true;
        break;
      }
      default:
        usage_short(stderr);
        return 1;
    }
  }

  // Default to a 65 second timeout if no other limits were specified.
  if (!has_max_count && !has_timeout) {
    timeout = 65;
  }

  int fd = lldp_open(interface);
  if (fd < 0) {
    return 1;
  }

  struct sigaction sa;

  // Clear all fields in sa.  In particular, SA_RESTART must not be set in
  // sa_flags field because we want recvfrom to be interrupted.
  memset(&sa, 0, sizeof(sa));

  sa.sa_handler = exit_handler;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  // Automatically stop after 65 seconds
  if (timeout > 0) {
    sigaction(SIGALRM, &sa, NULL);
    alarm(timeout);
  }

  // In the case when we are asked not to report CDP packets as they arrive,
  // we will save them in a global buffer.  On exiting, if we have not seen
  // any LLDP packets, we'll dump the CDP packets that arrived.
  if (!inline_cdp) {
    if (max_count > 0) {
      cdp_sav_data.max_sav_cdp = max_count;
    } else {
      cdp_sav_data.max_sav_cdp = DFLT_MAX_SAV_CDP;
    }
    cdp_sav_data.frame = malloc(sizeof(cdp_frame_t) *
                                cdp_sav_data.max_sav_cdp);
    if (cdp_sav_data.frame == NULL) {
      printf("memory allocation failed for saved frames\n");
      exit(1);
    }
  }

  printf("Listening for LLDP packets...\n");
  fflush(stdout);
  long count = 0;
  bool processed;
  while (true) {
    lldp_receive(fd, interface, inline_cdp, &processed);
    if (processed) {
      ++count;
      if (max_count > 0 && count >= max_count) {
        break;
      }
    }
    if (g_interrupt) {
      break;
    }
  }

  // Print saved CDP packets if we haven't printed anything yet
  if (count == 0) {
    for (int i = 0; i < cdp_sav_data.num_sav_cdp; i++) {
      lldp_process_packet(cdp_sav_data.frame[i].cdp_buf,
                          cdp_sav_data.frame[i].length,
                          interface, true, &processed);
    }
  }

  return 0;
}
