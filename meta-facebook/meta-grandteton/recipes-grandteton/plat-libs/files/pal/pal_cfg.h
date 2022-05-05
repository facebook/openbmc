#ifndef __PAL_CFG_H__
#define __PAL_CFG_H__

int pal_set_def_key_value(void);
int pal_get_syscfg_text(char *text);
int pal_get_key_value(char *key, char *value);
int pal_set_key_value(char *key, char *value);
int pal_set_ppin_info(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len);
#endif
