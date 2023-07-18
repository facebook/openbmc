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

#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <poll.h>

#define MAX_DATA_SIZE 3000
#define SUPPORTED_JTAG_CHAINS 2
#define SUPPORTED_I2C_BUSES 1
#define HEADER_SIZE 4
#define MAX_PACKET_SIZE (MAX_DATA_SIZE + HEADER_SIZE)
#define BUFFER_SIZE 256
#define MAX_REG_NAME 256
#define MAX_WAIT_CYCLES 256
#define BROADCAST_MESSAGE_ORIGIN_ID 7
#define MAX_FIELD_NAME_SIZE 40
// Two simple rules for the version string are:
// 1. less than 255 in length (or it will be truncated in the plugin)
// 2. no dashes, as they are used later up the sw stack between components.
static char asd_version[] = "ASD_BMC_v1.5.1";

#define TO_ENUM(ENUM) ENUM,
#define TO_STRING(STRING) #STRING,

// ASD Protocol Event IDs
typedef enum
{
    ASD_EVENT_PLRSTASSERT = 1,
    ASD_EVENT_PLRSTDEASSRT,
    ASD_EVENT_PRDY_EVENT,
    ASD_EVENT_PWRRESTORE,
    ASD_EVENT_PWRFAIL,
    ASD_EVENT_XDP_PRESENT,
    ASD_EVENT_MBP,
    ASD_EVENT_PWRRESTORE2,
    ASD_EVENT_PWRFAIL2,
    ASD_EVENT_PWRRESTORE3,
    ASD_EVENT_PWRFAIL3,
    ASD_EVENT_NONE
} ASD_EVENT;

typedef enum
{
    ST_OK,
    ST_ERR,
    ST_TIMEOUT
} STATUS;

typedef enum
{
    JTAG_FREQ = 1, // Size: 1 Byte
    DR_PREFIX,     // Size: 1 Byte
    DR_POSTFIX,    // Size: 1 Byte
    IR_PREFIX,     // Size: 2 Bytes, LSB first
    IR_POSTFIX,    // Size: 2 Bytes, LSB first
    PRDY_TIMEOUT   // Size: 1 Byte
} writeCfg;

typedef enum
{
    AGENT_CONTROL_TYPE = 0,
    JTAG_TYPE,
    PROBE_MODE_TYPE,
    DMA_TYPE,
    HARDWARE_LOG_EVENT = 5,
    I2C_TYPE
} headerType;

//// These values are based on the Interface.Pins definition in the OpenIPC
/// config xml and the 5.1.4 of the At-Scale-Debug spec.
typedef enum
{
    PIN_MIN = -1,
    PIN_PWRGOOD = 0,
    PIN_PREQ,
    PIN_RESET_BUTTON,
    PIN_POWER_BUTTON,
    PIN_EARLY_BOOT_STALL,
    PIN_SYS_PWR_OK,
    PIN_PRDY,
    PIN_TCK_MUX_SELECT,
    PIN_MAX // Insert before WRITE_PIN_MAX
} Pin;

// AGENT_CONTROL_TYPE commands
#define NUM_IN_FLIGHT_MESSAGES_SUPPORTED_CMD 3
#define OBTAIN_DOWNSTREAM_VERSION_CMD 5
#define AGENT_CONFIGURATION_CMD 8
#define MAX_DATA_SIZE_CMD 13
#define SUPPORTED_JTAG_CHAINS_CMD 14
#define SUPPORTED_I2C_BUSES_CMD 15
#define SUPPORTED_REMOTE_PROBES_CMD 16
#define REMOTE_PROBES_CONFIG_CMD 17
#define LOOPBACK_CMD 18

// AGENT_CONFIGURATION_CMD types
#define AGENT_CONFIG_TYPE_LOGGING 1
#define AGENT_CONFIG_TYPE_GPIO 2
#define AGENT_CONFIG_TYPE_JTAG_SETTINGS 3

// This mask is used for extracting jtag driver mode from a command byte
#define JTAG_DRIVER_MODE_MASK 0x01

// This mask is used for extracting chain select mode from a command byte
#define JTAG_CHAIN_SELECT_MODE_MASK 0x02

typedef enum
{
    JTAG_CHAIN_SELECT_MODE_SINGLE = 1,
    JTAG_CHAIN_SELECT_MODE_MULTI = 2
} JTAG_CHAIN_SELECT_MODE;

// Supported JTAG Scan Chains
typedef enum
{
    SCAN_CHAIN_0 = 0,
    SCAN_CHAIN_1,
    MAX_SCAN_CHAINS
} scanChain;

// The protocol spec states the following:
//    the status code that either contains 0x00 for successful completion
//    of a command, or an error code from 0x01 to 0x7f
typedef enum
{
    ASD_SUCCESS = 0,
    ASD_MSG_CRYPY_NOT_SUPPORTED = 0x25,
    ASD_FAILURE_INIT_JTAG_HANDLER = 0x26,
    ASD_FAILURE_INIT_I2C_HANDLER = 0x27,
    ASD_FAILURE_DEINIT_JTAG_HANDLER = 0x28,
    ASD_MSG_NOT_SUPPORTED = 0x29,
    ASD_FAILURE_PROCESS_JTAG_MSG = 0x2A,
    ASD_FAILURE_PROCESS_I2C_MSG = 0x2B,
    ASD_FAILURE_PROCESS_I2C_LOCK = 0x2C,
    ASD_I2C_MSG_NOT_SUPPORTED = 0x2D,
    ASD_FAILURE_REMOVE_I2C_LOCK = 0x2E,
    ASD_FAILURE_HEADER_SIZE = 0x2F,
    ASD_FAILURE_XDP_PRESENT = 0x30,
    ASD_FAILURE_INIT_VPROBE_HANDLER = 0x31,
    ASD_UNKNOWN_ERROR = 0x7F,
    ASD_PACKET_CONTINUATION = 0x80,
} ASDError;

struct message_header
{
    uint8_t origin_id : 3;
    uint8_t reserved : 1;
    uint8_t enc_bit : 1;
    uint8_t type : 3;
    uint8_t size_lsb : 8;
    uint8_t size_msb : 5;
    uint8_t tag : 3;
    uint8_t cmd_stat : 8;
} __attribute__((packed));

struct asd_message
{
    struct message_header header;
    unsigned char buffer[MAX_DATA_SIZE];
} __attribute__((packed));

typedef enum
{
    IPC_LogType_MIN = -1,
    IPC_LogType_Trace = 0,
    IPC_LogType_Debug,
    IPC_LogType_Info,
    IPC_LogType_Warning,
    IPC_LogType_Error,
    IPC_LogType_Off,
    IPC_LogType_MAX,
} IPC_LogType;

typedef struct remote_logging_config
{
    union
    {
        struct
        {
            uint8_t logging_level : 3;
            uint8_t logging_stream : 3;
        };
        unsigned char value;
    };
} __attribute__((packed)) remote_logging_config;

#define MAX_IxC_BUSES 4

#define ALL_BUS_CONFIG_TYPES(FUNC)                                             \
    FUNC(BUS_CONFIG_NOT_ALLOWED)                                               \
    FUNC(BUS_CONFIG_I2C)                                                       \
    FUNC(BUS_CONFIG_I3C)

typedef enum
{
    ALL_BUS_CONFIG_TYPES(TO_ENUM)
} bus_config_type;

static const char* BUS_CONFIG_TYPE_STRINGS[] = {
    ALL_BUS_CONFIG_TYPES(TO_STRING)};

typedef struct bus_options
{
    bool enable_i2c;
    bool enable_i3c;
    uint8_t bus_config_map[MAX_IxC_BUSES];
    bus_config_type bus_config_type[MAX_IxC_BUSES];
    uint8_t bus;
} bus_options;

//  At Scale Debug Commands
//
//  NAME            COMMAND MASK    NOTES
//
//  WriteEventCfg   00000000
#define WRITE_EVENT_CONFIG 0

//  WriteCfg        00000nnn        n defines the config setting
//                                  Note: Conflicts with WritePins
//                                  and WriteEventCfg!
#define WRITE_CFG_MIN 1 // +1 because of WriteEventCfg conflict
#define WRITE_CFG_MAX 6 // -1 because of WritePins conflict

// This mask is used to extract the write event index from a command byte
#define WRITE_CFG_MASK 0x7f

//  WritePins       00000111
#define WRITE_PINS 7

// This mask is used to extract the write pin from a command byte
#define WRITE_PIN_MASK 0x7f

#define SCAN_CHAIN_SELECT 0x40
#define SCAN_CHAIN_SELECT_MASK 0xf

//  ReadStatus      00001nnn        n defines the status register
#define READ_STATUS_MIN 8
#define READ_STATUS_MAX 0xf

// This mask is used to extract the read config index from a command byte
#define READ_STATUS_MASK 0x7
#define READ_STATUS_PIN_MASK 0x7f

//  WaitCycles      0001000e        If e is set to 1 enables TCK
//                                  during the wait period
#define WAIT_CYCLES_TCK_DISABLE 0x10
#define WAIT_CYCLES_TCK_ENABLE 0x11

//  WaitPRDY        00010010
#define WAIT_PRDY 0x12

//  ClearTimeout    00010011
#define CLEAR_TIMEOUT 0x13

//  TapReset        00010100
#define TAP_RESET 0x14

//  WaitSync        00011001
#define WAIT_SYNC 0x19
#define WAIT_SYNC_CMD_LENGTH 4

//  TapState        0010SSSS        S defines desired state
#define TAP_STATE_MIN 0x20
#define TAP_STATE_MAX 0x2f

//  This mask is used for extracting the desired tap state from a command
#define TAP_STATE_MASK 0xf

//  WriteScan       01LLLLLL        L is number of bits
#define WRITE_SCAN_MIN 0x40
#define WRITE_SCAN_MAX 0x7f

//  ReadScan        10LLLLLL        L is number of bits
#define READ_SCAN_MIN 0x80
#define READ_SCAN_MAX 0xbf

//  RWScan          11LLLLLL        L is number of bits
#define READ_WRITE_SCAN_MIN 0xc0
#define READ_WRITE_SCAN_MAX 0xff

//  This mask is used for extracting the length for all the scan commands
#define SCAN_LENGTH_MASK 0x3f

#define ASD_I2C_BUFFER_LEN 15
typedef struct asd_i2c_msg
{
    bool read;
    bool force_stop;
    uint8_t address;
    uint8_t length;
    uint8_t buffer[ASD_I2C_BUFFER_LEN];
} asd_i2c_msg;

//  At Scale Debug I2C Commands
//
//  NAME            COMMAND MASK    NOTES
//  WriteCfg        00000nnn        n defines the config setting
#define I2C_WRITE_CFG_BUS_SELECT 1
#define I2C_WRITE_CFG_SCLK 2

//  I2CRead         001CLLLL        L is number of bits
//                                  C is the continue bit
#define I2C_READ_MIN 0x20
#define I2C_READ_MAX 0x3f

#define I2C_LENGTH_MASK 0xf
#define I2C_CONTINUE_BIT_MASK 0x10
#define I2C_ADDRESS_MASK 0xfe
#define I2C_FORCE_STOP_MASK 0x1
#define I2C_ADDRESS_ACK 0x80

//  I2CWrite        011CLLLL        L is number of bits
//                                  C is the continue bit
#define I2C_WRITE_MIN 0x60
#define I2C_WRITE_MAX 0x7f

#define NUM_GPIOS 14
#define NUM_DBUS_FDS 1
typedef struct pollfd target_fdarr_t[NUM_GPIOS + NUM_DBUS_FDS];

#endif // COMMON_H
