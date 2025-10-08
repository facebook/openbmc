/*
 * Minimal platform constants for Yosemite 4 when reading SPD from BIC cache.
 * No DIMM I2C addresses here.
 */
#ifndef __DIMM_UTIL_PLAT_H__
#define __DIMM_UTIL_PLAT_H__

/* ---- YV4 topology / FRU  ---- */
#define NUM_CPU_YV4              1
#define MAX_DIMM_PER_CPU_YV4     20    /* Host(0-11) + CXL(12-19) DIMMs */

#define FRU_ID_MIN_YV4           1
#define FRU_ID_MAX_YV4           8
#define FRU_ID_ALL_YV4           9
#define NUM_FRU_YV4              (FRU_ID_MAX_YV4)

/* ---- SPD via BIC cache ---- */
#ifndef SPD_RAW_LEN
#define SPD_RAW_LEN          0x280   /* DDR5 SPD : 0x000 ~ 0x27F */
#endif

#ifndef DIMM_SPD_CACHE
#define DIMM_SPD_CACHE       0x03    /* BIC OEM device_type: SPD Cache (2-byte offset) */
#endif

/* ---- PLDM WF-BIC 相關定義 ---- */
#ifndef PLDM_WF_DEF_TID
#define PLDM_WF_DEF_TID      0x08
#endif

#ifndef PLDM_WF_OEM_TYPE
#define PLDM_WF_OEM_TYPE     0x3F    /* PLDM OEM type */
#endif

#ifndef PLDM_WF_READ_SPD_CHUNK
#define PLDM_WF_READ_SPD_CHUNK 0x05  /* WF-BIC SPD chunk read command */
#endif

#ifndef PLDM_WF_READ_SPD_COMPACT
#define PLDM_WF_READ_SPD_COMPACT 0x01 /* SD-BIC compact SPD read command */
#endif

#ifndef PLDM_WF_IANA0
#define PLDM_WF_IANA0        0x15    /* IANA Enterprise Number */
#define PLDM_WF_IANA1        0xA0
#define PLDM_WF_IANA2        0x00
#endif

#ifndef PLDM_WF_MAX_CHUNK
#define PLDM_WF_MAX_CHUNK    0x40    /* 64 bytes max per chunk */
#endif

/* ---- Helper functions ---- */
static inline uint8_t plat_bic_tid_from_fru_fallback(uint8_t /*fru*/) {
  return PLDM_WF_DEF_TID;
}

static inline unsigned char plat_mctp_eid_from_slot(unsigned char slot_id) {
  return (unsigned char)(slot_id * 10);
}

/* CXL DIMM EID mapping: slot*10+2 (12/22/.../82) */
static inline unsigned char plat_mctp_eid_cxl_from_slot(unsigned char slot_id) {
  return (unsigned char)(slot_id * 10 + 2);
}

/* DIMM index helpers */
static inline bool plat_is_host_dimm(uint8_t dimm) {
  return dimm <= 11;
}

static inline bool plat_is_cxl_dimm(uint8_t dimm) {
  return dimm >= 12 && dimm <= 19;
}

static inline uint8_t plat_cxl_dimm_to_local_index(uint8_t dimm) {
  return (uint8_t)(dimm - 12);  /* 12->0, 13->1, ..., 19->7 */
}

static inline uint8_t plat_cxl_dimm_to_mcio_index(uint8_t dimm) {
  uint8_t local = plat_cxl_dimm_to_local_index(dimm);
  return (local < 4) ? 0 : 1;  /* 0=MCIO4, 1=MCIO3 */
}

static inline uint8_t plat_cxl_dimm_to_mcio_dimm(uint8_t dimm) {
  uint8_t local = plat_cxl_dimm_to_local_index(dimm);
  return (local & 0x3);  /* 0..3 within each MCIO */
}

#endif /* __DIMM_UTIL_PLAT_H__ */