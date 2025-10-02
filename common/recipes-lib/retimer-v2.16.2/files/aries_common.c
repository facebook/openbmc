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
#include "aries_common.h"
#include "platform.h"
#include <pthread.h>
#include "aries_i2c.h"
#include "plat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_RETIMERS 1
#define NUM_LINKS_PER_RETIMER 1
#define NUM_TOTAL_LINKS NUM_RETIMERS * NUM_LINKS_PER_RETIMER

// pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

AriesErrorType SetupAriesDevice (AriesDeviceType* ariesDevice,
                AriesI2CDriverType* i2cDriver) {
  // AriesErrorType rc;

  i2cDriver->handle = asteraI2COpenConnection(bus, addr);
  if (i2cDriver->handle < 0) {
    CHECK_SUCCESS(ARIES_I2C_OPEN_FAILURE);
  }

  i2cDriver->slaveAddr = addr;
  i2cDriver->pecEnable = ARIES_I2C_PEC_DISABLE;
  i2cDriver->i2cFormat = ARIES_I2C_FORMAT_ASTERA;
  i2cDriver->lockInit = 0;

  ariesDevice->i2cDriver = i2cDriver;
  ariesDevice->i2cBus = bus;
  ariesDevice->partNumber = type;

  // rc = ariesInitDevice(ariesDevice, addr);
  // CHECK_ERR_RETURN(rc, i2cDriver->handle);

  return ARIES_SUCCESS;
}

AriesErrorType AriesGetFwVersion(uint16_t* ver)
{
  AriesDeviceType ariesDevice;
  AriesI2CDriverType i2cDriver;
  AriesErrorType rc;

  asteraLogSetLevel(2);
  SetupAriesDevice(&ariesDevice, &i2cDriver);

  rc = ariesFWStatusCheck(&ariesDevice);
  CHECK_ERR_RETURN(rc, i2cDriver.handle);

  asteraI2CCloseConnection(i2cDriver.handle);
  ver[0] = ariesDevice.fwVersion.major;
  ver[1] = ariesDevice.fwVersion.minor;
  ver[2] = ariesDevice.fwVersion.build;

  return ARIES_SUCCESS;
}

AriesErrorType AriesGetHealth(uint8_t *health)
{
  AriesDeviceType ariesDevice;
  AriesI2CDriverType i2cDriver;
  AriesErrorType rc;

  asteraLogSetLevel(2);
  SetupAriesDevice(&ariesDevice, &i2cDriver);

  rc = ariesFWStatusCheck(&ariesDevice);
  CHECK_ERR_RETURN(rc, i2cDriver.handle);

  asteraI2CCloseConnection(i2cDriver.handle);
  *health = (ariesDevice.mmHeartbeatOkay) ? 0x1 : 0x0;
  if (ariesDevice.codeLoadOkay) {
    *health |= 0x1 << 1;
  }

  return rc;
}

AriesErrorType AriesPrintState()
{
    AriesDeviceType ariesDevice;
    AriesI2CDriverType i2cDriver;
    AriesErrorType rc;

    // Enable SDK-level debug prints
    asteraLogSetLevel(2); // ASTERA_INFO type statements (or higher)

    SetupAriesDevice(&ariesDevice, &i2cDriver);
    rc = ariesInitDevice(&ariesDevice, addr);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Init device failed");
        return rc;
    }

    // NOTE: The following parameters are configured and checked by the SDK and
    // do not change the behavior of Firmware running on the Retimer

    // Set temperature thresholds
    // Trigger alert when Temp > 110C
    ariesDevice.tempAlertThreshC = 110.0;
    // Trigger warn when Temp > 100C
    ariesDevice.tempWarnThreshC = 100.0;
    // Trigger warn when link FoM < 0x55
    ariesDevice.minLinkFoMAlert = 0x55;


    // NOTE: In this case, we have two Retimers; since a system may have N Links
    // supported by M Retimers (N>=M), multiple link objects/structs may be used
    // to keep track of the Links' status. Configure Link structs
    AriesLinkType link[ARIES_MAX_NUM_LINKS];
    rc = ariesCreateLinkStructs(&ariesDevice, link);
    CHECK_SUCCESS(rc);

    // NOTE: In previous versions of the SDK, the example app stored all link
    // structs for all retimers in a flat array. While it is recommended to use
    // the nested array structure above, the new ariesCreateLinkStructs API can
    // also support flat arrays like below.

    // // Can use NUM_TOTAL_LINKS instead if it's defined
    // AriesLinkType link[NUM_RETIMERS * ARIES_MAX_NUM_LINKS];
    // int offset = 0;
    // for (i = 0; i < NUM_RETIMERS; i++)
    // {
    //     rc = ariesCreateLinkStructs(ariesDevice[i], &link[i+offset]);
    //     CHECK_SUCCESS(rc);
    //     offset += ariesDevice[i]->numLinks;
    // }

    // -------------------------------------------------------------------------
    // CONTINUOUS MONITORING
    // -------------------------------------------------------------------------
    // This portion of the example shows how Aries Links can be monitored
    // periodically during regular system health checking.
    int j = 0;
    int running = true;
    while (running)
    {
        for (j = 0; j < ariesDevice.numLinks; j++)
        {
            // Get current link state for link[i][j]
            rc = ariesCheckLinkHealth(&link[j]);
            CHECK_SUCCESS(rc);

            // Monitor or Log key parameters, such as
            // Current junction temperature, in C:
            //      link[i][j].device.currentTempC
            // All-time maximum junction temperature, in C:
            //      link[i][j].device.maxTempC
            // Current Link state (ARIES_STATE_FWD is normal L0 state):
            //      link[i][j].state.state
            // Current Link rate (e.g., 5 for Gen5):
            //      link[i][j].state.rate
            // Current Link recovery count, since last read:
            //      link[i][j].state.recoveryCount

            if (link[j].state.recoveryCount > 0)
            {
                ASTERA_WARN("Link%d went into recovery %d times!",
                            j, link[j].state.recoveryCount);
                rc = ariesClearLinkRecoveryCount(&link[j]);
                CHECK_SUCCESS(rc);
            }

            // -----------------------------------------------------------------
            // ERROR-SCENARIO HANDLING
            // -----------------------------------------------------------------
            // Check if the Link is in on expected state, and if not,
            // capture additional debug information. The expected Retimer
            // state during PCIe L0 is the "Forwarding" or "FWD" state.
            if (link[j].state.linkOkay == false)
            {
                ASTERA_ERROR("Unexpected link%d  state: %d", j,
                              link[j].state.state);
                ASTERA_ERROR("Now capturing link stats and logs");

                // Capture detailed debug information from Retimer
                rc = ariesLinkDumpDebugInfo(&link[j]);
                CHECK_SUCCESS(rc);

                // In this example, the continuous monitoring section exits
                // when the Link state is unexpected.
                running = false;
            }
        }

        // Wait 5 seconds before reading state again (this up to the user)
        sleep(5);
    }

    asteraI2CCloseConnection(i2cDriver.handle);
    return 0;
}

AriesErrorType AriesMargin ()
{
    AriesDeviceType ariesDevice;
    AriesI2CDriverType i2cDriver;
    AriesErrorType rc;

    // Enable SDK-level debug prints
    asteraLogSetLevel(2); // ASTERA_INFO type statements (or higher)

    SetupAriesDevice(&ariesDevice, &i2cDriver);
    rc = ariesInitDevice(&ariesDevice, addr);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Init device failed");
        return rc;
    }

    // Print FW version
    ASTERA_INFO("FW Version: %d.%d.%d", ariesDevice.fwVersion.major,
                ariesDevice.fwVersion.minor, ariesDevice.fwVersion.build);

    // Get device orientation
    int o;
    rc = ariesGetPortOrientation(&ariesDevice, &o);

    AriesOrientationType orientation;
    if (o == 0)
    {
        ASTERA_DEBUG("ARIES_ORIENTATION_NORMAL");
        orientation = ARIES_ORIENTATION_NORMAL;
    }
    else if (o == 1)
    {
        ASTERA_DEBUG("ARIES_ORIENTATION_REVERSED");
        orientation = ARIES_ORIENTATION_REVERSED;
    }
    else
    {
        ASTERA_ERROR("Could not determine port orientation");
        return ARIES_INVALID_ARGUMENT;
    }

    // Initialize our margin device. This will store important information about
    // the device we will margin
    AriesRxMarginType marginDevice;
    marginDevice.device = &ariesDevice;
    marginDevice.partNumber = type;
    marginDevice.orientation = orientation;
    marginDevice.do1XAnd0XCapture = true;
    marginDevice.errorCountLimit = 4;

    if (width == 0) {
      // does not set width from platform, decide width by partNumber
      if (marginDevice.partNumber == ARIES_PTX16)
      {
          width = 16;
      }
      else
      {
          width = 8;
      }
    }


    // Run the ariesLogEye method to check the eye stats for all the lanes on
    // the Up stream pseudo port on this device and save them to a document
    ASTERA_INFO("ariesLogEye start");
    ariesLogEye(&marginDevice, ARIES_UP_STREAM_PSEUDO_PORT, width,
                "margin_test", 0, 0.5);
    ASTERA_INFO("ariesLogEye end");

    // Run eyeDiagram method to find the eye for all lanes on the
    // UPSTREAMPSEUDOPORT on this device the results will be saved in our
    // eyeDiagramArr variable and will also be saved to respective files.
    
    /*
      FAE confirm this portion will not be used when FAE analysis the margin result.
      Remove to save execution time.
    int i;
    for (i = 0; i < width; i++)
    {
        ASTERA_INFO("Measuring eye diagram on USPP lane %d", i);
        ariesEyeDiagram(&marginDevice, ARIES_UP_STREAM_PSEUDO_PORT, i, 4, 0.5);
    }
    */

    // Close all open connections
    asteraI2CCloseConnection(i2cDriver.handle);
    return 0;
  }

#ifdef __cplusplus
}
#endif
