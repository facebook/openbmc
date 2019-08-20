#ifndef _LATTICE_H_
#define _LATTICE_H_

#define	LATTICE_INS_LENGTH		0x08

//lattice's cmd
#define ISC_ADDRESS_SHIFT		0x01
#define	ISC_ERASE			0x03
#define DATA_SHIFT			0x02

#define	DISCHARGE			0x14
#define	ISC_ENABLE			0x15
#define	IDCODE				0x16
#define	UES_READ			0x17
#define	UES_PROGRAM			0x1a
#define	PRELOAD				0x1c
#define	PROGRAM_DISABLE			0x1e
#define	ISC_ADDRESS_INIT		0x21
#define	ISC_PROG_INCR			0x27
#define	ISC_READ_INCR			0x2a
#define	PROGRAM_DONE			0x2f
#define	SRAM_ENABLE			0x55
#define	LSCC_READ_INCR_RTI		0x6a
#define	LSCC_PROGRAM_INCR_RTI		0x67

#define	READ_STATUS			0xb2
/*LCMXO2 Programming Command*/
#define LCMXO2_IDCODE_PUB          0xE0
#define LCMXO2_ISC_ENABLE_X        0x74
#define LCMXO2_LSC_CHECK_BUSY      0xF0
#define LCMXO2_LSC_READ_STATUS     0x3C
#define LCMXO2_ISC_ERASE           0x0E
#define LCMXO2_LSC_INIT_ADDRESS    0x46
#define LCMXO2_LSC_INIT_ADDR_UFM   0x47
#define LCMXO2_LSC_PROG_INCR_NV    0x70
#define LCMXO2_ISC_PROGRAM_USERCOD 0xC2
#define LCMXO2_USERCODE            0xC0
#define LCMXO2_LSC_READ_INCR_NV    0x73
#define LCMXO2_ISC_PROGRAM_DONE    0x5E
#define LCMXO2_ISC_DISABLE         0x26
#define BYPASS                     0xFF

/*************************************************************************************/
#if 0
/* LC LCMXO2-2000HC */
int lcmxo2_2000hc_cpld_ver(unsigned int *ver);
int lcmxo2_2000hc_cpld_flash_enable(void);
int lcmxo2_2000hc_cpld_flash_disable(void);
int lcmxo2_2000hc_cpld_erase(void);
int lcmxo2_2000hc_cpld_program(FILE *jed_fd);
int lcmxo2_2000hc_cpld_verify(FILE *jed_fd);
#endif
/*LCMXO2Family*/
int LCMXO2Family_cpld_update(FILE *jed_fd);
int LCMXO2Family_cpld_Get_Ver(unsigned int *ver);
int LCMXO2Family_cpld_Get_id(unsigned int *dev_id);

int cpld_device_open(void);
int cpld_device_close(void);
/*************************************************************************************/
extern struct cpld_dev_info lattice_device_list[2];
#endif
