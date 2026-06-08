#pragma once
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>

#define FTDI_MDIO_SOCK_PATH "/run/ftdi_mdio.sock"

typedef enum {
  IPC_MDIO_READ = 1,
  IPC_MDIO_WRITE = 2,
  IPC_MDIO_READ_C45 = 3,
  IPC_MDIO_WRITE_C45 = 4,
  IPC_GPIO_GET = 5,
  IPC_GPIO_SET = 6,
} ipc_action_t;

typedef struct {
  uint8_t action; /* ipc_action_t */
  uint8_t phyid;
  uint8_t devad;
  uint16_t regaddr;
  uint16_t value;
  /* gpio only */
  uint8_t gpio_dir;
} __attribute__((packed)) mdio_request_t;

typedef struct {
  int32_t status; /* 0 = ok, negative = error */
  uint16_t value; /* read result */
} __attribute__((packed)) mdio_response_t;
