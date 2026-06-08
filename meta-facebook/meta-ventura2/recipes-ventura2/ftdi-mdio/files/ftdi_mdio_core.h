#pragma once
#include <libftdi1/ftdi.h>
#include <stdbool.h>
#include <stdint.h>

/* ── log ── */
typedef enum { LOG_ERROR = 0, LOG_WARN, LOG_INFO, LOG_DEBUG } LOG_LEVEL;
extern LOG_LEVEL g_log_level;
void log_print(LOG_LEVEL level, const char* fmt, ...);

/* ── global ftdi state (owned by core) ── */
extern struct ftdi_context ftdic;
extern struct ftdi_device_list* ftdi_list;

typedef struct {
  uint16_t dir;
  uint16_t level;
} ftdi_gpio_s;
extern ftdi_gpio_s gpio_s;

/* ── device lifecycle ── */
int device_open(int device, int intf_id);
int device_close(void);
int device_search(int flag);

/* ── clock ── */
int clk_divider_set(uint16_t clk_div);

/* ── MDIO ── */
int mdio_read_c22(uint8_t phyid, uint8_t phyreg, uint16_t* data);
int mdio_write_c22(uint8_t phyid, uint8_t phyreg, uint16_t data);
int mdio_read_c45(
    uint8_t phyid,
    uint8_t devad,
    uint16_t phyreg,
    uint16_t* data);
int mdio_write_c45(
    uint8_t phyid,
    uint8_t devad,
    uint16_t phyreg,
    uint16_t data);

/* ── GPIO ── */
int gpio_get();
int gpio_set(uint8_t gpio_num, uint8_t dir, uint8_t value);
