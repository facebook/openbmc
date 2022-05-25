#ifndef __PAL_GUID_H__
#define __PAL_GUID_H__

#include <openbmc/obmc-pal.h>
#include <facebook/netlakemtp_common.h>

#ifdef __cplusplus
extern "C" {
#endif

static int pal_set_guid(uint16_t offset, char *guid);
static int pal_get_guid(uint16_t offset, char *guid);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PAL_H__ */
