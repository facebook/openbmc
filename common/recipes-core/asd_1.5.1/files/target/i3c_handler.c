/*
Copyright (c) 2021, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "i3c_handler.h"

// Disabling clang-format to avoid include alphabetical re-order that leads
// into a conflict for i2c-dev.h that requires std headers to be added before.

// clang-format off
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
//#include <uapi/linux/i3c/i3cdev.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/file.h>
// clang-format on

#include "logging.h"

#define I3C_DEV_FILE_NAME "/dev/i3c"
#define I3C_MASTER_DRV_FILE_NAME "/sys/bus/platform/drivers/dw-i3c-master"
#define MAX_I3C_DEV_FILENAME 256
#define I3C_BUS_ADDRESS_RESERVED 127

struct i3c_ioc_priv_xfer {
    __u64 data;
    __u16 len;
    __u8 rnw;
    __u8 pad[5];
};

#define I3C_DEV_IOC_MAGIC 0x07
#define I3C_PRIV_XFER_SIZE(N) \
    ((((sizeof(struct i3c_ioc_priv_xfer)) * (N)) < (1 << _IOC_SIZEBITS)) \
    ? ((sizeof(struct i3c_ioc_priv_xfer)) * (N)) : 0)
#define I3C_IOC_PRIV_XFER(N) \
    _IOC(_IOC_READ|_IOC_WRITE, I3C_DEV_IOC_MAGIC, 30, I3C_PRIV_XFER_SIZE(N))

static const ASD_LogStream stream = ASD_LogStream_I2C;
static const ASD_LogOption option = ASD_LogOption_None;

static bool i3c_enabled(I3C_Handler* state);
static bool i3c_device_drivers_opened(I3C_Handler* state);
static STATUS i3c_open_device_drivers(I3C_Handler* state, uint8_t bus);
static void i3c_close_device_drivers(I3C_Handler* state);
static bool i3c_bus_allowed(I3C_Handler* state, uint8_t bus);
static STATUS i3c_get_dev_name(I3C_Handler* state, uint8_t bus, uint8_t* dev);

#define AST2600_I3C_BUSES 4
const char* i3c_bus_names[AST2600_I3C_BUSES] = {
    "1e7a2000.i3c0", "1e7a3000.i3c1", "1e7a4000.i3c2", "1e7a5000.i3c3"};

I3C_Handler* I3CHandler(bus_config* config)
{
    if (config == NULL)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Invalid config parameter.");
        return NULL;
    }

    I3C_Handler* state = (I3C_Handler*)malloc(sizeof(I3C_Handler));
    if (state == NULL)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to malloc I3C_Handler.");
    }
    else
    {
        for (int i = 0; i < i3C_MAX_DEV_HANDLERS; i++)
            state->i3c_driver_handlers[i] = UNINITIALIZED_I3C_DRIVER_HANDLE;
        state->config = config;
        state->bus_token = UNINITIALIZED_I3C_BUS_TOKEN;
#ifdef HAVE_DBUS
        state->dbus = dbus_helper();
        if (state->dbus == NULL)
        {
            free(state);
            state = NULL;
        }
#endif
    }

    return state;
}

STATUS i3c_initialize(I3C_Handler* state)
{
    STATUS status = ST_ERR;
    if (state != NULL && i3c_enabled(state))
    {
#ifdef HAVE_DBUS
        status = dbus_initialize(state->dbus);
        if (status == ST_OK)
        {
            state->dbus->fd = sd_bus_get_fd(state->dbus->bus);
            if (state->dbus->fd < 0)
            {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "sd_bus_get_fd failed");
                status = ST_ERR;
            }
        }
        else
        {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "Failed to init i3c dbus handler");
        }
#endif
        state->i3c_bus = I3C_BUS_ADDRESS_RESERVED;
    }
    return status;
}

STATUS i3c_deinitialize(I3C_Handler* state)
{
    STATUS status = ST_OK;

    if (state == NULL)
        return ST_ERR;

    i3c_close_device_drivers(state);

#ifdef HAVE_DBUS
    // Release I3C BMC ownership at ASD exit
    if (state->bus_token != UNINITIALIZED_I3C_BUS_TOKEN)
    {
        status = dbus_rel_i3c_ownership(state->dbus, state->bus_token);
        if (status == ST_OK)
        {
            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C, ASD_LogOption_None,
                    "Release BMC i3c bus ownership succeed");

            state->bus_token = UNINITIALIZED_I3C_BUS_TOKEN;
        }
        else
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                    "Release BMC i3c bus ownership failed");
        }
    }

    if (state->dbus != NULL)
    {
        dbus_deinitialize(state->dbus);
        free(state->dbus);
        state->dbus = NULL;
    }
#endif
    //state = NULL;
    return status;
}

STATUS i3c_bus_flock_dev_handlers(I3C_Handler* state, uint8_t bus, int op)
{
    STATUS status = ST_OK;
    ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C, ASD_LogOption_None,
            "i3c - bus %d %s", bus, op == LOCK_EX ? "LOCK" : "UNLOCK");

    if (state == NULL)
        return ST_ERR;

    if (state->i3c_bus == I3C_BUS_ADDRESS_RESERVED)
    {
        status = i3c_bus_select(state, bus);
    }
    if (status == ST_OK)
    {
        for (int i = 0; i < i3C_MAX_DEV_HANDLERS; i++)
        {
            if (state->i3c_driver_handlers[i] ==
                UNINITIALIZED_I3C_DRIVER_HANDLE)
                continue;

            if (flock(state->i3c_driver_handlers[i], op) != 0)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C,
                        ASD_LogOption_None,
                        "i3c flock for bus %d failed dev %x handler = 0x%x",
                        bus, i, state->i3c_driver_handlers[i]);
                status = ST_ERR;
            }
        }
    }
    return status;
}

STATUS i3c_bus_flock(I3C_Handler* state, uint8_t bus, int op)
{
    STATUS status = ST_OK;
#ifdef HAVE_DBUS
    I3c_Ownership owner = CPU_OWNER;
#endif

    ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C, ASD_LogOption_None,
            "i3c - bus %d %s", bus, op == LOCK_EX ? "LOCK" : "UNLOCK");

#ifdef HAVE_DBUS
    if (state == NULL || state->dbus == NULL)
        return ST_ERR;

    // Request I3C BMC ownership on the first xfer attempt
    if (state->bus_token == UNINITIALIZED_I3C_BUS_TOKEN)
    {
        ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C, ASD_LogOption_None,
                "Request i3c bus ownership");

        status = dbus_req_i3c_ownership(state->dbus, &state->bus_token);
        if (status == ST_OK)
        {
            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C, ASD_LogOption_None,
                    "Request i3c bus ownership succeed token: %d",
                    state->bus_token);
            // Wait 1s for system to bind the driver and create dev handlers
            usleep(1000000);
            status = i3c_open_device_drivers(state, bus);
            if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C,
                        ASD_LogOption_None, "Open i3c device drivers failed");
                status = dbus_rel_i3c_ownership(state->dbus, state->bus_token);
                if (status == ST_OK)
                {
                    ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C,
                            ASD_LogOption_None,
                            "Release BMC i3c bus ownership succeed");
                }
                state->bus_token = UNINITIALIZED_I3C_BUS_TOKEN;
                return ST_ERR;
            }
        }
        else
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                    "Request i3c bus ownership failed");
            return status;
        }
    }

    // Read configuration from dbus
    status = dbus_read_i3c_ownership(state->dbus, &owner);
    if (status == ST_OK)
    {
        ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C, ASD_LogOption_None,
                "i3c ownership %s", owner == CPU_OWNER ? "CPU" : "BMC");
        if (owner == BMC_OWNER)
        {
#endif
            status = i3c_bus_flock_dev_handlers(state, bus, op);
            if (status != ST_OK)
            {
                // If lock fail, unlock all files
                if (op == LOCK_EX)
                {
                    i3c_bus_flock_dev_handlers(state, bus, LOCK_UN);
                }
            }
#ifdef HAVE_DBUS
        }
        else
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                    "BMC does not have i3c bus ownership");
            return ST_ERR;
        }
    }
    else
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                "Fail to read i3c bus ownership from dbus");
    }
#endif
    return status;
}

STATUS i3c_bus_select(I3C_Handler* state, uint8_t bus)
{
    STATUS status = ST_ERR;
    if (state != NULL && i3c_enabled(state))
    {
        if (bus == state->i3c_bus)
        {
            if (i3c_device_drivers_opened(state))
            {
                status = ST_OK;
            }
            else
            {
                ASD_log(ASD_LogLevel_Error, stream, option, "Selecting Bus %d",
                        bus);
                status = i3c_open_device_drivers(state, bus);
            }
        }
        else if (i3c_bus_allowed(state, bus))
        {
            i3c_close_device_drivers(state);
            ASD_log(ASD_LogLevel_Error, stream, option, "Selecting Bus %d",
                    bus);
            status = i3c_open_device_drivers(state, bus);
            if (status == ST_OK)
            {
                state->config->default_bus = bus;
            }
        }
        else
        {
            ASD_log(ASD_LogLevel_Error, stream, option, "Bus %d not allowed",
                    bus);
        }
    }
    return status;
}

STATUS i3c_set_sclk(I3C_Handler* state, uint16_t sclk)
{
    if (state == NULL || !i3c_enabled(state))
        return ST_ERR;
    return ST_OK;
}

STATUS i3c_read_write(I3C_Handler* state, void* msg_set)
{
    STATUS status = ST_OK;
    if (state == NULL || msg_set == NULL || !i3c_enabled(state))
        return ST_ERR;

    // Convert i2c packet to i3c request format
    struct i2c_rdwr_ioctl_data* ioctl_data = msg_set;
    struct i3c_ioc_priv_xfer* xfers;
    int handle = UNINITIALIZED_I3C_DRIVER_HANDLE;
    int addr = UNINITIALIZED_I3C_DRIVER_HANDLE;
    xfers =
        (struct i3c_ioc_priv_xfer*)calloc(ioctl_data->nmsgs, sizeof(*xfers));

    if (xfers == NULL)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "I3C_RDWR memory allocation failed");
        return ST_ERR;
    }

    for (int i = 0; i < ioctl_data->nmsgs; i++)
    {
        xfers[i].len = ioctl_data->msgs[i].len;
        xfers[i].data = (__u64)(uintptr_t)ioctl_data->msgs[i].buf;
        xfers[i].rnw = (ioctl_data->msgs[i].flags & I2C_M_RD) ? 1 : 0;
        if (handle == UNINITIALIZED_I3C_DRIVER_HANDLE)
        {
            addr = ioctl_data->msgs[i].addr;
            if (addr >= 0 && addr < i3C_MAX_DEV_HANDLERS)
            {
                handle = state->i3c_driver_handlers[addr];
                ASD_log(ASD_LogLevel_Debug, stream, option,
                        "I3C_RDWR ioctl addr 0x%x handle %d len %d rnw %d",
                        addr, handle, xfers[i].len, xfers[i].rnw);
                ASD_log_buffer(ASD_LogLevel_Debug, stream, option,
                               (const unsigned char *)(uintptr_t)xfers[i].data, xfers[i].len, "I3cBuf");
            }
            else
            {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "I3C_RDWR wrong addr %d", addr);
            }
        }
    }

    if (handle != UNINITIALIZED_I3C_DRIVER_HANDLE)
    {
        int ret = ioctl(handle, I3C_IOC_PRIV_XFER(ioctl_data->nmsgs), xfers);
        if (ret < 0)
        {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "I3C_RDWR ioctl returned %d - %d - %s", ret, errno,
                    strerror(errno));
            status = ST_ERR;
        }
    }
    else
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "I3C_RDWR invalid handle for addr %x", addr);
        status = ST_ERR;
    }

    if (xfers)
        free(xfers);

    return status;
}

static bool i3c_enabled(I3C_Handler* state)
{
    return state->config->enable_i3c;
}

static bool i3c_device_drivers_opened(I3C_Handler* state)
{
    for (int i = 0; i < i3C_MAX_DEV_HANDLERS; i++)
    {
        if (state->i3c_driver_handlers[i] != UNINITIALIZED_I3C_DRIVER_HANDLE)
        {
            return true;
        }
    }
    return false;
}

static STATUS i3c_open_device_drivers(I3C_Handler* state, uint8_t bus)
{
    STATUS status = ST_ERR;
    char i3c_dev_name[MAX_I3C_DEV_FILENAME];
    char i3c_dev[MAX_I3C_DEV_FILENAME*2];

    status = i3c_get_dev_name(state, bus, i3c_dev_name);
    if (status != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Could not find i3c bus %d dev name", bus);
        return status;
    }

    status = ST_ERR;
    for (int i = 0; i < i3C_MAX_DEV_HANDLERS; i++)
    {
        snprintf(i3c_dev, sizeof(i3c_dev), "/dev/%s-3c00000000%x", i3c_dev_name,
                 i);
        state->i3c_driver_handlers[i] = open(i3c_dev, O_RDWR);
        if (state->i3c_driver_handlers[i] != UNINITIALIZED_I3C_DRIVER_HANDLE)
        {
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "open device driver %s for bus %d handle %d", i3c_dev, bus,
                    state->i3c_driver_handlers[i]);
            status = ST_OK;
        }
        else
        {
            ASD_log(ASD_LogLevel_Debug, stream, option, "Can't open %s",
                    i3c_dev);
        }
    }

    state->i3c_bus = bus;

    return status;
}

static void i3c_close_device_drivers(I3C_Handler* state)
{
    for (int i = 0; i < i3C_MAX_DEV_HANDLERS; i++)
    {
        if (state->i3c_driver_handlers[i] != UNINITIALIZED_I3C_DRIVER_HANDLE)
        {
            close(state->i3c_driver_handlers[i]);
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "closing dev handler %x", i);
            state->i3c_driver_handlers[i] = UNINITIALIZED_I3C_DRIVER_HANDLE;
        }
    }
}

static bool i3c_bus_allowed(I3C_Handler* state, uint8_t bus)
{
    if (state == NULL)
        return false;
    for (int i = 0; i < MAX_IxC_BUSES; i++)
    {
        if (state->config->bus_config_map[i] == bus &&
            state->config->bus_config_type[i] == BUS_CONFIG_I3C)
            return true;
    }
    return false;
}

static STATUS i3c_get_dev_name(I3C_Handler* state, uint8_t bus, uint8_t* dev)
{
    STATUS status = ST_ERR;
    char i3c_bus_drv[MAX_I3C_DEV_FILENAME];
    int dev_handle = UNINITIALIZED_I3C_DRIVER_HANDLE;

    if (bus >= AST2600_I3C_BUSES)
    {
        ASD_log(ASD_LogLevel_Error, stream, option, "Unexpected i3c bus");
        return ST_ERR;
    }

    for (int i = 0; i < AST2600_I3C_BUSES; i++)
    {
        snprintf(dev, MAX_I3C_DEV_FILENAME, "i3c-%d", i);
        snprintf(i3c_bus_drv, MAX_I3C_DEV_FILENAME, "%s/%s/i3c-%d",
                 I3C_MASTER_DRV_FILE_NAME, i3c_bus_names[bus], i);

        dev_handle = open(i3c_bus_drv, O_RDONLY);
        if (dev_handle == UNINITIALIZED_I3C_DRIVER_HANDLE)
        {
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "Can't open i3c bus driver %s", i3c_bus_drv);
        }
        else
        {
            ASD_log(ASD_LogLevel_Debug, stream, option, "Found dev %s on %s",
                    dev, i3c_bus_drv);
            close(dev_handle);
            status = ST_OK;
            break;
        }
    }
    return status;
}
