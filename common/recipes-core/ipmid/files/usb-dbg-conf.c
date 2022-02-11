#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "usb-dbg-conf.h"

bool plat_supported(void)
{
  return false;
}

int plat_get_post_phase(uint8_t fru, post_phase_desc_t **desc, size_t *desc_count)
{
  return -1;
}

int plat_get_gdesc(uint8_t fru, gpio_desc_t **desc, size_t *desc_count)
{
  return -1;
}

int plat_get_sensor_desc(uint8_t fru, sensor_desc_t **desc, size_t *desc_count)
{
  return -1;
}

uint8_t plat_get_fru_sel(void)
{
  return 0;
}

int plat_get_me_status(uint8_t fru, char *status)
{
  return -1;
}

int plat_get_board_id(char *id)
{
  return -1;
}

int plat_get_syscfg_text(uint8_t fru, char *syscfg)
{
  return -1;
}

int
plat_get_etra_fw_version(uint8_t slot_id, char *fw_text)
{
  return -1;
}

int plat_get_extra_sysinfo(uint8_t fru, char *info)
{
  return -1;
}
