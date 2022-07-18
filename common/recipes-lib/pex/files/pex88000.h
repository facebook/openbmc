#ifndef __PEX88000_H__
#define __PEX88000_H__

#ifdef __cplusplus
extern "C" {
#endif

/* memory region */
#define BRCM_CHIME_AXI_CSR_ADDR   0x001F0100
#define BRCM_CHIME_AXI_CSR_DATA   0x001F0104
#define BRCM_CHIME_AXI_CSR_CTL    0x001F0108

#define BRCM_TEMP_SNR0_CTL_REG1     0xFFE78504
#define BRCM_TEMP_SNR0_CTL_REG13    0xFFE78534
#define BRCM_TEMP_NR0_STAT_REG0     0xFFE78538

#define BRCM_SET_TEMP_CTL_REG1      0x200653E8

#define BRCM_SPI_FW_REGION1  0x80000
#define BRCM_SPI_FW_REGION2  0x200000

#define BRCM_CONFIG_VERSION_REG  0xFFF00008
/* Base 0x80000 or 0x200000 */
#define BRCM_FW_VERSION_OFT 0x1C
#define BRCM_FW_SIZE_OFT 0x30
#define BRCM_FW_XML_SIZE_OFT 0x278
#define BRCM_FW_REGION_STATUS_HEADER_OFT 0x38
#define BRCM_FW_REGION_CURRENT_STATUS_OFT 0x3C

#define SPIBASE    0x2a0c0000
#define SPISHBASE  0x10000000

#define BRCM_I2C5_CMD_READ  0x4
#define BRCM_I2C5_CMD_WRITE 0x3

#define MAX_RETRY_TIMES 50

#define CMD_INTERNAL_DELAY_TIME_MS 300

#define MAX_VALUE_LEN 256

struct write_cmd_format {
    uint8_t readwrite;
    uint8_t img_act_op;
    uint8_t reserved[2];
    uint32_t data_len;
};

uint8_t pex88000_get_main_version(uint8_t bus, char *ver);
uint8_t pex88000_get_sbr_version(uint8_t bus, char *ver);
uint8_t pex88000_get_MFG_config_version(uint8_t bus, char *ver);

#endif