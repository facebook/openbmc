#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include "pal.h"
#include <openbmc/kv.h>

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
  CARD_1AVA = 1,
  CARD_2AVA = 2,
  CARD_3AVA = 3,
  CARD_4AVA = 4,
  CARD_RETIMER = 5,
  CARD_HBA = 6,
  CARD_OTHER_FLASH = 7,
  CARD_UNKNOWN = 0x80,
  // Custom to ease computation. Not
  // sent by BIOS.
  CARD_NIC = 0xff,
  CARD_AVA = 0xfe
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

#define SS_IDX 0
#define DS_IDX 1
#define CONF_START 2
struct conf_s {
  const char *name;
  const char *desc;
  machine_config_info info;
};

static struct conf_s configurations[] = {
  {"SS_D", "Single Side, default", {0}},
  {"DS_D", "Double Side, default", {0}},
  {"SS_0", "Type 8 Head Node", {0, SINGLE_SIDE, 2, 4, 0, 0, RISER_2SLOT, 0, CARD_NIC, CARD_ABSENT, CARD_RETIMER, CARD_ABSENT, 0}},
  {"SS_1", "Type 6/8 compute (no-flash)", {0, SINGLE_SIDE, 2, 8, 1, 0, RISER_ABSENT, 0, CARD_NIC, CARD_ABSENT, CARD_ABSENT, CARD_ABSENT, 0}},
  {"SS_2", "Custom 3 Ava cards, no boot-drive", {0, SINGLE_SIDE, 2, 8, 0, 0, RISER_3SLOT, 0, CARD_NIC, CARD_AVA, CARD_AVA, CARD_AVA, 0}},
  {"SS_3", "Type 6 with Ava, no boot-drive", {0, SINGLE_SIDE, 2, 8, 0, 0, RISER_2SLOT, 0, CARD_NIC, CARD_AVA, CARD_ABSENT, CARD_ABSENT, 0}},
  {"SS_4", "Type 6 with Ava", {0, SINGLE_SIDE, 2, 8, 1, 0, RISER_2SLOT, 0, CARD_NIC, CARD_AVA, CARD_ABSENT, CARD_ABSENT, 0}},
  {"SS_5", "Type 6 with Ava (alt)", {0, SINGLE_SIDE, 2, 8, 0, 0, RISER_2SLOT, 0, CARD_NIC, CARD_ABSENT, CARD_AVA, CARD_ABSENT, 0}},
  {"SS_6", "JBOG", {0, SINGLE_SIDE, 2, 8, 1, 0, RISER_2SLOT, 0, CARD_NIC, CARD_RETIMER, CARD_RETIMER, CARD_ABSENT, 0}},
  {"SS_7", "Type 3 with Ava", {0, SINGLE_SIDE, 2, 8, 0, 0, RISER_2SLOT, 0, CARD_NIC, CARD_AVA, CARD_AVA, CARD_ABSENT, 0}},
  {"SS_8", "Type 9 with HBA", {0, SINGLE_SIDE, 2, 8, 0, 0, RISER_2SLOT, 0, CARD_NIC, CARD_HBA, CARD_HBA, CARD_ABSENT, 0}},
  {"SS_9", "Type 6 with Ava (top slot)", {0, SINGLE_SIDE, 2, 8, 1, 0, RISER_2SLOT, 0, CARD_NIC, CARD_ABSENT, CARD_AVA, CARD_ABSENT, 0}},
  {"DS_0", "DS compute", {0, DOUBLE_SIDE, 2, 12, 0, 0, RISER_2SLOT, 0, CARD_NIC, CARD_ABSENT, CARD_ABSENT, CARD_ABSENT, 0}},
  {"DS_1", "DS compute with boot drive", {0, DOUBLE_SIDE, 2, 12, 1, 0, RISER_2SLOT, 0, CARD_NIC, CARD_ABSENT, CARD_ABSENT, CARD_ABSENT, 0}},
  {"DS_2", "DS memory without boot-drive", {0, DOUBLE_SIDE, 2, 24, 0, 0, RISER_2SLOT, 0, CARD_NIC, CARD_ABSENT, CARD_ABSENT, CARD_ABSENT, 0}},
  {"DS_3", "DS memory with boot-drive", {0, DOUBLE_SIDE, 2, 24, 1, 0, RISER_2SLOT, 0, CARD_NIC, CARD_ABSENT, CARD_ABSENT, CARD_ABSENT, 0}},
  {"DS_4", "DS AEP without boot-drive", {0, DOUBLE_SIDE, 2, 12, 0, 0, RISER_2SLOT, 0, CARD_NIC, CARD_ABSENT, CARD_ABSENT, CARD_ABSENT, 12}},
  {"DS_5", "DS AEP with boot-drive", {0, DOUBLE_SIDE, 2, 12, 1, 0, RISER_2SLOT, 0, CARD_NIC, CARD_ABSENT, CARD_ABSENT, CARD_ABSENT, 12}}
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

static uint8_t pcie_card_resolve(uint8_t type)
{
  switch(type) {
    case CARD_ABSENT:
    case CARD_RETIMER:
    case CARD_HBA:
    case CARD_OTHER_FLASH:
      break;
    case CARD_1AVA:
    case CARD_2AVA:
    case CARD_3AVA:
    case CARD_4AVA:
      type = CARD_AVA;
      break;
    case CARD_UNKNOWN:
    default:
      type = CARD_UNKNOWN;
      break;
  }
  return type;
}

// Sets to known values of knobs we do not care
// about. So, the lookup logic can be easier.
static void set_defaults(machine_config_info *mc)
{
  mc->chassis_type = 0;
  mc->processor_count = 2;
  mc->hdd25_count = 0;
  mc->pcie_card_loc = 0;
  mc->slot1_pciecard_type = CARD_NIC; // NIC always exists
  mc->slot2_pciecard_type = pcie_card_resolve(mc->slot2_pciecard_type);
  mc->slot3_pciecard_type = pcie_card_resolve(mc->slot3_pciecard_type);
  mc->slot4_pciecard_type = pcie_card_resolve(mc->slot4_pciecard_type);
}

static bool config_equal(machine_config_info *m1, machine_config_info *m2)
{
  return memcmp(m1, m2, sizeof(machine_config_info)) == 0;
}

static struct conf_s *machine_config(machine_config_info *mc)
{
  int i;

  set_defaults(mc);

  for(i = CONF_START; i < NUM_CONFIGURATIONS; i++) {
    if (config_equal(mc, &configurations[i].info)) {
      return &configurations[i];
    }
  }
  if (mc->MB_type == SINGLE_SIDE) {
    return &configurations[SS_IDX];
  }
  if (mc->MB_type == DOUBLE_SIDE) {
    return &configurations[DS_IDX];
  }
  return NULL;
}

int
pal_set_machine_configuration(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  machine_config_info mc;
  struct conf_s *conf;

  if (req_len < sizeof(machine_config_info)) {
    syslog(LOG_CRIT, "Invalid machine configuration received");
    return -1;
  }

  sprintf(key, "mb_machine_config");
  kv_set(key, (char *)req_data, sizeof(machine_config_info), KV_FPERSIST);

  memcpy(&mc, &req_data[0], sizeof(machine_config_info));
  conf = machine_config(&mc);
  if (!conf) {
    strcpy(value, default_machine_config_name());
    kv_set("mb_system_conf", value, 0, KV_FPERSIST);
    kv_set("mb_system_conf_desc", value, 0, KV_FPERSIST);
    return 0;
  }
  strcpy(value, conf->name);
  kv_set("mb_system_conf", value, 0, KV_FPERSIST);
  memset(value, 0, sizeof(value));
  strcpy(value, conf->desc);
  kv_set("mb_system_conf_desc", value, 0, KV_FPERSIST);
  return 0;
}

int pal_get_machine_configuration(char *conf)
{
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  int ret;

  sprintf(key, "mb_system_conf");

  ret = kv_get(key, value, NULL, KV_FPERSIST);
  if (ret < 0) {
    strcpy(value, default_machine_config_name());
    kv_set(key, value, 0, KV_FPERSIST | KV_FCREATE);
  }
  strcpy(conf, value);
  return 0;
}
