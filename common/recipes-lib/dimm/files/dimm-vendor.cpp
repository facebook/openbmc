#include <stdio.h>
#include <stdint.h>
#include "dimm.h"

typedef struct _dimm_mfg {
  uint16_t id;
  const char *name;
} dimm_mfg;
static dimm_mfg dimm_manufacturer[] = {
 { 0x8980, "Intel"    },
 { 0x1080, "NEC"      },
 { 0xb380, "IDT"      },
 { 0x9780, "TI"       },
 { 0xad80, "SK Hynix" },
 { 0x2c80, "Micron"   },
 { 0xc180, "Infineon" },
 { 0xce80, "Samsung"  },
 { 0x3d80, "Tek"      },
 { 0x9801, "Kingston" },
 { 0x9401, "Smart"    },
 { 0xfe02, "Elpida"   },
 { 0xc802, "Agilent"  },
 { 0x9e02, "Corsair"  },
 { 0x0b83, "Nanya"    },
 { 0x9483, "Mushkin"  },
 { 0xb304, "Inphi"    },
 { 0xcb04, "ADATA"    },
 { 0x2304, "Renesas"  },
 { 0x5185, "Qimonda"  },
 { 0xba85, "Virtium"  },
 { 0x3286, "Montage"  },
 { 0xd086, "Silego"   },
 { 0x9d86, "Rambus"   },
 { 0x2a0b, "MPS"      },
};

const char *
mfg_string(uint16_t id) {
  uint8_t i;

  for (i = 0; i < ARRAY_SIZE(dimm_manufacturer); i++) {
    if (id == dimm_manufacturer[i].id) {
      return dimm_manufacturer[i].name;
    }
  }

  return "Unknown";
}

static int
get_spd5_mfg(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint16_t offs, char *mfg_str) {
  uint8_t buf[16] = {0}, dimm_present = 0;
  uint16_t mfg_id = 0;

  util_read_spd_with_retry(fru_id, cpu, dimm, offs, 2, 0, buf, &dimm_present);
  if (!dimm_present) {
    snprintf(mfg_str, LEN_MFG_STRING, "Unknown");
    return -1;
  }

  mfg_id = (buf[1] << 8) | buf[0];
  snprintf(mfg_str, LEN_MFG_STRING, "%s", mfg_string(mfg_id));

  return 0;
}

int
get_spd5_dimm_vendor(uint8_t fru_id, uint8_t cpu, uint8_t dimm, char *mfg_str) {
  // (byte513 << 8) | byte512
  return get_spd5_mfg(fru_id, cpu, dimm, 0x200, mfg_str);
}

int
get_spd5_reg_vendor(uint8_t fru_id, uint8_t cpu, uint8_t dimm, char *mfg_str) {
  // (byte241 << 8) | byte240
  return get_spd5_mfg(fru_id, cpu, dimm, 0xf0, mfg_str);
}

int
get_spd5_pmic_vendor(uint8_t fru_id, uint8_t cpu, uint8_t dimm, char *mfg_str) {
  // (byte199 << 8) | byte198
  return get_spd5_mfg(fru_id, cpu, dimm, 0xc6, mfg_str);
}

int
get_spd5_dimm_mfg_date(uint8_t fru_id, uint8_t cpu, uint8_t dimm, char *date_str) {
  uint8_t buf[16] = {0}, dimm_present = 0;

  // byte515 ~ byte516
  util_read_spd_with_retry(fru_id, cpu, dimm, 0x203, 2, 0, buf, &dimm_present);
  if (!dimm_present) {
    snprintf(date_str, LEN_MFG_STRING, "Unknown");
    return -1;
  }
  snprintf(date_str, LEN_MFG_STRING, "20%02x Week%02x", buf[0], buf[1]);

  return 0;
}
