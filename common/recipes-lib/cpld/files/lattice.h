#ifndef _LATTICE_H_
#define _LATTICE_H_

#define	LATTICE_INS_LENGTH		0x08

//lattice's cmd
#define ISC_ADDRESS_SHIFT		0x01
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

/*XO2, XO3, NX common*/
#define ISC_ENABLE_X                  0x74
#define LSC_CHECK_BUSY                0xF0
#define LSC_READ_STATUS               0x3C
#define LSC_READ_STATUS1              0x3D
#define LSC_INIT_ADDRESS              0x46
#define LSC_INIT_ADDR_UFM             0x47
#define LSC_PROG_INCR_NV              0x70
#define LSC_READ_INCR_NV              0x73
#define ISC_PROGRAM_DONE              0x5E
#define	ISC_ERASE                     0x0E
#define IDCODE_PUB                    0xE0
#define USERCODE                      0xC0
#define ISC_DISABLE                   0x26
#define ISC_PROGRAM_USERCODE          0xC2


typedef struct
{
  unsigned long int QF;
  unsigned int *CF;
  unsigned int CF_Line;
  unsigned int *UFM;
  unsigned int UFM_Line;
  unsigned int Version;
  unsigned int CheckSum;
  unsigned int FEARBits;
  unsigned int FeatureRow;
} CPLDInfo;

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define LATTICE_COL_SIZE 128

int LCMXO2Family_Get_Update_Data_Size(FILE *jed_fd, int *cf_size, int *ufm_size);
int LCMXO2Family_JED_File_Parser(FILE *jed_fd, CPLDInfo *dev_info, int cf_size, int ufm_size);
int NX_JED_File_Parser(FILE *jed_fd, CPLDInfo *dev_info, int cf_size, int ufm_size);
int NX_Get_Update_Data_Size(FILE *jed_fd, int *cf_size, int *ufm_size);
int get_lattice_dev_list(struct cpld_dev_info** list );
#endif
