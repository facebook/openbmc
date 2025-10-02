#include "aries_api_types.h"

#ifndef SMBUS_BLOCK_READ_UNSUPPORTED
#define SMBUS_BLOCK_READ_UNSUPPORTED
#endif

// BIC-based retimer
#define BIC_HEADER0 0x80
#define BIC_HEADER1 0x3F
#define BIC_HEADER2 0x01
#define BIC_HEADER3 0x15
#define BIC_HEADER4 0xA0
#define BIC_HEADER5 0x00
#define BIC_HEADER6 0x18
#define BIC_HEADER7 0x52

#define BIC_HEADER0_OFFSET 0x00
#define BIC_HEADER1_OFFSET 0x01
#define BIC_HEADER2_OFFSET 0x02
#define BIC_HEADER3_OFFSET 0x03
#define BIC_HEADER4_OFFSET 0x04
#define BIC_HEADER5_OFFSET 0x05
#define BIC_HEADER6_OFFSET 0x06
#define BIC_HEADER7_OFFSET 0x07
#define BIC_I2C_BUS_OFFSET 0x08
#define BIC_I2C_ADDR_OFFSET 0x09
#define BIC_I2C_READ_LEN_OFFSET 0x0A
#define BIC_I2C_CMDCODE_OFFSET 0x0B
#define BIC_I2C_WRITE_LEN_OFFSET 0x0C
#define BIC_I2C_PAYLOAD_OFFSET 0x0D

#define BIC_RX_CMD_STATUS_OFFSET 0x09
#define BIC_RX_LEN_OFFSET 0x0A
#define BIC_RX_PAYLOAD_OFFSET 0x0B

#define BIC_STS_READY 0x00

#define PLDM_CMD_SIZE 512
#define PLDM_TX_SIZE 512
#define PLDM_RX_SIZE 512

#define PLDM_MAX_RETRY 5
#define PLDM_RX_FIRST_SPACE 13

#ifdef __cplusplus
extern "C" {
#endif

    extern uint8_t bus; //0'base
    extern uint8_t addr;  //8 bits AL
    extern uint8_t width;
    extern AriesDevicePartType type;

    void plat_rt_preinit(void *args);

    struct retimer_config {
        uint8_t slot_id;
        AriesDevicePartType type;
        uint8_t retimer_bus; //0'base
        uint8_t retimer_addr;  //8 bits
        uint8_t retimer_width;
    };

    void BIC_Set_I2c_Addr(uint8_t addr);
    void BIC_Set_I2c_Bus(uint8_t bus);
    uint8_t BIC_Set_eid(uint8_t eid);

    uint32_t pldm_get_response(FILE* fp, uint8_t* pbRx, int stat_byte);

    int i2c_smbus_write_block_data_bic(int file, uint8_t command, uint8_t length, uint8_t* values);
    int i2c_smbus_read_i2c_block_data_bic(int file, uint8_t command, uint8_t length, uint8_t* values);

#ifdef __cplusplus
} // extern "C"
#endif