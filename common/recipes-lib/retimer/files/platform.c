/**
 * @file platform.c
 * @Implementation of platform functions for the SDK.
 */

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <linux/i2c-dev.h>
#include <openbmc/obmc-i2c.h>
#include "include/platform.h"
#include "include/aries_margin.h"
#include "include/aries_link.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_RETIMERS 1
#define NUM_LINKS_PER_RETIMER 1
#define NUM_TOTAL_LINKS NUM_RETIMERS * NUM_LINKS_PER_RETIMER

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

/**
 * @brief Set I2C slave address
 *
 * @param[in]  file     I2C handle
 * @param[in]  address  Slave address
 * @param[in]  force    Override user provied slave address with default
 *                      I2C_SLAVE address
 * @return     int      Zero if success, else a negative value
 */
int asteraI2CSetSlaveAddress(int file, int address, int force)
{
    /* With force, let the user read from/write to the registers
       even when a driver is also running */
    if (ioctl(file, force ? I2C_SLAVE_FORCE : I2C_SLAVE, address) < 0)
    {
        fprintf(stderr, "Error: Could not set address to 0x%02x: %s\n", address,
                strerror(errno));
        return -errno;
    }
    return 0; // Equivalent to ARIES_SUCCESS
}

/**
 * @brief Open I2C connection
 *
 * @param[in]  i2cBus      I2C Bus
 * @param[in]  address  Slave address
 */
int asteraI2COpenConnection(int i2cBus, int slaveAddress)
{
    int file;
    int quiet = 0;
    char filename[20];
    int size = sizeof(filename);

    snprintf(filename, size, "/dev/i2c/%d", i2cBus);
    filename[size - 1] = '\0';
    file = open(filename, O_RDWR);

    if (file < 0 && (errno == ENOENT || errno == ENOTDIR))
    {
        sprintf(filename, "/dev/i2c-%d", i2cBus);
        file = open(filename, O_RDWR);
    }

    if (file < 0 && !quiet)
    {
        if (errno == ENOENT)
        {
            fprintf(stderr,
                    "Error: Could not open file "
                    "`/dev/i2c-%d' or `/dev/i2c/%d': %s\n",
                    i2cBus, i2cBus, strerror(ENOENT));
        }
        else
        {
            fprintf(stderr,
                    "Error: Could not open file "
                    "`%s': %s\n",
                    filename, strerror(errno));
            if (errno == EACCES)
            {
                fprintf(stderr, "Run as root?\n");
            }
        }
    }
    asteraI2CSetSlaveAddress(file, slaveAddress, 0);
    return file;
}

/**
 * @brief Close I2C connection
 *
 * @param[in]  file      I2C handle
 */
void asteraI2CCloseConnection(int file)
{
    close(file);
}

int asteraI2CWriteBlockData(int handle, uint8_t cmdCode, uint8_t numBytes,
                            uint8_t* buf)
{ 
    return i2c_smbus_write_i2c_block_data(handle, cmdCode, numBytes, buf);
}

int asteraI2CReadBlockData(int handle, uint8_t cmdCode, uint8_t numBytes,
                           uint8_t* buf)
{
    return i2c_smbus_read_i2c_block_data(handle, cmdCode, numBytes, buf);
}

int asteraI2CBlock(int handle)
{
    return 0; // Equivalent to ARIES_SUCCESS
}

int asteraI2CUnblock(int handle)
{
    return 0; // Equivalent to ARIES_SUCCESS
}

AriesErrorType AriesInit(int bus, int addr)
{
    AriesDeviceType* ariesDevice;
    AriesI2CDriverType* i2cDriver;
    AriesErrorType rc;
    int ariesHandle;
    int i2cBus = bus;
    int ariesSlaveAddress = addr;
    AriesDevicePartType partNumber = ARIES_PTX16;

    asteraLogSetLevel(2);

    // Open connection to Aries Retimer
    ariesHandle = asteraI2COpenConnection(i2cBus, ariesSlaveAddress);

    // Initialize I2C Driver for SDK transactions
    i2cDriver = (AriesI2CDriverType*)malloc(sizeof(AriesI2CDriverType));

    if (i2cDriver == NULL) {
        return ARIES_OUT_OF_MEMORY;
    }

    i2cDriver->handle = ariesHandle;
    i2cDriver->slaveAddr = ariesSlaveAddress;
    i2cDriver->pecEnable = ARIES_I2C_PEC_DISABLE;
    i2cDriver->i2cFormat = ARIES_I2C_FORMAT_ASTERA;
    // Flag to indicate lock has not been initialized. Call ariesInitDevice()
    // later to initialize.
    i2cDriver->lockInit = 0;

    // Initialize Aries device structure
    ariesDevice = (AriesDeviceType*)malloc(sizeof(AriesDeviceType));

    if (ariesDevice == NULL) {
        return ARIES_OUT_OF_MEMORY;
    }

    ariesDevice->i2cDriver = i2cDriver;
    ariesDevice->i2cBus = i2cBus;
    ariesDevice->partNumber = partNumber;


    rc = ariesInitDevice(ariesDevice, ariesSlaveAddress);
    if (rc != ARIES_SUCCESS)
    {
       ASTERA_ERROR("Init device failed");
    }

    uint8_t dataBytes[4];
    rc = ariesReadBlockData(i2cDriver, 0, 4, dataBytes);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Failed to read gbl_param_reg0");
        return rc;
    }
    int glb_param_reg0 = (dataBytes[3] << 24) + (dataBytes[2] << 16) +
                         (dataBytes[1] << 8) + dataBytes[0];


    asteraI2CCloseConnection(ariesHandle);

    if (ariesDevice) {
        free(ariesDevice);
    }

    if (i2cDriver) {
        free(i2cDriver);
    }

    return rc;
}

AriesErrorType AriestFwUpdate(int bus, int addr, const char* fp)
{
    AriesDeviceType* ariesDevice;
    AriesI2CDriverType* i2cDriver;
    AriesErrorType rc;
    int ariesHandle;
    int i2cBus = bus;
    int ariesSlaveAddress = addr;
    AriesDevicePartType partNumber = ARIES_PTX16;

    asteraLogSetLevel(2);

    ariesHandle = asteraI2COpenConnection(i2cBus, ariesSlaveAddress);

    i2cDriver = (AriesI2CDriverType*)malloc(sizeof(AriesI2CDriverType));

    if (i2cDriver == NULL) {
        return ARIES_OUT_OF_MEMORY;
    }

    i2cDriver->handle = ariesHandle;
    i2cDriver->slaveAddr = ariesSlaveAddress;
    i2cDriver->pecEnable = ARIES_I2C_PEC_DISABLE;
    i2cDriver->i2cFormat = ARIES_I2C_FORMAT_ASTERA;

    i2cDriver->lockInit = 0;

    ariesDevice = (AriesDeviceType*)malloc(sizeof(AriesDeviceType));
    if (ariesDevice == NULL) {
        return ARIES_OUT_OF_MEMORY;
    }

    ariesDevice->i2cDriver = i2cDriver;
    ariesDevice->i2cBus = i2cBus;
    ariesDevice->partNumber = partNumber;

    // -------------------------------------------------------------------------
    // INITIALIZATION
    // -------------------------------------------------------------------------
    // Check Connection and Init device
    // If the connection is not good, the ariesInitDevice() API will enable ARP
    // and update the i2cDriver with the new address. It also checks for the
    // Main Micro heartbeat before reading the FW version. In case the heartbeat
    // is not up, it sets the firmware version to 0.0.0.
    rc = ariesInitDevice(ariesDevice, ariesSlaveAddress);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Init device failed");
        return rc;
    }

    rc = ariesUpdateFirmware(ariesDevice, fp,
                                 ARIES_FW_IMAGE_FORMAT_IHX);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Failed to update the firmware image. RC = %d", rc);
    }

    // Reboot device to check if FW version was applied
    // Assert HW reset
    ASTERA_INFO("Performing Retimer HW reset ...");
    rc = ariesSetHwReset(ariesDevice, 1);
    CHECK_SUCCESS(rc);
    // Wait 10 ms before de-asserting
    usleep(10000);
    // De-assert HW reset
    rc = ariesSetHwReset(ariesDevice, 0);
    CHECK_SUCCESS(rc);
    usleep(5000000);

    rc = ariesInitDevice(ariesDevice, ariesSlaveAddress);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Init device failed");
        return -1;
    }

    ASTERA_INFO("Updated FW Version is %d.%d.%d", ariesDevice->fwVersion.major,
                ariesDevice->fwVersion.minor, ariesDevice->fwVersion.build);

    asteraI2CCloseConnection(ariesHandle);

    if (ariesDevice) {
        free(ariesDevice);
    }

    if (i2cDriver) {
        free(i2cDriver);
    }

    return ARIES_SUCCESS;
}

AriesErrorType AriesGetFwVersion(int bus, int addr, uint16_t* ver)
{
    AriesDeviceType* ariesDevice;
    AriesI2CDriverType* i2cDriver;
    AriesErrorType rc;
    int ariesHandle;
    int i2cBus = bus;
    int ariesSlaveAddress = addr;
    AriesDevicePartType partNumber = ARIES_PTX16;

    asteraLogSetLevel(2);

    ariesHandle = asteraI2COpenConnection(i2cBus, ariesSlaveAddress);

    i2cDriver = (AriesI2CDriverType*)malloc(sizeof(AriesI2CDriverType));
    i2cDriver->handle = ariesHandle;
    i2cDriver->slaveAddr = ariesSlaveAddress;
    i2cDriver->pecEnable = ARIES_I2C_PEC_DISABLE;
    i2cDriver->i2cFormat = ARIES_I2C_FORMAT_ASTERA;

    i2cDriver->lockInit = 0;

    ariesDevice = (AriesDeviceType*)malloc(sizeof(AriesDeviceType));
    ariesDevice->i2cDriver = i2cDriver;
    ariesDevice->i2cBus = i2cBus;
    ariesDevice->partNumber = partNumber;

    ariesFWStatusCheck(ariesDevice);

    ver[0] = ariesDevice->fwVersion.major;
    ver[1] = ariesDevice->fwVersion.minor;
    ver[2] = ariesDevice->fwVersion.build;

    asteraI2CCloseConnection(ariesHandle);

    if (ariesDevice) {
        free(ariesDevice);
    }

    if (i2cDriver) {
        free(i2cDriver);
    }

    return ARIES_SUCCESS;
}

AriesErrorType AriesGetTemp(int bus, int addr, float* temp)
{
    AriesDeviceType* ariesDevice;
    AriesI2CDriverType* i2cDriver;
    AriesErrorType rc;
    int ariesHandle;
    int i2cBus = bus;
    int ariesSlaveAddress = addr;
    AriesDevicePartType partNumber = ARIES_PTX16;

    asteraLogSetLevel(2);

    ariesHandle = asteraI2COpenConnection(i2cBus, ariesSlaveAddress);
    i2cDriver = (AriesI2CDriverType*)malloc(sizeof(AriesI2CDriverType));
    if (i2cDriver == NULL) {
        return ARIES_OUT_OF_MEMORY;
    }

    i2cDriver->handle = ariesHandle;
    i2cDriver->slaveAddr = ariesSlaveAddress;
    i2cDriver->pecEnable = ARIES_I2C_PEC_DISABLE;
    i2cDriver->i2cFormat = ARIES_I2C_FORMAT_ASTERA;

    i2cDriver->lockInit = 0;

    ariesDevice = (AriesDeviceType*)malloc(sizeof(AriesDeviceType));
    if (ariesDevice == NULL) {
        return ARIES_OUT_OF_MEMORY;
    }

    ariesDevice->i2cDriver = i2cDriver;
    ariesDevice->i2cBus = i2cBus;
    ariesDevice->partNumber = partNumber;
    ariesDevice->tempCalCodeAvg = 84;

    rc = ariesFWStatusCheck(ariesDevice);
    if (rc != ARIES_SUCCESS) {
        return rc;
    }

    if (ariesFirmwareIsAtLeast(ariesDevice, 2, 2, 0)) {
        ariesDevice->i2cDriver->mmWideRegisterValid = 1;
    }
    else {
        ariesDevice->i2cDriver->mmWideRegisterValid = 0;
    }

    // Read Temperature
    rc = ariesGetCurrentTemp(ariesDevice);
    CHECK_SUCCESS(rc);

    *temp = ariesDevice->currentTempC;
    // Close all open connections
    asteraI2CCloseConnection(ariesHandle);

    if (ariesDevice) {
        free(ariesDevice);
    }

    if (i2cDriver) {
        free(i2cDriver);
    }

    return ARIES_SUCCESS;
}

AriesErrorType AriesMargin(int id, const char* type, const char* fname)
{
    AriesDeviceType* ariesDevice;
    AriesI2CDriverType* i2cDriver;
    AriesErrorType rc;
    int ariesHandle;
    int i2cBus = RT_DEV_INFO[id].bus;
    int ariesSlaveAddress = RT_DEV_INFO[id].addr;
    AriesDevicePartType partNumber = ARIES_PTX16;

    asteraLogSetLevel(2);

    // Open connection to Aries Retimer
    ariesHandle = asteraI2COpenConnection(i2cBus, ariesSlaveAddress);

    i2cDriver = (AriesI2CDriverType*)malloc(sizeof(AriesI2CDriverType));
    if (i2cDriver == NULL) {
        return ARIES_OUT_OF_MEMORY;
    }

    i2cDriver->handle = ariesHandle;
    i2cDriver->slaveAddr = ariesSlaveAddress;
    i2cDriver->pecEnable = ARIES_I2C_PEC_DISABLE;
    i2cDriver->i2cFormat = ARIES_I2C_FORMAT_ASTERA;
    i2cDriver->lockInit = 0;

    ariesDevice = (AriesDeviceType*)malloc(sizeof(AriesDeviceType));
    if (ariesDevice == NULL) {
        return ARIES_OUT_OF_MEMORY;
    }

    ariesDevice->i2cDriver = i2cDriver;
    ariesDevice->i2cBus = i2cBus;
    ariesDevice->partNumber = partNumber;

    rc = ariesInitDevice(ariesDevice, ariesSlaveAddress);
    if (rc != ARIES_SUCCESS)
    {
      ASTERA_ERROR("Init device failed");
      return rc;
    }

    // Get device orientation
    int o;
    rc = ariesGetPortOrientation(ariesDevice, &o);
    CHECK_SUCCESS(rc);

    AriesOrientationType orientation;
    if (o == 0)
    {
      orientation = ARIES_ORIENTATION_NORMAL;
    }
    else if (o == 1)
    {
      orientation = ARIES_ORIENTATION_REVERSED;
    }
    else
    {
      ASTERA_ERROR("Could not determine port orientation");
      return ARIES_INVALID_ARGUMENT;
    }

    uint8_t* ec1 = (uint8_t*)malloc(sizeof(uint8_t) * 16);
    uint8_t* ec2 = (uint8_t*)malloc(sizeof(uint8_t) * 16);
    uint8_t** ec = (uint8_t**)malloc(sizeof(uint8_t*) * 2);
    ec[0] = ec1;
    ec[1] = ec2;

    // Initialize our margin device. This will store important information about
    // the device we will margin
    AriesRxMarginType* marginDevice = (AriesRxMarginType*)malloc(
       sizeof(AriesRxMarginType));
    if (marginDevice == NULL) {
      return ARIES_OUT_OF_MEMORY;
    }

    marginDevice->device = ariesDevice;
    marginDevice->partNumber = partNumber;
    marginDevice->orientation = orientation;
    marginDevice->do1XAnd0XCapture = true;
    marginDevice->errorCountLimit = 4;
    marginDevice->errorCount = ec;

    int width;
    if (marginDevice->partNumber == ARIES_PTX16)
    {
      width = 16;
    }
    else if (marginDevice->partNumber == ARIES_PTX08)
    {
      width = 8;
    }

    // Initialize our eyeResults array that we will store the results from
    // ariesLogEye() in (the results will also be saved to a file)
    double*** eyeResults = (double***)malloc(sizeof(double**) * 2);
    if (eyeResults == NULL) {
        return ARIES_OUT_OF_MEMORY;
    }

    int i = 0;
    for (i = 0; i < 2; i++)
    {
      eyeResults[i] = (double**)malloc(sizeof(double*) * width);
      int j;
      for (j = 0; j < width; j++)
      {
        eyeResults[i][j] = (double*)malloc(sizeof(double) * 5);
      }
    }

    // Run the ariesLogEye method to check the eye stats for all the lanes on
    // the Up stream pseudo port on this device and save them to a document
    if (!strcmp(type, "up")) {
      ariesLogEye(marginDevice, ARIES_UP_STREAM_PSEUDO_PORT, width, fname,
             0, 0.5, eyeResults);
    }
    else if (!strcmp(type, "down")) {
      ariesLogEye(marginDevice, ARIES_DOWN_STREAM_PSEUDO_PORT, width, fname,
             0, 0.5, eyeResults);
    }
    else {
      ASTERA_ERROR("Unknown type");
    }

    asteraI2CCloseConnection(ariesHandle);

    if (ec1) {
        free(ec1);
    }

    if (ec2) {
        free(ec2);
    }

    if (ec) {
        free(ec);
    }

    if (eyeResults) {
        free(eyeResults);
    }

    if (marginDevice) {
        free(marginDevice);
    }

    if (ariesDevice) {
        free(ariesDevice);
    }

    if (i2cDriver) {
        free(i2cDriver);
    }

    return ARIES_SUCCESS;
}

#ifdef __cplusplus
}
#endif
