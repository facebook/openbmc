#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * base64_decode
 * Caller is responsible for freeing the returned buffer.
 */
uint8_t *base64_decode(const char *src, int32_t len, int32_t *out_len);

/**
 * base64_encode
 * Caller is responsible for freeing the returned buffer.
 */
char *base64_encode(const uint8_t *src, int32_t len, int32_t *out_len);

#ifdef __cplusplus
}
#endif
