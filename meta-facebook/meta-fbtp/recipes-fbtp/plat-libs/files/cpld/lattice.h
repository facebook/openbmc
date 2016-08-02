#define	LATTICE_INS_LENGTH		0x08

//lattice's cmd
#define 	ISC_ADDRESS_SHIFT			0x01
#define	ISC_ERASE					0x03
#define 	DATA_SHIFT					0x02

#define	DISCHARGE					0x14
#define	ISC_ENABLE					0x15
#define	IDCODE						0x16
#define	UES_READ					0x17
#define	UES_PROGRAM				0x1a
#define	SAMPLE						0x1c
#define	PROGRAM_DISABLE			0x1e
#define	ISC_ADDRESS_INIT			0x21
#define	ISC_PROG_INCR				0x27
#define	ISC_READ_INCR				0x2a
#define	PROGRAM_DONE				0x2f
#define	SRAM_ENABLE				0x55
#define	LSCC_READ_INCR_RTI			0x6a
#define	LSCC_PROGRAM_INCR_RTI		0x67


#define	READ_STATUS				0xb2

#define	BYPASS						0xff
/*************************************************************************************/
/* LC LC4064V-XXT4 */
extern int lc4064v_xxt4_cpld_erase(int fd);
extern int lc4064v_xxt4_cpld_program(int fd, FILE *jed_fd);
extern int lc4064v_xxt4_cpld_verify(int fd, FILE *jed_fd);

/*************************************************************************************/
/* LC LCMXO2280C */
extern int lcmxo2280c_cpld_erase(int fd);
extern int lcmxo2280c_cpld_program(int fd, FILE *jed_fd);
extern int lcmxo2280c_cpld_verify(int fd, FILE *jed_fd);




/*************************************************************************************/
struct cpld_dev_info {
	const char		*name;
	unsigned int 		dev_id;
	unsigned short 	dr_bits;			//row width
	unsigned int 		row_num;		//address length
	int (*cpld_id)(int fd, unsigned int *id);
	int (*cpld_erase)(int fd);
	int (*cpld_program)(int fd, FILE *jed_fd);
	int (*cpld_verify)(int fd, FILE *jed_fd);
};

/*************************************************************************************/

static struct cpld_dev_info lattice_device_list[] = {
	[0] = {
		.name = "LC LC4064V-XXT4",
		.dev_id = 0x01809043,
		.dr_bits = 352,
		.row_num = 95,
		.cpld_erase = lc4064v_xxt4_cpld_erase,
		.cpld_program = lc4064v_xxt4_cpld_program,
		.cpld_verify = lc4064v_xxt4_cpld_verify,
	},
	[1] = {
		.name = "LC LCMXO2280C",
		//.dev_id = 0x0128D043,
		.dev_id = 0x012BB043,
		.dr_bits = 436,
		.row_num = 1116,
		.cpld_erase = lcmxo2280c_cpld_erase,
		.cpld_program = lcmxo2280c_cpld_program,
		.cpld_verify = lcmxo2280c_cpld_verify,
	}
};
