#ifndef __PAL_SWITCHTEC_H__
#define __PAL_SWITCHTEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SWITCHTEC_DEV "/dev/i2c-12@0x%x"
#define SWITCH_BASE_ADDR 0x18
#define SWITCHTEC_UPDATE_CMD \
  "switchtec fw-update "SWITCHTEC_DEV" %s -y"

enum {
  PAX_BL2 = 0,
  PAX_IMG,
  PAX_CFG,
};

int pal_get_pax_bl2_ver(uint8_t, char*);
int pal_get_pax_fw_ver(uint8_t, char*);
int pal_get_pax_cfg_ver(uint8_t, char*);
int pal_check_pax_fw_type(uint8_t, const char*);
int pal_pax_fw_update(uint8_t, const char*);
void pal_clear_pax_cache(uint8_t);
int pal_read_pax_dietemp(uint8_t, float*);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_SWITCHTEC_H__ */