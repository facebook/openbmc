/**
 * @file platform.h
 * @brief Definition of platform functions for the SDK.
 */

#include "aries_api.h"

#ifdef __cplusplus
extern "C" {
#endif

int asteraI2CWriteBlockData(int handle, uint8_t cmdCode, uint8_t numBytes,
                            uint8_t* buf);

int asteraI2CReadBlockData(int handle, uint8_t cmdCode, uint8_t numBytes,
                           uint8_t* buf);

int asteraI2CBlock(int handle);

int asteraI2CUnblock(int handle);

AriesErrorType AriesInit(int bus, int addr);
AriesErrorType AriestFwUpdate(int bus, int addr, const char* fp);
AriesErrorType AriesGetFwVersion(int bus, int addr, uint16_t* version);
AriesErrorType AriesGetTemp(int bus, int addr, float* temp);

#ifdef __cplusplus
}
#endif
