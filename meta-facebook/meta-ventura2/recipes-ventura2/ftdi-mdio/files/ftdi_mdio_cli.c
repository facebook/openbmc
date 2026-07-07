 /*
 * ftdi_mdio_cli.c
 *
 * FTDI MDIO/GPIO command-line tool.
 *
 * Supports two modes:
 *   daemon mode     - forwards commands to ftdi-mdio-daemon via Linux socket
 *                     (-d/-i not required)
 *   standalone mode - opens USB directly, no daemon needed
 *                     (-d/-i required)
 *
 * Mode is selected automatically: daemon mode if /run/ftdi_mdio.sock is
 * reachable, standalone mode otherwise.
 */

#include "ftdi_mdio_core.h"
#include "ftdi_mdio_daemon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

/* ────────────────────────────────────────────────────────── */
/*  CLI action enum */
/* ────────────────────────────────────────────────────────── */
typedef enum {
  FTDI_GPIO_GET=1,
  FTDI_GPIO_SET,
  FTDI_MDIO_READ,
  FTDI_MDIO_WRITE,
  FTDI_MDIO_READ_C45,
  FTDI_MDIO_WRITE_C45,
  FTDI_MDIO_LIST,
  FTDI_MDIO_LISTALL,
  FTDI_MDIO_EEPROM,
  END_OF_FUNCLIST
} FTDI_MDIO_ACTION;

struct ftdi_data {
  FTDI_MDIO_ACTION action;
  unsigned int args_set;
  unsigned int device;
  unsigned int interface;
  unsigned int phyid;
  unsigned int devad;
  unsigned int regaddr;
  unsigned int value;
  unsigned int listall_flag;
  unsigned int gpio_num;
  unsigned int gpio_dir;
  unsigned int clk_div;
};

#define ARG_DEVICE (1u << 0)
#define ARG_IFACE (1u << 1)
#define ARG_PHYID (1u << 2)
#define ARG_REGADDR (1u << 3)
#define ARG_VALUE (1u << 4)
#define ARG_DEVAD (1u << 5)
#define ARG_GPIO_DIR (1u << 6)
#define ARG_GPIO_NUM (1u << 7)
#define ARG_GPIO_LVL (1u << 8)

#define DEVICEID_MAX 0xFF
#define REGADDR_MAX 0xFFFF
#define WRITE_VALUE_MAX 0xFFFF

/* ────────────────────────────────────────────────────────── */
/*  IPC helper: send a request to the daemon, return response  */
/* ────────────────────────────────────────────────────────── */
static int ipc_call(mdio_request_t* req, mdio_response_t* rsp) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return -1;
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, FTDI_MDIO_SOCK_PATH, sizeof(addr.sun_path) - 1);

  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    fprintf(
        stderr,
        "Error: cannot connect to daemon (%s)\n"
        "Please start it first: ftdi_mdio_daemon -d <devaddr> -i <interface>\n",
        FTDI_MDIO_SOCK_PATH);
    close(fd);
    return -1;
  }

  send(fd, req, sizeof(*req), 0);
  ssize_t n = recv(fd, rsp, sizeof(*rsp), MSG_WAITALL);
  close(fd);

  if (n != (ssize_t)sizeof(*rsp)) {
    fprintf(stderr, "Error: daemon response truncated\n");
    return -1;
  }
  return rsp->status;
}

static int daemon_available(void) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0)
    return 0;
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, FTDI_MDIO_SOCK_PATH, sizeof(addr.sun_path) - 1);
  int ok = (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0);
  close(fd);
  return ok;
}

static void showusage(int cmd) {
  FILE* out = stderr;
  switch (cmd) {
    case FTDI_GPIO_GET:
      fprintf(out, "Usage:\n");
      fprintf(out, "  [daemon]     ftdi-mdio gpioget -n <0-15>\n");
      fprintf(out, 
          "  [standalone] ftdi-mdio gpioget -d <0-255> -i <1-4> -n <0-15>\n");
      break;

    case FTDI_GPIO_SET:
      fprintf(out, "Usage:\n");
      fprintf(out, "  [daemon]     ftdi-mdio gpioset -n <0-15> -w <0|1> -l <0|1>\n");
      fprintf(out, 
          "  [standalone] ftdi-mdio gpioset -d <0-255> -i <1-4> -n <0-15> -w <0|1> -l <0|1>\n");
      break;

    case FTDI_MDIO_READ:
      fprintf(out, "Usage:\n");
      fprintf(out, "  [daemon]     ftdi-mdio read -p <0-31> -r <0x0000-0xFFFF>\n");
      fprintf(out, 
          "  [standalone] ftdi-mdio read -d <0-255> -i <1-4> -p <0-31> -r <0x0000-0xFFFF>\n");
      fprintf(out, 
          "  Options: --clkdiv <0x0000-0xFFFF>  (TCK = 30MHz / (1 + clkdiv))\n");
      break;

    case FTDI_MDIO_WRITE:
      fprintf(out, "Usage:\n");
      fprintf(out, 
          "  [daemon]     ftdi-mdio write -p <0-31> -r <0x0000-0xFFFF> -v <0x0000-0xFFFF>\n");
      fprintf(out, 
          "  [standalone] ftdi-mdio write -d <0-255> -i <1-4> -p <0-31> -r <0x0000-0xFFFF> -v <0x0000-0xFFFF>\n");
      break;

    case FTDI_MDIO_READ_C45:
      fprintf(out, "Usage:\n");
      fprintf(out, 
          "  [daemon]     ftdi-mdio read_c45 -p <0-31> -da <0-31> -r <0x0000-0xFFFF>\n");
      fprintf(out, 
          "  [standalone] ftdi-mdio read_c45 -d <0-255> -i <1-4> -p <0-31> -da <0-31> -r <0x0000-0xFFFF>\n");
      break;

    case FTDI_MDIO_WRITE_C45:
      fprintf(out, "Usage:\n");
      fprintf(out, 
          "  [daemon]     ftdi-mdio write_c45 -p <0-31> -da <0-31> -r <0x0000-0xFFFF> -v <0x0000-0xFFFF>\n");
      fprintf(out, 
          "  [standalone] ftdi-mdio write_c45 -d <0-255> -i <1-4> -p <0-31> -da <0-31> -r <0x0000-0xFFFF> -v <0x0000-0xFFFF>\n");
      break;

    case FTDI_MDIO_EEPROM:
      fprintf(out, "Usage:\n");
      fprintf(out, "  ftdi-mdio eeprom -d <0-255> -i <1-4>\n");
      break;

    default:
      fprintf(out, "\nUsage: ftdi-mdio COMMAND [options]\n\n");
      fprintf(out, 
          "Commands (standalone: -d/-i required; daemon: -d/-i not needed):\n");
      fprintf(out, "  read        - MDIO C22 read\n");
      fprintf(out, "  write       - MDIO C22 write\n");
      fprintf(out, "  read_c45    - MDIO C45 read\n");
      fprintf(out, "  write_c45   - MDIO C45 write\n");
      fprintf(out, "  gpioget     - Get GPIO value\n");
      fprintf(out, "  gpioset     - Set GPIO value\n\n");
      fprintf(out, "USB commands (always direct, no daemon):\n");
      fprintf(out, "  list        - List FTDI devices\n");
      fprintf(out, "  listall     - List FTDI devices (detail)\n");
      fprintf(out, "  eeprom      - Show EEPROM info\n\n");
      fprintf(out, "Options:\n");
      fprintf(out, 
          "  -d <0-255>  - USB device address (required in standalone mode)\n");
      fprintf(out, 
          "  -i <1-4>    - FTDI interface    (required in standalone mode)\n");
      fprintf(out, "  --clkdiv    - Clock divider (MDIO commands only)\n");
      fprintf(out, "  --debug     - Enable debug output\n");
      fprintf(out, "  --quiet     - Suppress non-error messages\n\n");
      fprintf(out, "Daemon:\n");
      fprintf(out, "  ftdi-mdio-daemon -d <devaddr> -i <interface> [--debug]\n");
      break;
  }
}

static inline void check_argc(int argc, int min, int action) {
  if (argc < min) {
    showusage(action);
    exit(0);
  }
}

static int
process_arguments(int argc, char** argv, struct ftdi_data* ctrl_data) {
  int i = 1;
  long devid, phyid, devad, regaddr, value;

  while (i < argc) {
    if (strcmp(argv[i], "gpioget") == 0)
      ctrl_data->action = FTDI_GPIO_GET;
    else if (strcmp(argv[i], "gpioset") == 0)
      ctrl_data->action = FTDI_GPIO_SET;
    else if (strcmp(argv[i], "read") == 0)
      ctrl_data->action = FTDI_MDIO_READ;
    else if (strcmp(argv[i], "write") == 0)
      ctrl_data->action = FTDI_MDIO_WRITE;
    else if (strcmp(argv[i], "read_c45") == 0)
      ctrl_data->action = FTDI_MDIO_READ_C45;
    else if (strcmp(argv[i], "write_c45") == 0)
      ctrl_data->action = FTDI_MDIO_WRITE_C45;
    else if (strcmp(argv[i], "list") == 0)
      ctrl_data->action = FTDI_MDIO_LIST;
    else if (strcmp(argv[i], "listall") == 0) {
      ctrl_data->action = FTDI_MDIO_LIST;
      ctrl_data->listall_flag = 1;
    } else if (strcmp(argv[i], "eeprom") == 0)
      ctrl_data->action = FTDI_MDIO_EEPROM;
    else if (strcmp(argv[i], "-d") == 0) {
      devid = strtol(argv[++i], NULL, 0);
      if (devid > DEVICEID_MAX || devid < 0) {
        log_print(LOG_ERROR, "Error: device address out of range\n");
        return -1;
      }
      ctrl_data->device = (unsigned int)devid;
      ctrl_data->args_set |= ARG_DEVICE;
    } else if (strcmp(argv[i], "-i") == 0) {
      ctrl_data->interface = (unsigned int)strtol(argv[++i], NULL, 0);
      if (ctrl_data->interface > 4 || ctrl_data->interface < 1) {
        log_print(LOG_ERROR, "Error: interface out of range\n");
        return -1;
      }
      ctrl_data->args_set |= ARG_IFACE;
    } else if (strcmp(argv[i], "-p") == 0) {
      phyid = strtol(argv[++i], NULL, 0);
      if (phyid < 0 || phyid > 31) {
        log_print(LOG_ERROR, "Wrong PHY id (0-31)\n");
        return -1;
      }
      ctrl_data->phyid = (unsigned int)phyid;
      ctrl_data->args_set |= ARG_PHYID;
    } else if (strcmp(argv[i], "-da") == 0) {
      devad = strtol(argv[++i], NULL, 0);
      if (devad < 0 || devad > 31) {
        log_print(LOG_ERROR, "Wrong DEVAD (0-31)\n");
        return -1;
      }
      ctrl_data->devad = (unsigned int)devad;
      ctrl_data->args_set |= ARG_DEVAD;
    } else if (strcmp(argv[i], "-r") == 0) {
      regaddr = strtol(argv[++i], NULL, 0);
      if (regaddr > REGADDR_MAX || regaddr < 0) {
        log_print(LOG_ERROR, "Error: regaddr out of range\n");
        return -1;
      }
      ctrl_data->regaddr = (unsigned int)regaddr;
      ctrl_data->args_set |= ARG_REGADDR;
    } else if (strcmp(argv[i], "-v") == 0) {
      value = strtol(argv[++i], NULL, 0);
      if (value > WRITE_VALUE_MAX || value < 0) {
        log_print(LOG_ERROR, "Error: value out of range\n");
        return -1;
      }
      ctrl_data->value = (unsigned int)value;
      ctrl_data->args_set |= ARG_VALUE;
    } else if (strcmp(argv[i], "-w") == 0) {
      ctrl_data->gpio_dir = (unsigned int)strtol(argv[++i], NULL, 10);
      ctrl_data->args_set |= ARG_GPIO_DIR;
    } else if (strcmp(argv[i], "-n") == 0) {
      ctrl_data->regaddr = (unsigned int)strtol(argv[++i], NULL, 10);
      ctrl_data->args_set |= ARG_GPIO_NUM;
    } else if (strcmp(argv[i], "-l") == 0) {
      ctrl_data->value = (unsigned int)strtol(argv[++i], NULL, 10);
      ctrl_data->args_set |= ARG_GPIO_LVL;
    } else if (strcmp(argv[i], "--clkdiv") == 0) {
      ctrl_data->clk_div = (unsigned int)strtol(argv[++i], NULL, 10);
    } else if (strcmp(argv[i], "--debug") == 0)
      g_log_level = LOG_DEBUG;
    else if (strcmp(argv[i], "--quiet") == 0)
      g_log_level = LOG_ERROR;
    else {
      return -1;
    }

    i++;
  }
  return 0;
}

#define REQUIRE_ARGS(cond, msg, action) \
  do {                                  \
    if (!(cond)) {                      \
      fprintf(stderr, "%s\n", msg);              \
      showusage(action);                \
      return -1;                        \
    }                                   \
  } while (0)

static int validate_args(const struct ftdi_data* d, int use_daemon) {
  REQUIRE_ARGS((d->action != 0), "Error: no command specified", 0);

  if (!use_daemon) {
    switch (d->action) {
      case FTDI_MDIO_READ:
      case FTDI_MDIO_WRITE:
      case FTDI_MDIO_READ_C45:
      case FTDI_MDIO_WRITE_C45:
      case FTDI_GPIO_GET:
      case FTDI_GPIO_SET:
      case FTDI_MDIO_EEPROM:
        REQUIRE_ARGS(
            (d->args_set & ARG_DEVICE) && (d->args_set & ARG_IFACE),
            "standalone: -d and -i are required",
            d->action);
        break;
      default:
        break;
    }
  }

  switch (d->action) {
    case FTDI_MDIO_READ:
      REQUIRE_ARGS(
          (d->args_set & ARG_PHYID) && (d->args_set & ARG_REGADDR),
          "read: -p and -r are required",
          d->action);
      break;
    case FTDI_MDIO_WRITE:
      REQUIRE_ARGS(
          (d->args_set & ARG_PHYID) && (d->args_set & ARG_REGADDR) &&
              (d->args_set & ARG_VALUE),
          "write: -p, -r and -v are required",
          d->action);
      break;
    case FTDI_MDIO_READ_C45:
      REQUIRE_ARGS(
          (d->args_set & ARG_PHYID) && (d->args_set & ARG_DEVAD) &&
              (d->args_set & ARG_REGADDR),
          "read_c45: -p, -da and -r are required",
          d->action);
      break;
    case FTDI_MDIO_WRITE_C45:
      REQUIRE_ARGS(
          (d->args_set & ARG_PHYID) && (d->args_set & ARG_DEVAD) &&
              (d->args_set & ARG_REGADDR) && (d->args_set & ARG_VALUE),
          "write_c45: -p, -da, -r and -v are required",
          d->action);
      break;
    default:
      break;
  }
  return 0;
}

int main(int argc, char* argv[]) {
  struct ftdi_data ctrl_data;
  memset(&ctrl_data, 0, sizeof(ctrl_data));
  ctrl_data.clk_div = 0xFFFFFFFF;

  if (argc < 2) {
    showusage(0);
    return 2;
  }

  if (process_arguments(argc, argv, &ctrl_data) < 0) {
    showusage(0);
    return 2;
  }

  int use_daemon = daemon_available();
  log_print(
      LOG_DEBUG,
      use_daemon ? "[main] daemon found — using IPC mode\n"
                 : "[main] daemon not found — using standalone USB mode\n");

  if (validate_args(&ctrl_data, use_daemon) < 0)
    return 2;

  mdio_request_t req;
  mdio_response_t rsp;
  uint16_t mdio_data;

  /* ── list / eeprom: always direct USB, no daemon ── */
  switch (ctrl_data.action) {
    case FTDI_MDIO_LIST:
      device_search(ctrl_data.listall_flag);
      return 0;

    case FTDI_MDIO_EEPROM:
      if (device_open(ctrl_data.device, ctrl_data.interface) < 0) {
        log_print(LOG_ERROR, "device_open failed\n");
        return 1;
      }
      ftdi_read_eeprom(&ftdic);
      ftdi_eeprom_decode(&ftdic, 1);
      device_close();
      return 0;

    default:
      break;
  }

  if (use_daemon) {
    switch (ctrl_data.action) {
      case FTDI_MDIO_READ:
        memset(&req, 0, sizeof(req));
        req.action = IPC_MDIO_READ;
        req.phyid = (uint8_t)ctrl_data.phyid;
        req.regaddr = (uint16_t)ctrl_data.regaddr;
        if (ipc_call(&req, &rsp) < 0)
          return 1;
        printf("0x%04X\n", rsp.value);
        break;

      case FTDI_MDIO_WRITE:
        memset(&req, 0, sizeof(req));
        req.action = IPC_MDIO_WRITE;
        req.phyid = (uint8_t)ctrl_data.phyid;
        req.regaddr = (uint16_t)ctrl_data.regaddr;
        req.value = (uint16_t)ctrl_data.value;
        if (ipc_call(&req, &rsp) < 0)
          return 1;
        break;

      case FTDI_MDIO_READ_C45:
        memset(&req, 0, sizeof(req));
        req.action = IPC_MDIO_READ_C45;
        req.phyid = (uint8_t)ctrl_data.phyid;
        req.devad = (uint8_t)ctrl_data.devad;
        req.regaddr = (uint16_t)ctrl_data.regaddr;
        if (ipc_call(&req, &rsp) < 0)
          return 1;
        printf("0x%04X\n", rsp.value);
        break;

      case FTDI_MDIO_WRITE_C45:
        memset(&req, 0, sizeof(req));
        req.action = IPC_MDIO_WRITE_C45;
        req.phyid = (uint8_t)ctrl_data.phyid;
        req.devad = (uint8_t)ctrl_data.devad;
        req.regaddr = (uint16_t)ctrl_data.regaddr;
        req.value = (uint16_t)ctrl_data.value;
        if (ipc_call(&req, &rsp) < 0)
          return 1;
        break;

      case FTDI_GPIO_GET:
        memset(&req, 0, sizeof(req));
        req.action = IPC_GPIO_GET;
        req.regaddr = (uint16_t)ctrl_data.regaddr;
        if (ipc_call(&req, &rsp) < 0)
          return 1;
        printf("GPIO=0x%04X\n", rsp.value);
        break;

      case FTDI_GPIO_SET:
        memset(&req, 0, sizeof(req));
        req.action = IPC_GPIO_SET;
        req.regaddr = (uint16_t)ctrl_data.regaddr;
        req.gpio_dir = (uint8_t)ctrl_data.gpio_dir;
        req.value = (uint16_t)ctrl_data.value;
        if (ipc_call(&req, &rsp) < 0)
          return 1;
        printf("GPIO=0x%04X\n", rsp.value);
        break;

      default:
        break;
    }

  } else {
    if (device_open(ctrl_data.device, ctrl_data.interface) < 0) {
      log_print(LOG_ERROR, "Error: device_open failed!\n");
      return 1;
    }
    if (ctrl_data.clk_div != 0xFFFFFFFF)
      clk_divider_set((uint16_t)ctrl_data.clk_div);

    switch (ctrl_data.action) {
      case FTDI_MDIO_READ:
        if (mdio_read_c22(ctrl_data.phyid, ctrl_data.regaddr, &mdio_data) < 0) {
          device_close();
          return 1;
        }
        printf("0x%04X\n", mdio_data);
        break;

      case FTDI_MDIO_WRITE:
        mdio_write_c22(
            ctrl_data.phyid, ctrl_data.regaddr, (uint16_t)ctrl_data.value);
        break;

      case FTDI_MDIO_READ_C45:
        if (mdio_read_c45(
                ctrl_data.phyid,
                ctrl_data.devad,
                ctrl_data.regaddr,
                &mdio_data) < 0) {
          device_close();
          return 1;
        }
        printf("0x%04X\n", mdio_data);
        break;

      case FTDI_MDIO_WRITE_C45:
        mdio_write_c45(
            ctrl_data.phyid,
            ctrl_data.devad,
            ctrl_data.regaddr,
            (uint16_t)ctrl_data.value);
        break;

      case FTDI_GPIO_GET:
        gpio_get();
        break;

      case FTDI_GPIO_SET:
        gpio_get();
        gpio_set(ctrl_data.regaddr, ctrl_data.gpio_dir, ctrl_data.value);
        gpio_get();
        break;

      default:
        device_close();
        showusage(0);
        return 1;
    }

    device_close();
  }

  return 0;
}
