/**
 * @file platform.h
 * @brief Definition of platform functions for the SDK.
 */

#ifndef COMMON_API_H_
#define COMMON_API_H_

#include "aries_api.h"
#include "aries_margin.h"
#include "aries_link.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CHECK_ERR_LOGGING(rc)                                              \
{                                                                          \
    if (rc != ARIES_SUCCESS)                                               \
    {                                                                      \
        ASTERA_ERROR("Unexpected return code: %d", rc);                    \
    }                                                                      \
}

#define CHECK_ERR_CLOSE(rc, fd)                                            \
{                                                                          \
    if (rc != ARIES_SUCCESS)                                               \
    {                                                                      \
        ASTERA_ERROR("Unexpected return code: %d", rc);                    \
        if (fd)                                                            \
        {                                                                  \
             close(fd);                                                    \
        }                                                                  \
    }                                                                      \
}

#define CHECK_ERR_RETURN(rc, fd)                                            \
{                                                                          \
    if (rc != ARIES_SUCCESS)                                               \
    {                                                                      \
        ASTERA_ERROR("Unexpected return code: %d", rc);                    \
        if (fd)                                                            \
        {                                                                  \
             close(fd);                                                    \
        }                                                                  \
        return rc;                                                         \
    }                                                                      \
}

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
