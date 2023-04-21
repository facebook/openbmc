/**
 * @file platform.h
 * @brief Definition of platform functions for the SDK.
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include "aries_api.h"
#include "aries_margin.h"
#include "aries_link.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int bus;
  int addr;
} RETIMER_DEV;

RETIMER_DEV RT_DEV_INFO[] = {
  {60, 0x24},
  {61, 0x24},
  {62, 0x24},
  {63, 0x24},
  {64, 0x24},
  {65, 0x24},
  {66, 0x24},
  {67, 0x24},
};

int asteraI2CWriteBlockData(int handle, uint8_t cmdCode, uint8_t numBytes,
                            uint8_t* buf);

int asteraI2CReadBlockData(int handle, uint8_t cmdCode, uint8_t numBytes,
                           uint8_t* buf);

int asteraI2CBlock(int handle);

int asteraI2CUnblock(int handle);

// Fill in the basic config to ariesDeice and I2cDriver structure
AriesErrorType SetupAriesDevice (AriesDeviceType* ariesDevice,
             AriesI2CDriverType* i2cDriver, int bus, int addr);

// Perform FW update
AriesErrorType AriestFwUpdate(int bus, int addr, const char* fp);

// Get FW update
AriesErrorType AriesGetFwVersion(int bus, int addr, uint16_t* version);

// Get Current Temp
AriesErrorType AriesGetTemp(AriesDeviceType* ariesDevice,
        AriesI2CDriverType* i2cDriver, int bus, int addr, float* temp);

// Logs eye results to a file
AriesErrorType AriesMargin(int id, const char* type, const char* fp);

AriesErrorType AriesPrintState(int id);

#ifdef __cplusplus
}
#endif

#endif
