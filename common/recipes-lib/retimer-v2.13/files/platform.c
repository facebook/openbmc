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

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_RETIMERS 1
#define NUM_LINKS_PER_RETIMER 1
#define NUM_TOTAL_LINKS NUM_RETIMERS * NUM_LINKS_PER_RETIMER

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
      fprintf(stderr, "Error: Could not open file "
                    "`/dev/i2c-%d' or `/dev/i2c/%d': %s\n",
                    i2cBus, i2cBus, strerror(ENOENT));
    }
    else
    {
      fprintf(stderr, "Error: Could not open file "
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

AriesErrorType SetupAriesDevice (AriesDeviceType* ariesDevice,
                AriesI2CDriverType* i2cDriver, int bus, int addr) {
  AriesErrorType rc;

  i2cDriver->handle = asteraI2COpenConnection(bus, addr);
  if (i2cDriver->handle < 0) {
    return ARIES_I2C_OPEN_FAILURE;
  }

  i2cDriver->slaveAddr = addr;
  i2cDriver->pecEnable = ARIES_I2C_PEC_DISABLE;
  i2cDriver->i2cFormat = ARIES_I2C_FORMAT_ASTERA;
  i2cDriver->lockInit = 0;

  ariesDevice->i2cDriver = i2cDriver;
  ariesDevice->i2cBus = bus;
  ariesDevice->partNumber = ARIES_PTX16;
  ariesDevice->tempCalCodeAvg = 84;

  ariesDevice->tempAlertThreshC = 110.0;
  ariesDevice->tempWarnThreshC  = 100.0;
  ariesDevice->minLinkFoMAlert  =  0x55;

  rc = ariesInitDevice(ariesDevice, addr);
  if (rc != ARIES_SUCCESS) {
    CHECK_SUCCESS(rc);
    ASTERA_ERROR("Init device failed");
  }

  asteraI2CCloseConnection(i2cDriver->handle);
  return rc;
}

AriesErrorType AriestFwUpdate(int bus, int addr, const char* fp)
{
  AriesDeviceType ariesDevice;
  AriesI2CDriverType i2cDriver;
  AriesErrorType rc;

  asteraLogSetLevel(2);

  rc = SetupAriesDevice(&ariesDevice, &i2cDriver, bus, addr);
  if (rc != ARIES_SUCCESS) {
    CHECK_SUCCESS(rc);
    return rc;
  }

  i2cDriver.handle = asteraI2COpenConnection(bus, addr);
  if (i2cDriver.handle < 0) {
    return ARIES_I2C_OPEN_FAILURE;
  }

  rc = ariesUpdateFirmware(&ariesDevice, fp, ARIES_FW_IMAGE_FORMAT_IHX);
  if (rc != ARIES_SUCCESS) {
    ASTERA_ERROR("Failed to update the firmware image. RC = %d", rc);
  }

  // Reboot device to check if FW version was applied
  // Assert HW reset
  ASTERA_INFO("Performing Retimer HW reset ...");
  rc = ariesSetHwReset(&ariesDevice, 1);
  CHECK_SUCCESS(rc);
  // Wait 10 ms before de-asserting
  usleep(10000);
  // De-assert HW reset
  rc = ariesSetHwReset(&ariesDevice, 0);
  CHECK_SUCCESS(rc);
  usleep(5000000);

  rc = ariesInitDevice(&ariesDevice, addr);
  if (rc != ARIES_SUCCESS)
  {
    CHECK_SUCCESS(rc);
    ASTERA_ERROR("Init device failed");
    return rc;
  }

  ASTERA_INFO("Updated FW Version is %d.%d.%d", ariesDevice.fwVersion.major,
                ariesDevice.fwVersion.minor, ariesDevice.fwVersion.build);

  asteraI2CCloseConnection(i2cDriver.handle);
  return rc;
}

AriesErrorType AriesGetFwVersion(int bus, int addr, uint16_t* ver)
{
  AriesDeviceType ariesDevice;
  AriesI2CDriverType i2cDriver;
  AriesErrorType rc;

  asteraLogSetLevel(2);

  rc = SetupAriesDevice(&ariesDevice, &i2cDriver, bus, addr);
  if (rc != ARIES_SUCCESS) {
    CHECK_SUCCESS(rc);
    return rc;
  }

  ver[0] = ariesDevice.fwVersion.major;
  ver[1] = ariesDevice.fwVersion.minor;
  ver[2] = ariesDevice.fwVersion.build;

  return rc;
}

AriesErrorType AriesGetTemp(AriesDeviceType* ariesDevice,
        AriesI2CDriverType* i2cDriver, int bus, int addr, float* temp)
{
  AriesErrorType rc;

  asteraLogSetLevel(2);

  i2cDriver->handle = asteraI2COpenConnection(bus, addr);
  if (i2cDriver->handle < 0) {
    return ARIES_I2C_OPEN_FAILURE;
  }

  rc = ariesGetCurrentTemp(ariesDevice);
  CHECK_SUCCESS(rc);

  *temp = ariesDevice->currentTempC;

  asteraI2CCloseConnection(i2cDriver->handle);
  return rc;
}

AriesErrorType AriesMargin(int id, const char* type, const char* fname)
{
  AriesDeviceType ariesDevice;
  AriesI2CDriverType i2cDriver;
  AriesErrorType rc;
  int bus = RT_DEV_INFO[id].bus ;
  int addr = RT_DEV_INFO[id].addr;

  asteraLogSetLevel(2);

  rc = SetupAriesDevice(&ariesDevice, &i2cDriver, bus, addr);
  if (rc != ARIES_SUCCESS) {
    CHECK_SUCCESS(rc);
    return rc;
  }

  i2cDriver.handle = asteraI2COpenConnection(bus, addr);
  if (i2cDriver.handle < 0) {
    return ARIES_I2C_OPEN_FAILURE;
  }

  // Get device orientation
  int o;
  rc = ariesGetPortOrientation(&ariesDevice, &o);
  CHECK_SUCCESS(rc);

  AriesOrientationType orientation;
  if (o == 0) {
    orientation = ARIES_ORIENTATION_NORMAL;
  }
  else if (o == 1) {
    orientation = ARIES_ORIENTATION_REVERSED;
  }
  else {
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

  marginDevice->device = &ariesDevice;
  marginDevice->partNumber = ariesDevice.partNumber;
  marginDevice->orientation = orientation;
  marginDevice->do1XAnd0XCapture = true;
  marginDevice->errorCountLimit = 4;
  marginDevice->errorCount = ec;

  int width;
  if (marginDevice->partNumber == ARIES_PTX16) {
    width = 16;
  }
  else if (marginDevice->partNumber == ARIES_PTX08) {
    width = 8;
  }

  // Initialize our eyeResults array that we will store the results from
  // ariesLogEye() in (the results will also be saved to a file)
  double*** eyeResults = (double***)malloc(sizeof(double**) * 2);
  if (eyeResults == NULL) {
    return ARIES_OUT_OF_MEMORY;
  }

  for (int i = 0; i < 2; i++) {
    eyeResults[i] = (double**)malloc(sizeof(double*) * width);

    for (int j = 0; j < width; j++) {
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

  asteraI2CCloseConnection(i2cDriver.handle);
  return ARIES_SUCCESS;
}

#ifdef __cplusplus
}
#endif
