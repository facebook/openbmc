#ifndef _USB_DBG_CONF_H_
#define _USB_DBG_CONF_H_
#include <stdint.h>
#include <stdbool.h>

/* Used for systems which do not specifically have a 
 * phase, and we want to ignore the phase provided by the
 * debug card */
#define PHASE_ANY 0xff

typedef struct _post_desc {
  uint8_t code;
  char    desc[32];
} post_desc_t;

typedef struct _post_phase_desc {
  int         phase;
  post_desc_t *post_tbl;
  size_t       post_tbl_cnt;
} post_phase_desc_t;

typedef struct _gpio_desc {
  uint8_t pin;
  uint8_t level;
  uint8_t def;
  char    desc[32];
} dbg_gpio_desc_t;

typedef struct _sensor_desc {
  char    name[32];
  uint8_t sensor_num;
  char    unit[5];
  uint8_t fru;
  uint8_t disp_prec;
} sensor_desc_t;

bool plat_supported(void);

/* Returns the pointer to the post phase table for the given FRU */
int plat_get_post_phase(uint8_t fru, post_phase_desc_t **desc, size_t *desc_count);

/* Return the GPIO descriptor table for the given FRU */
int plat_get_gdesc(uint8_t fru, dbg_gpio_desc_t **desc, size_t *desc_count);

/* Return the sensor descriptor table for the given FRU */
int plat_get_sensor_desc(uint8_t fru, sensor_desc_t **desc, size_t *desc_count);

/* Returns the FRU the hand-switch is switched to. If it is switched to BMC
 * it returns FRU_ALL. Note, if in err, it returns FRU_ALL */
uint8_t plat_get_fru_sel(void);

/* Returns the ME Status string for the given FRU */
int plat_get_me_status(uint8_t fru, char *status);

/* Returns the character representation of the Board ID */
int plat_get_board_id(char *id);

/* Return the system configuration for the FRU.  */
int plat_get_syscfg_text(uint8_t fru, char *syscfg);

int plat_get_etra_fw_version(uint8_t slot_id, char *fw_text);

/* Returns the extra information for the given FRU */
int plat_get_extra_sysinfo(uint8_t fru, char *info);

/* Returns the debug card UART selection number */
int plat_udbg_get_uart_sel_num(uint8_t *uart_sel_num);

/* Returns the debug card UART selection name by given UART selection number */
int plat_udbg_get_uart_sel_name(uint8_t uart_sel_num, char *uart_sel_name);

/* Returns the extra post code page info */
int plat_dword_postcode_buf(uint8_t fru, char *status);

/* Returns the DIMM loop description */
int plat_get_amd_mrc_desc(uint16_t major, uint16_t minor, char *desc);
int plat_get_mrc_desc(uint8_t fru, uint16_t major, uint16_t minor, char *desc);

#endif
