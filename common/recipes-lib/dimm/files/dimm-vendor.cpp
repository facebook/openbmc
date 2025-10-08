#include <stdio.h>
#include <stdint.h>
#include <string.h>
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
 { 0x8c8a, "Richtek"  },
};

struct loc_decode { uint8_t code; const char* name; };
static const loc_decode LOC_SAMSUNG[] = {
  { 1, "South Korea" },
  { 2, "China"       },
  { 3, "Philippines" },
  { 4, "Vietnam"     },
};

static const loc_decode LOC_MICRON[] = {
  {  1, "SIG (USA)"         },
  {  2, "MTB (Taiwan)"      },
  {  5, "MNG (Malaysia)"    },
  {  6, "MMP (Malaysia)"    },
  {  7, "MNI (India)"       },
  {  8, "SING (Singapore)"  },
  { 10, "MSI (India)"       },
  { 15, "MXA (China)"       },
  { 26, "Hotayi (Malaysia)" },
  { 37, "TSMT (Taiwan)"     },
};

static const loc_decode LOC_HYNIX[] = {
  { 1, "Korea" },
  { 2, "China"       },
  { 3, "Vietnam" },
};

static const char* lookup_loc(uint8_t code, const loc_decode* tbl, size_t n) {
  for (size_t i = 0; i < n; ++i)
    if (tbl[i].code == code) return tbl[i].name;
  return nullptr;
}

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

static const char* ddr_loc_string_by_vendor(const char* vendor, uint8_t code) {
  if (strcmp(vendor, "Samsung") == 0) {
    if (const char* s = lookup_loc(code, LOC_SAMSUNG, ARRAY_SIZE(LOC_SAMSUNG))) return s;
  } else if (strcmp(vendor, "Micron") == 0) {
    if (const char* s = lookup_loc(code, LOC_MICRON, ARRAY_SIZE(LOC_MICRON)))  return s;
  } else if (strcmp(vendor, "SK Hynix") == 0) {
    if (const char* s = lookup_loc(code, LOC_HYNIX, ARRAY_SIZE(LOC_HYNIX)))   return s;
  } else if (strcmp(vendor, "Nanya") == 0) {
    if (code == 0x4D) return "Taiwan";
  }

  static char hexbuf[6];
  snprintf(hexbuf, sizeof(hexbuf), "0x%02X", code);
  return hexbuf;
}

const char* get_ddr4_dimm_vendor_location_string(uint8_t fru_id, uint8_t cpu, uint8_t dimm) {
  static const char* kUnknown = "Unknown";
  util_set_EE_page(fru_id, cpu, dimm, 1);
  uint8_t id_buf[2] = {0}, present = 0;
  if (util_read_spd_with_retry(fru_id, cpu, dimm, 0x40, 2, 0, id_buf, &present) != 0 || !present) {
    return kUnknown;
  }
  uint16_t mfg_id = ((uint16_t)id_buf[1] << 8) | id_buf[0];
  const char* vendor = mfg_string(mfg_id);

  uint8_t loc = 0; present = 0;
  if (util_read_spd_with_retry(fru_id, cpu, dimm, 0x42, 1, 0, &loc, &present) != 0 || !present) {
    return kUnknown;
  }

  return ddr_loc_string_by_vendor(vendor, loc);
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

int get_spd5_dimm_vendor_location(uint8_t fru_id, uint8_t cpu, uint8_t dimm, char *out) {
  // byte514
  if (!out){
    return -1;
  }

  out[0] = '\0';

  char vendor[LEN_MFG_STRING] = {0};
  if (get_spd5_dimm_vendor(fru_id, cpu, dimm, vendor) < 0) {
    snprintf(out, LEN_MFG_STRING, "Unknown");
    return -1;
  }

  uint8_t loc_raw = 0, got = 0;
  if (util_read_spd_with_retry(fru_id, cpu, dimm, 0x0202, 1, 1000, &loc_raw, &got) != 0 || got != 1) {
    snprintf(out, LEN_MFG_STRING, "Unknown");
    return -1;
  }

  if (strcmp(vendor, "Samsung") == 0) {
    if (const char* s = lookup_loc(loc_raw, LOC_SAMSUNG, ARRAY_SIZE(LOC_SAMSUNG))) {
      snprintf(out, LEN_MFG_STRING, "%s", s);
      return 0;
    }
  } else if (strcmp(vendor, "Micron") == 0) {
    if (const char* s = lookup_loc(loc_raw, LOC_MICRON, ARRAY_SIZE(LOC_MICRON))) {
      snprintf(out, LEN_MFG_STRING, "%s", s);
      return 0;
    }
  } else if (strcmp(vendor, "SK Hynix") == 0) {
    if (const char* s = lookup_loc(loc_raw, LOC_HYNIX, ARRAY_SIZE(LOC_HYNIX))) {
      snprintf(out, LEN_MFG_STRING, "%s", s);
      return 0;
    }
  }


  snprintf(out, LEN_MFG_STRING, "0x%02X", loc_raw);
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
