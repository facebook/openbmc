/*
 * Minimal platform constants for Yosemite 4 when reading SPD from BIC cache.
 * No DIMM I2C addresses here.
 */
#ifndef __DIMM_UTIL_PLAT_H__
#define __DIMM_UTIL_PLAT_H__

/* ---- YV4 topology / FRU  ---- */
#define NUM_CPU_YV4              1
#define MAX_DIMM_PER_CPU_YV4     12

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

static inline unsigned char plat_mctp_eid_from_slot(unsigned char slot_id) {
  return (unsigned char)(slot_id * 10);
}


#endif /* __DIMM_UTIL_PLAT_H__ */
