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
#define	IDCODE_PUB			0xE0
#define	BYPASS				0xff

/*************************************************************************************/
/* LC LCMXO2-2000HC */
extern int lcmxo2_2000hc_cpld_ver(unsigned int *ver);
extern int lcmxo2_2000hc_cpld_flash_enable(void);
extern int lcmxo2_2000hc_cpld_flash_disable(void);
extern int lcmxo2_2000hc_cpld_erase(void);
extern int lcmxo2_2000hc_cpld_program(FILE *jed_fd);
extern int lcmxo2_2000hc_cpld_verify(FILE *jed_fd);

/*************************************************************************************/
struct cpld_dev_info {
	const char		*name;
	unsigned int 	dev_id;
	unsigned short 	dr_bits;		//row width
	unsigned int 	row_num;		//address length
	int (*cpld_ver)(unsigned int *id);
	int (*cpld_flash_enable)(void);
	int (*cpld_flash_disable)(void);
	int (*cpld_erase)(void);
	int (*cpld_program)(FILE *jed_fd);
	int (*cpld_verify)(FILE *jed_fd);
};

/*************************************************************************************/
static struct cpld_dev_info lattice_device_list[] = {
	[0] = {
		.name = "LC LCMXO2-2000HC",
		.dev_id = 0x012BB043,
		.dr_bits = 1272,
		.row_num = 420,
		.cpld_ver = lcmxo2_2000hc_cpld_ver,
		.cpld_flash_enable = lcmxo2_2000hc_cpld_flash_enable,
		.cpld_flash_disable = lcmxo2_2000hc_cpld_flash_disable,
		.cpld_erase = lcmxo2_2000hc_cpld_erase,
		.cpld_program = lcmxo2_2000hc_cpld_program,
		.cpld_verify = lcmxo2_2000hc_cpld_verify,
  },
  [1] = {
    .name = "LC LCMXO2-4000HC",
    .dev_id = 0x012BC043,
    .dr_bits = 1272, // un-confirmed value
    .row_num = 420, // un-confirmed value
    .cpld_ver = lcmxo2_2000hc_cpld_ver,
    .cpld_flash_enable = lcmxo2_2000hc_cpld_flash_enable,
    .cpld_flash_disable = lcmxo2_2000hc_cpld_flash_disable,
    .cpld_erase = lcmxo2_2000hc_cpld_erase,
    .cpld_program = lcmxo2_2000hc_cpld_program,
    .cpld_verify = lcmxo2_2000hc_cpld_verify,
  }
};
