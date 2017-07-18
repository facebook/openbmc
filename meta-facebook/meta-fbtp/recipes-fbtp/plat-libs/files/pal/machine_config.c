#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include "pal.h"
#include <openbmc/kv.h>
#include <openbmc/edb.h>

#define PLAT_ID_SKU_MASK 0x10 // BIT4: 0- Single Side, 1- Double Side

enum {
  SINGLE_SIDE = 0,
  DOUBLE_SIDE = 1
};

enum {
  RISER_ABSENT = 0,
  RISER_2SLOT  = 1,
  RISER_3SLOT  = 2
};

enum {
  CARD_ABSENT = 0,
  CARD_PRESENT = 1
};

typedef struct
{
  uint8_t chassis_type; // 00 - ORv1, 01 - ORv2 (FBTP)
  uint8_t MB_type;      // 00 - SS, 01 - DS, 02 - Type3
  uint8_t processor_count;
  uint8_t memory_count;
  uint8_t hdd35_count;  // 0/1 in FBTP, ff - unknown
  uint8_t hdd25_count; // 0 for FBTP
  uint8_t riser_type;  // 00 - not installed, 01 - 2 slot, 02 - 3 slot
  uint8_t pcie_card_loc; // Bit0 - Slot1 Present/Absent, Bit1 - Slot 2 Present/Absent etc.
  uint8_t slot1_pciecard_type; // Always NIC for FBTP
  uint8_t slot2_pciecard_type; // 2-4: 00 - Absent, 01 - AVA 2 x m.2, 02 - AVA 3x m.2,
  uint8_t slot3_pciecard_type; //      03 - AVA 4 x m.2, 04 - Re-timer, 05 - HBA
  uint8_t slot4_pciecard_type; //      06 - Other flash cards (Intel, HGST), 80 - Unknown
  uint8_t AEP_mem_count;
} machine_config_info;

#define NUM_CONFIGURATIONS (sizeof(configurations)/sizeof(configurations[0]))

static struct {
  const char *name;
  machine_config_info info;
} configurations[15] = {
  {"SS_0", {0, SINGLE_SIDE, 2, 4, 0, 0, RISER_2SLOT, 0, CARD_PRESENT, CARD_ABSENT, CARD_PRESENT, CARD_ABSENT, 0}},
  {"SS_1", {0, SINGLE_SIDE, 2, 8, 1, 0, RISER_ABSENT, 0, CARD_PRESENT, CARD_ABSENT, CARD_ABSENT, CARD_ABSENT, 0}},
  {"SS_2", {0, SINGLE_SIDE, 2, 8, 0, 0, RISER_3SLOT, 0, CARD_PRESENT, CARD_PRESENT, CARD_PRESENT, CARD_PRESENT, 0}},
  {"SS_3", {0, SINGLE_SIDE, 2, 8, 0, 0, RISER_2SLOT, 0, CARD_PRESENT, CARD_PRESENT, CARD_ABSENT, CARD_ABSENT, 0}},
  {"SS_4", {0, SINGLE_SIDE, 2, 8, 1, 0, RISER_2SLOT, 0, CARD_PRESENT, CARD_PRESENT, CARD_ABSENT, CARD_ABSENT, 0}},
  {"SS_5", {0, SINGLE_SIDE, 2, 8, 0, 0, RISER_2SLOT, 0, CARD_PRESENT, CARD_ABSENT, CARD_PRESENT, CARD_ABSENT, 0}},
  {"SS_6", {0, SINGLE_SIDE, 2, 8, 1, 0, RISER_2SLOT, 0, CARD_PRESENT, CARD_PRESENT, CARD_PRESENT, CARD_ABSENT, 0}},
  {"SS_7", {0, SINGLE_SIDE, 2, 8, 0, 0, RISER_2SLOT, 0, CARD_PRESENT, CARD_PRESENT, CARD_PRESENT, CARD_ABSENT, 0}},
  {"SS_8", {0, SINGLE_SIDE, 2, 8, 0, 0, RISER_2SLOT, 0, CARD_PRESENT, CARD_PRESENT, CARD_PRESENT, CARD_ABSENT, 0}},
  {"DS_0", {0, DOUBLE_SIDE, 2, 12, 0, 0, RISER_2SLOT, 0, CARD_PRESENT, CARD_ABSENT, CARD_ABSENT, CARD_ABSENT, 0}},
  {"DS_1", {0, DOUBLE_SIDE, 2, 12, 1, 0, RISER_2SLOT, 0, CARD_PRESENT, CARD_ABSENT, CARD_ABSENT, CARD_ABSENT, 0}},
  {"DS_2", {0, DOUBLE_SIDE, 2, 24, 0, 0, RISER_2SLOT, 0, CARD_PRESENT, CARD_ABSENT, CARD_ABSENT, CARD_ABSENT, 0}},
  {"DS_3", {0, DOUBLE_SIDE, 2, 24, 1, 0, RISER_2SLOT, 0, CARD_PRESENT, CARD_ABSENT, CARD_ABSENT, CARD_ABSENT, 0}},
  {"DS_4", {0, DOUBLE_SIDE, 2, 12, 0, 0, RISER_2SLOT, 0, CARD_PRESENT, CARD_ABSENT, CARD_ABSENT, CARD_ABSENT, 12}},
  {"DS_5", {0, DOUBLE_SIDE, 2, 12, 1, 0, RISER_2SLOT, 0, CARD_PRESENT, CARD_ABSENT, CARD_ABSENT, CARD_ABSENT, 12}},
};

static const char *default_machine_config_name(void)
{
  uint8_t plat_id;
  if (pal_get_platform_id(&plat_id)) {
    /* Default to SS */
    return "SS_D";
  }
  if (plat_id & PLAT_ID_SKU_MASK) {
    return "DS_D";
  }
  return "SS_D";
}

// Sets to known values of knobs we do not care
// about. So, the lookup logic can be easier.
static void set_defaults(machine_config_info *mc)
{
  mc->chassis_type = 0;
  mc->processor_count = 2;
  mc->hdd25_count = 0;
  mc->pcie_card_loc = 0;
  mc->slot1_pciecard_type = CARD_PRESENT; // NIC always exists
  mc->slot2_pciecard_type = mc->slot2_pciecard_type > 0 ? CARD_PRESENT : CARD_ABSENT;
  mc->slot3_pciecard_type = mc->slot3_pciecard_type > 0 ? CARD_PRESENT : CARD_ABSENT;
  mc->slot4_pciecard_type = mc->slot4_pciecard_type > 0 ? CARD_PRESENT : CARD_ABSENT;
}

static bool config_equal(machine_config_info *m1, machine_config_info *m2)
{
  return memcmp(m1, m2, sizeof(machine_config_info)) == 0;
}

static const char *machine_config_name(machine_config_info *mc)
{
  int i;
  for(i = 0; i < NUM_CONFIGURATIONS; i++) {
    if (config_equal(mc, &configurations[i].info)) {
      return configurations[i].name;
    }
  }
  if (mc->MB_type == SINGLE_SIDE) {
    return "SS_D";
  }
  if (mc->MB_type == DOUBLE_SIDE) {
    return "DS_D";
  }
  return default_machine_config_name();
}

int
pal_set_machine_configuration(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  machine_config_info mc;

  if (req_len < sizeof(machine_config_info)) {
    syslog(LOG_CRIT, "Invalid machine configuration received");
    return -1;
  }

  sprintf(key, "mb_machine_config");
  kv_set_bin(key, (char *)req_data, sizeof(machine_config_info));

  memcpy(&mc, &req_data[0], sizeof(machine_config_info));

  sprintf(key, "mb_system_conf");

  set_defaults(&mc);
  strcpy(value, machine_config_name(&mc));

  /* Set kv first because get_machine_configuration
   * tests for cache first and then for kv. That way
   * we avoid (at minimum shrink the window for)
   * the potential race between the two */
  kv_set(key, value);
  edb_cache_set(key, value);
  return 0;
}

int pal_get_machine_configuration(char *conf)
{
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  int ret;

  sprintf(key, "mb_system_conf");
  /* Cache is the most current value, if that fails,
   * check persistent kv store for previous boot conf,
   * if that fails, then get platform ID to ensure
   * we use default SS or DS */
  ret = edb_cache_get(key, value);
  if (ret < 0) {
    ret = kv_get(key, value);
    if (ret < 0) {
      strcpy(value, default_machine_config_name());
      kv_set(key, value);
    }
    edb_cache_set(key, value);
  }
  strcpy(conf, value);
  return 0;
}

