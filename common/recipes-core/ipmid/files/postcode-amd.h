#if defined CONFIG_POSTCODE_AMD
#include <stdint.h>
#include <string.h>
#include <stdio.h>
void pal_parse_amd_sec_pei_dxe(uint32_t post_code, char *str );;
void pal_parse_amd_agesa(uint32_t post_code, char *str );
void pal_parse_amd_abl(uint32_t post_code, char *str );
void pal_parse_amd_posrt_80(uint32_t post_code, char *str );
void pal_parse_amd_post_code_helper(uint32_t post_code, char *str );
#endif