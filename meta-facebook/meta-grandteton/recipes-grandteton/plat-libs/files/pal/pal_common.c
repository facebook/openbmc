#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/libgpio.h>
#include "pal_common.h"
#include "pal_def.h"

bool
is_cpu_socket_occupy(uint8_t cpu_idx) {
  static bool cached = false;
  static unsigned int cached_id = 0;

  if (!cached) {
    const char *shadows[] = {
      "FM_CPU0_SKTOCC_LVT3_PLD_N",
      "FM_CPU1_SKTOCC_LVT3_PLD_N"
    };
    if (gpio_get_value_by_shadow_list(shadows, ARRAY_SIZE(shadows), &cached_id)) {
      return false;
    }
    cached = true;
  }

  // bit == 1 implies CPU is absent.
  if (cached_id & (1 << cpu_idx)) {
    return false;
  }
  return true;
}

bool
pal_skip_access_me(void) {
  if (!access("/tmp/fin_bios_upd", F_OK)) {
    return true;
  }

  return false;
}
