#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#include "plat.h"
#include "aries_api_types.h"
#include "aries_error.h"
#include "astera_log.h"

// BIC PLDM Command Prefix
const char* BIC_TxPrefix = "pldmtool raw -m";
const char* BIC_RxPrefix = "pldmtool: Rx: ";

uint8_t slot_id = 1;
AriesDevicePartType type = ARIES_PTX08;
uint8_t bus = 0; // 0'base
uint8_t addr = 0;  // 8 bits AL
uint8_t width = 0;

// Covert hex to int
int hexCharToInt(char c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    return -1; // illegal char
}

// Convert hex string with space to buffer
// ex: "01 02 87 87" → [0x01, 0x02, 0x87, 0x87]
int hexStringToBuffer(const char* hexStr, unsigned char* buffer)
{
    int count = 0;

    while (*hexStr)
    {
        while (*hexStr == ' ')
        {
            hexStr++; // skip space
        }

        if (*hexStr == '\0' || *hexStr == '\n')
        {
            break;
        }

        if (!isxdigit(hexStr[0]) || !isxdigit(hexStr[1]))
        {
            return -1; // illegal format
        }

        int hi = hexCharToInt(hexStr[0]);
        int lo = hexCharToInt(hexStr[1]);
        buffer[count++] = (hi << 4) | lo;
        hexStr += 2;  // Forward two char
        while (*hexStr == ' ')
            hexStr++; // skip space
    }
    return count;
}

// To get PLDM response
uint32_t pldm_get_response(FILE* fp, uint8_t* pbRx, int stat_byte)
{
    char* pIdx;
    int bufLen;

    // Response
    char pb[PLDM_RX_SIZE];

    // Skip TX line
    if (fgets(pb, PLDM_RX_SIZE, fp) == NULL)
    {
        pclose(fp);
        return -1;
    }
    // Read RX line
    if (fgets(pb, PLDM_RX_SIZE, fp) == NULL)
    {
        pclose(fp);
        return -1;
    }
    pclose(fp);

    // Check if pldm RX prefix exist or not
    pIdx = strstr(pb, BIC_RxPrefix);
    if (pIdx == NULL)
    {
        return -1;
    }

    bufLen = hexStringToBuffer(&pb[PLDM_RX_FIRST_SPACE], &pbRx[0]);

    if (bufLen < 0)
    {
        ASTERA_ERROR("String format is incorrect !!");
        return -1;
    }

    // Check pldm status byte
    if (pbRx[stat_byte] != BIC_STS_READY)
    {
        ASTERA_ERROR("pldm sts 0x%02X", pbRx[stat_byte]);
        return -1;
    }

    return (bufLen);
}

// pldmtool raw -m 10 -d 0x80 0x3F 0x01 0x15 0xA0 0x00 0x18 0x52
//                       0x0A => I2C BUS
//                       0x46 => I2C_Address
//                       0x00 => Read bytes length
//                       0x0A 0x03 0x00 0x09 0x15 => AL payload match standard
//                       spec 0x0A => [$cmdCode] 0x03 => [$write_numBytes] 0x00
//                       0x09 0x15 [$write_buff]

// Tx: 80 3f 01 15 a0 00 18 52 0a 46 00 0a 03 00 09 15
// Rx: 00 3f 01 00 15 a0 00 1c 52 00
// [00] => i2c transaction status

// pldmtool raw -m 10 -d 0x80 0x3F 0x01 0x15 0xA0 0x00 0x18 0x52
//                       0x0A => I2C BUS
//                       0x46 => I2C_Address
//                       0x02 => Read bytes length
//                       0x09 => [$cmdCode]

// Tx: 80 3f 01 15 a0 00 18 52 0a 46 02 09
// Rx: 00 3f 01 00 15 a0 00 1c 52 [00] [02] [78] [87]
// [00] => i2c transaction status
// [02] => i2c transaction length
// [78] [87] => i2c Rx data (w/o length)
int i2c_smbus_write_block_data_bic(int file, uint8_t command, uint8_t length,
                                     uint8_t* values)
{
    (void)file; /* unused */

    FILE* fp;
    int rc, i;
    uint8_t pb[64] = {0}; // Need to check command spec for max size

    // Prepare BIC payload
    // First byte is cmdcode for i2c_smbus_write_i2c_block_data
    pb[BIC_HEADER0_OFFSET] = BIC_HEADER0;
    pb[BIC_HEADER1_OFFSET] = BIC_HEADER1;
    pb[BIC_HEADER2_OFFSET] = BIC_HEADER2;
    pb[BIC_HEADER3_OFFSET] = BIC_HEADER3;
    pb[BIC_HEADER4_OFFSET] = BIC_HEADER4;
    pb[BIC_HEADER5_OFFSET] = BIC_HEADER5;
    pb[BIC_HEADER6_OFFSET] = BIC_HEADER6;
    pb[BIC_HEADER7_OFFSET] = BIC_HEADER7;
    pb[BIC_I2C_BUS_OFFSET] = bus;
    pb[BIC_I2C_ADDR_OFFSET] = addr;
    pb[BIC_I2C_READ_LEN_OFFSET] = 0x00;    // Read 0 byte for write only
    pb[BIC_I2C_CMDCODE_OFFSET] = command;  // Copy SMBus cmdcode first
    pb[BIC_I2C_WRITE_LEN_OFFSET] = length; // Write length

    // Copy SMBus Tx payload to buffer
    for (i = 0; i < length; i++)
    {
        pb[BIC_I2C_PAYLOAD_OFFSET + i] = values[i];
    }

    char pldm_command[PLDM_CMD_SIZE];
    char pldm_Tx_string[PLDM_TX_SIZE] = "";
    char temp_string[8];

    sprintf(pldm_command, "%s %d -d ", BIC_TxPrefix, slot_id);

    // Prepare string Tx buffer
    for (i = 0; i < (BIC_I2C_PAYLOAD_OFFSET + length); i++)
    {
        sprintf(temp_string, "%d", pb[i]);
        strcat(pldm_Tx_string, temp_string);
        strcat(pldm_Tx_string, " ");
    }
    strcat(pldm_command, pldm_Tx_string);

    // Retry PLDM_MAX_RETRY(5) times
    for (i = 0; i < PLDM_MAX_RETRY; i++)
    {
        fp = popen(pldm_command, "r");
        if (fp != NULL)
        {
            break;
        }
        usleep(100000);// sleep 0.1 second
    }

    if (fp == NULL)
    {
        perror("popen() failed");
        return -1;
    }

    uint8_t pbRx[PLDM_RX_SIZE];
    rc = pldm_get_response(fp, pbRx, BIC_RX_CMD_STATUS_OFFSET);

    // Need to check if the status Equivalent to SMBus API
    if (rc == -1)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int i2c_smbus_read_i2c_block_data_bic(int file, uint8_t command, uint8_t length,
                                        uint8_t* values)
{
    (void)file; /* unused */

    FILE* fp;
    int rc, i, nums;
    uint8_t pb[64] = {0}; // Need to check command spec for max size

    // Prepare BIC payload
    pb[BIC_HEADER0_OFFSET] = BIC_HEADER0;
    pb[BIC_HEADER1_OFFSET] = BIC_HEADER1;
    pb[BIC_HEADER2_OFFSET] = BIC_HEADER2;
    pb[BIC_HEADER3_OFFSET] = BIC_HEADER3;
    pb[BIC_HEADER4_OFFSET] = BIC_HEADER4;
    pb[BIC_HEADER5_OFFSET] = BIC_HEADER5;
    pb[BIC_HEADER6_OFFSET] = BIC_HEADER6;
    pb[BIC_HEADER7_OFFSET] = BIC_HEADER7;
    pb[BIC_I2C_BUS_OFFSET] = bus;
    pb[BIC_I2C_ADDR_OFFSET] = addr;
    pb[BIC_I2C_READ_LEN_OFFSET] = length; // Read n bytes
    pb[BIC_I2C_CMDCODE_OFFSET] = command; // Copy SMBus cmdcode first

    char pldm_command[PLDM_CMD_SIZE];
    char pldm_Tx_string[PLDM_TX_SIZE] = "";
    char temp_string[8];

    sprintf(pldm_command, "%s %d -d ", BIC_TxPrefix, slot_id);

    // Prepare string Tx buffer
    for (i = 0; i < (BIC_I2C_CMDCODE_OFFSET + 1); i++)
    {
        sprintf(temp_string, "%d", pb[i]);
        strcat(pldm_Tx_string, temp_string);
        strcat(pldm_Tx_string, " ");
    }
    strcat(pldm_command, pldm_Tx_string);

    // Retry PLDM_MAX_RETRY(5) times
    for (i = 0; i < PLDM_MAX_RETRY; i++)
    {
        fp = popen(pldm_command, "r");
        if (fp != NULL)
        {
            break;
        }
        usleep(100000);// sleep 0.1 second
    }

    if (fp == NULL)
    {
        perror("popen() failed");
        return -1;
    }

    uint8_t pbRx[PLDM_RX_SIZE];
    nums = pldm_get_response(fp, pbRx, BIC_RX_CMD_STATUS_OFFSET);

    // Check if PLDM Rx length match to request Rx length
    if (length != (pbRx[BIC_RX_LEN_OFFSET] + 1))
    {
        ASTERA_TRACE("len 0x%02x, pldm 0x%02x", length,
                     pbRx[BIC_RX_LEN_OFFSET]);
        return -1;
    }
    else
    {
// First byte returned is length for original definitions
#ifdef SMBUS_BLOCK_READ_UNSUPPORTED
        // Copy to buffer
        for (i = 0; i < length; i++)
        {
            values[i] = pbRx[BIC_RX_LEN_OFFSET + i];
        }

        // Need to check if the status Equivalent to SMBus API
        return (pbRx[BIC_RX_LEN_OFFSET] + 1);
#else
        // Copy to buffer
        for (i = 0; i < (length - 1); i++)
        {
            values[i] = pbRx[BIC_RX_PAYLOAD_OFFSET + i];
        }

        // Need to check if the status Equivalent to SMBus API
        return (pbRx[BIC_RX_LEN_OFFSET]);
#endif
    }
}

// Astera retimer functions

void plat_rt_preinit(void *args)
{
    struct retimer_config *config = (struct retimer_config *)args;

    slot_id = config->slot_id;
    type = config->type;
    bus = config->retimer_bus;
    addr = config->retimer_addr;
    width = config->retimer_width;

    printf("setup slot_id:%u bus:%u addr:0x%02X \n", slot_id, bus, addr);
}

int asteraI2COpenConnection(int i2cBus, int slaveAddress)
{
    (void)i2cBus;  /* unused */
    (void)slaveAddress;  /* unused */

    return 0;
}

AriesErrorType asteraI2CWriteBlockData(int handle, uint8_t cmdCode, uint8_t numBytes, uint8_t* buf)
{
    int rc = i2c_smbus_write_block_data_bic(handle, cmdCode, numBytes, buf);

    if (rc != 0)
    {
        return -1; // Equivalent to ARIES_FAILURE
    }
    return 0;
}

int asteraI2CReadBlockData(int handle, uint8_t cmdCode, uint8_t numBytes, uint8_t* buf)
{
    return i2c_smbus_read_i2c_block_data_bic(handle, cmdCode, numBytes, buf);
}

int asteraI2CWriteReadBlockData(int handle, uint8_t cmdCode, uint8_t numBytes, uint8_t* buf)
{
    (void)handle; /* unused */
    (void)cmdCode; /* unused */
    (void)numBytes; /* unused */
    (void)buf; /* unused */

    return -1;
}