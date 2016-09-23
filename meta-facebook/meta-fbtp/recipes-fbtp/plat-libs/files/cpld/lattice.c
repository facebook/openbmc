#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <termios.h>

#include <sys/mman.h>
#include "lattice.h"
#include "ast-jtag.h"

extern struct cpld_dev_info *cur_dev;
extern xfer_mode mode;
extern int debug;

/*************************************************************************************/
unsigned int jed_file_parse_header(FILE *jed_fd)
{
	//File
	unsigned char tmp_buf[160];
	unsigned int *jed_data;

	unsigned int row  = 0;
	unsigned char input_char, input_bit;
	int sdr_array = 0, data_bit = 0, bit_cnt = 0;
	int cmp_err = 0;

	//Header paser
	while(fgets(tmp_buf, 120, jed_fd)!= NULL) {
		if (debug)
			printf("%s \n",tmp_buf);
		if (tmp_buf[0] == 0x4C) { // "L"
			break;
		}
	}
}

unsigned int jed_file_parser(FILE *jed_fd, unsigned int len, unsigned int *dr_data)
{
	int i = 0;
	unsigned char input_char, input_bit;
	int sdr_array = 0, data_bit = 0, bit_cnt = 0;
	int cmp_err = 0;

	//Jed row
	for(i = 0; i < len; i++) {
		input_char = fgetc(jed_fd);
		if ((input_char == 0x30) || (input_char == 0x31)) { // "0", "1"
			if (input_char == 0x30) {
				input_bit = 0;
			} else {
				input_bit = 1;
			}
			if (debug)
				printf("%d",input_bit);
			dr_data[sdr_array] |= (input_bit << data_bit);
			data_bit++;
			bit_cnt++;

			if((data_bit % 32) == 0) {
				if (debug)
					printf(" [%i] : %x \n",sdr_array, dr_data[sdr_array]);
				data_bit = 0;
				sdr_array++;
			}
		} else if (input_char == 0xd) {
			i--;
		} else if (input_char == 0xa) {
			i--;
		} else {
			if (debug)
				printf("parser errorxx [%x]\n", input_char);
			break;
		}
	}
	if (debug)
		printf(" [%i] : %x , Total %d \n",sdr_array, dr_data[sdr_array], bit_cnt);

	if(bit_cnt != len) {
		if (debug)
			printf("File Error \n");

		return -1;
	}

	return 0;
}

/*
 * TODO: Not yet tested at all, need to verify the JED and 
 * change the function accordingly. This function needs
 * to be changed (minor) to handle all the cases
 */
int lcmxo2_2000hc_cpld_program(FILE *jed_fd)
{
	int i;
	unsigned int tdo = 0;
	unsigned int *dr_data;
	//file
	unsigned int *jed_data;

	unsigned int row  = 0;
	int cmp_err = 0;

	dr_data = malloc(((424/32) + 1) * sizeof(unsigned int));
	jed_data = malloc(((cur_dev->dr_bits/32) + 1) * sizeof(unsigned int));

	//RUNTEST	IDLE	15 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle( 0, 0, 3);

	//! Check the IDCODE_PUB
	//SIR	8	TDI  (E0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, IDCODE_PUB);

	//SDR 32	TDI  (00000000)
	//		TDO  (01285043)
	//		MASK (FFFFFFFF);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer( 0, 32, dr_data);

	if(dr_data[0] != 0x12BB043) {
		if (debug)
			printf("ID Fail : %08x [0x012BB043] \n",dr_data[0]);

		return -1;
	}

	//! Program Bscan register

	//! Shift in Preload(0x1C) instruction
	//SIR	8	TDI  (1C);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x1C);

	//SDR	424 TDI (FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF);
	memset(dr_data, 0xff, (424/32 + 1) * sizeof(unsigned int));
	ast_jtag_tdi_xfer( 0, 424, dr_data);

	//! Enable the Flash (Transparent Mode)
	//! Shift in ISC ENABLE X (0x74) instruction
	//SIR 8 TDI  (74);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x74);

	//SDR 8 TDI  (00);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	dr_data[0] = 0x00;
	ast_jtag_tdi_xfer(0, 8, dr_data);
	usleep(3000);

	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8 TDI  (0E);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x0E);

	//SDR 8 TDI  (01);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	dr_data[0] = 0x01;
	ast_jtag_tdi_xfer(0, 8, dr_data);
	usleep(1000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8 TDI  (FF)
	//		TDO  (00)
	//		MASK (C0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);

	//! Shift in ISC ENABLE X (0x74) instruction
	//SIR 8 TDI  (74);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x74);

	//SDR 8 TDI  (08);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	dr_data[0] = 0x08;
	ast_jtag_tdi_xfer( 0, 8, dr_data);
	usleep(3000);

	//! Check the Key Protection fuses

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8 TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00024040);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer( 0, 32, dr_data);

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8 TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00010000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);

	//! Erase the Flash
	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8 TDI  (0E);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x0E);

	//SDR 8 TDI  (0E);
	//RUNTEST IDLE	2 TCK;
	dr_data[0] = 0x0E;
	ast_jtag_tdi_xfer( 0, 8, dr_data);

	//! Shift in LSC_CHECK_BUSY(0xF0) instruction
	//SIR 8 TDI  (F0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xF0);

	//LOOP 800 ;
	//RUNTEST IDLE	2 TCK	1.00E-002 SEC;
	//SDR 1 TDI  (0)
	//		TDO  (0);
	//ENDLOOP ;
	dr_data[0] = 0;
	for(i = 0; i <800 ; i++) {
		usleep(2000);
		ast_jtag_tdo_xfer( 0, 1, dr_data);
	}

	//! Read the status bit

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8 TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00003000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);
	if(dr_data[0] != 0x0) {
		if (debug)
			printf("%x [0x0]\n", dr_data[0]);
	}

	if (debug)
		printf("Erase Done \n");

	fseek(jed_fd, 0, SEEK_SET);
	jed_file_parse_header(jed_fd);

	//! Program CFG

	if (debug)
		printf("Program CFG \n");

	//! Shift in LSC_INIT_ADDRESS(0x46) instruction
	//SIR 8	TDI  (46);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x46);

	//SDR 8	TDI  (04);
	//RUNTEST IDLE	2 TCK	1.00E-002 SEC;
	dr_data[0] = 0x04;
	ast_jtag_tdo_xfer(0, 32, dr_data);

	if (debug)
		printf("Program 3198 .. \n");

//	mode = SW_MODE;
	for(row =0 ; row < cur_dev->row_num; row++) {
		memset(dr_data, 0, (cur_dev->dr_bits/32) * sizeof(unsigned int));
		jed_file_parser(jed_fd, cur_dev->dr_bits, dr_data);

		//! Shift in LSC_PROG_INCR_NV(0x70) instruction
		//SIR 8 TDI  (70);

		ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, 0x70);


		//! Shift in Data Row = 1
		//SDR 128 TDI  (120600000040000000DCFFFFCDBDFFFF);
		//RUNTEST IDLE	2 TCK;
		ast_jtag_tdi_xfer(0, cur_dev->dr_bits, dr_data);
		usleep(1000);

		//! Shift in LSC_CHECK_BUSY(0xF0) instruction
		//SIR 8 TDI  (F0);

		ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, 0xF0);

		//LOOP 10 ;
		//RUNTEST IDLE	1.00E-003 SEC;
		//SDR 1 TDI  (0)
		//		TDO  (0);
		//ENDLOOP ;
		for(i = 0;i < 10; i++) {
			usleep(3000);
			dr_data[0] = 0;
			ast_jtag_tdo_xfer(0, 1, dr_data);
			if(dr_data[0] == 0) break;
		}

		if(dr_data[0] != 0) {
			if (debug)
				printf("row %d, Fail [%d] \n", row, dr_data[0]);
		} else {
			if (debug)
				printf(".");
		}

	}
//	mode = HW_MODE;

	//! Program the UFM
	if (debug)
		printf("Program the UFM : 640\n");

	//! Shift in LSC_INIT_ADDR_UFM(0x47) instruction
	//SIR 8	TDI  (47);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x47);

	for(row =0 ; row < 640; row++) {
		memset(dr_data, 0, (cur_dev->dr_bits/32) * sizeof(unsigned int));
		jed_file_parser(jed_fd, cur_dev->dr_bits, dr_data);

		//! Shift in LSC_PROG_INCR_NV(0x70) instruction
		//SIR 8	TDI  (70);
		ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, 0x70);

		//! Shift in Data Row = 1
		//SDR 128 TDI  (00000000000000000000000000000000);
		//RUNTEST IDLE	2 TCK;
		ast_jtag_tdi_xfer(0, cur_dev->dr_bits, dr_data);

		//! Shift in LSC_CHECK_BUSY(0xF0) instruction
		//SIR 8	TDI  (F0);
		ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, 0xF0);

		//LOOP 10 ;
		//RUNTEST IDLE	1.00E-003 SEC;
		//SDR 1	TDI  (0)
		//		TDO  (0);
		//ENDLOOP ;
		for(i = 0;i < 10; i++) {
			usleep(3000);
			dr_data[0] = 0;
			ast_jtag_tdo_xfer(0, 1, dr_data);
			if(dr_data[0] == 0) break;
		}

		if(dr_data[0] != 0) {
			if (debug)
				printf("row %d, Fail [%d] \n",row, dr_data[0]);
		} else {
			if (debug)
				printf(".");
		}

	}

	//! Program USERCODE

	//! Shift in READ USERCODE(0xC0) instruction
	//SIR 8	TDI  (C0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xC0);

	//SDR 32	TDI  (00000000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdi_xfer(0, 32, dr_data);

	//! Shift in ISC PROGRAM USERCODE(0xC2) instruction
	//SIR 8	TDI  (C2);
	//RUNTEST IDLE	2 TCK	1.00E-002 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xC2);
	usleep(2000);

	//! Read the status bit

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00003000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);

	//! Program Feature Rows

	//! Shift in LSC_INIT_ADDRESS(0x46) instruction
	//SIR 8	TDI  (46);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x46);

	//SDR 8	TDI  (02);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	dr_data[0] = 0x02;
	ast_jtag_tdi_xfer(0, 8, dr_data);
	usleep(3000);

	//! Shift in LSC_PROG_FEATURE(0xE4) instruction
	//SIR 8	TDI  (E4);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xE4);

	//SDR 64	TDI  (0000000000000000);
	//RUNTEST IDLE	2 TCK;
	dr_data[0] = 0x00000000;
	dr_data[1] = 0x00000000;
	ast_jtag_tdi_xfer(0, 64, dr_data);


	//! Shift in LSC_CHECK_BUSY(0xF0) instruction
	//SIR 8	TDI  (F0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xF0);

	//LOOP 10 ;
	//RUNTEST IDLE	1.00E-003 SEC;
	//SDR 1	TDI  (0)
	//		TDO  (0);
	//ENDLOOP ;
	for(i = 0;i < 10; i++) {
		usleep(3000);
		dr_data[0] = 0;
		ast_jtag_tdo_xfer(0, 1, dr_data);
	}

	//! Shift in LSC_READ_FEATURE (0xE7) instruction
	//SIR 8	TDI  (E7);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xE7);

	//SDR 64	TDI  (0000000000000000)
	//		TDO  (0000000000000000);
	dr_data[0] = 0x00000000;
	dr_data[1] = 0x00000000;
	ast_jtag_tdo_xfer(0, 64, dr_data);

	//! Shift in in LSC_PROG_FEABITS(0xF8) instruction
	//SIR 8	TDI  (F8);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xF8);

	//SDR 16	TDI  (0620);
	//RUNTEST IDLE	2 TCK;
	dr_data[0] = 0x0620;
	ast_jtag_tdi_xfer(0, 16, dr_data);

	//! Shift in LSC_CHECK_BUSY(0xF0) instruction
	//SIR 8	TDI  (F0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xF0);

	//LOOP 10 ;
	//RUNTEST IDLE	1.00E-003 SEC;
	//SDR 1	TDI  (0)
	//		TDO  (0);
	//ENDLOOP ;
	for(i = 0;i < 10; i++) {
		usleep(3000);
		dr_data[0] = 0;
		ast_jtag_tdo_xfer(0, 1, dr_data);
	}

	//! Shift in in LSC_READ_FEABITS(0xFB) instruction
	//SIR 8	TDI  (FB);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xFB);

	//SDR 16	TDI  (0000)
	//		TDO  (0620)
	//		MASK (FFF2);
	dr_data[0] = 0x0;
	ast_jtag_tdo_xfer(0, 16, dr_data);

	//! Program DONE bit

	//! Shift in ISC PROGRAM DONE(0x5E) instruction
	//SIR 8	TDI  (5E);
	//RUNTEST IDLE	2 TCK;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x5E);

	//! Shift in LSC_CHECK_BUSY(0xF0) instruction
	//SIR 8	TDI  (F0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xF0);

	//LOOP 10 ;
	//RUNTEST IDLE	1.00E-003 SEC;
	//SDR 1	TDI  (0)
	//		TDO  (0);
	//ENDLOOP ;
	for(i = 0;i < 10; i++) {
		usleep(3000);
		dr_data[0] = 0;
		ast_jtag_tdo_xfer(0, 1, dr_data);
	}

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//		TDO  (04)
	//		MASK (C4);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xFF);

	//! Exit the programming mode

	//! Shift in ISC DISABLE(0x26) instruction
	//SIR 8	TDI  (26);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x26);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)//;
	//RUNTEST IDLE	2 TCK	1.00E-001 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xFF);

	free(jed_data);
	free(dr_data);

	return 0;

}

int lcmxo2_2000hc_cpld_flash_enable(void)
{
	unsigned int data=0;
	unsigned int *dr_data;

	unsigned int alloc_size = ((cur_dev->dr_bits /32 + 1) * sizeof(unsigned int) + 4096 ) % 4096;

	dr_data = (unsigned int *)malloc(alloc_size);
	if (dr_data == NULL) {
		return -1;;
	}

	//RUNTEST	IDLE	15 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle( 0, 0, 3);
	usleep(3000);

	//! Program Bscan register

	//! Shift in Preload(0x1C) instruction
	//SIR	8	TDI  (1C);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x1C);

	//SDR 424 TDI  (FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF);
	memset(dr_data, 0xff, ((424/32) + 1) * sizeof(unsigned int));
	ast_jtag_tdi_xfer(0, 424, dr_data);

	//! Enable the Flash (Transparent Mode)
	//! Shift in ISC ENABLE X(0x74) instruction
	//SIR 8	TDI  (74);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x74);

	//SDR 8	TDI  (00);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x00;
	ast_jtag_tdi_xfer( 0, 8, &data);
	usleep(3000);

	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8	TDI  (0E);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x0E);

	//SDR 8	TDI  (01);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	data = 0x01;
	ast_jtag_tdi_xfer(0, 8, &data);
	usleep(3000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//	TDO  (00)
	//	MASK (C0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);

	//! Shift in ISC ENABLE X (0x74) instruction
	//SIR 8	TDI  (74);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x74);

	//SDR 8	TDI  (08);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x08;
	ast_jtag_tdi_xfer( 0, 8, &data);
	usleep(3000);

	if (dr_data != NULL)
		free(dr_data);

	return 0;
}

int lcmxo2_2000hc_cpld_flash_disable(void)
{
	//! Exit the programming mode

	//! Shift in ISC DISABLE(0x26) instruction
	//SIR 8	TDI  (26);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x26);
	usleep(1000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF);
	//RUNTEST IDLE	2 TCK	1.00E-001 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);
	usleep(1000);

	return 0;
}

int lcmxo2_2000hc_cpld_ver(unsigned int *ver)
{
	unsigned int data=0;
	unsigned int *dr_data;

	unsigned int alloc_size = ((cur_dev->dr_bits /32 + 1) * sizeof(unsigned int) + 4096 ) % 4096;

	dr_data = (unsigned int *)malloc(alloc_size);
	if (dr_data == NULL) {
		return -1;;
	}

	//! Verify USERCODE
	//! Shift in READ USERCODE(0xC0) instruction
	//SIR 8	TDI  (C0);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xC0);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (FFFFFFFF);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);
	*ver = dr_data[0];

	if (dr_data != NULL)
		free(dr_data);

	return 0;
}

/*
 * TODO: Tested with CPLD programmed against F08_V01.jed
 * It needs to be tested thoroughly before using it.
 */
int lcmxo2_2000hc_cpld_verify(FILE *jed_fd)
{
	int i;
	unsigned int data=0;
	unsigned int tdo;
	unsigned int *jed_data;
	unsigned int *dr_data;
	unsigned int row  = 0;
	int cmp_err = 0;
	int parser_err = 0;

	int cfg_err = 0;
	int ufm_err = 0;

	int ret_err = -1;

	unsigned int alloc_size = ((cur_dev->dr_bits /32 + 1) * sizeof(unsigned int) + 4096 ) % 4096;

	dr_data = (unsigned int *)malloc(alloc_size);
	if (dr_data == NULL) {
		if(debug)
			printf("memory allocation for dr_data failed.\n");

		goto err_handle;
	}

	jed_data = (unsigned int *)malloc(alloc_size);
	if (jed_data == NULL) {
		if(debug)
			printf("memory allocation for jed_data failed.\n");

		goto err_handle;
	}

	//RUNTEST	IDLE	15 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle( 0, 0, 3);
	usleep(3000);

	//! Check the IDCODE_PUB
	//SIR	8	TDI  (E0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, IDCODE_PUB);

	//SDR 32	TDI  (00000000)
	//		TDO  (012BB043)
	//		MASK (FFFFFFFF);
	ast_jtag_tdo_xfer( 0, 32, dr_data);

	if(dr_data[0] != 0x12BB043) {
		if (debug)
			printf("ID Fail : %08x [0x012BB043] \n",dr_data[0]);

		free(dr_data);
	}

	//! Program Bscan register

	//! Shift in Preload(0x1C) instruction
	//SIR	8	TDI  (1C);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x1C);

	//SDR 424 TDI  (FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF);
	memset(dr_data, 0xff, ((424/32) + 1) * sizeof(unsigned int));
	ast_jtag_tdi_xfer(0, 424, dr_data);

	//! Enable the Flash (Transparent Mode)
	//! Shift in ISC ENABLE X (0x74) instruction
	//SIR 8	TDI  (74);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x74);

	//SDR 8	TDI  (00);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x00;
	ast_jtag_tdi_xfer( 0, 8, &data);
	usleep(3000);

	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8	TDI  (0E);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x0E);

	//SDR 8	TDI  (01);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	data = 0x01;
	ast_jtag_tdi_xfer(0, 8, &data);
	usleep(3000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//	TDO  (00)
	//	MASK (C0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);

	//! Shift in ISC ENABLE X (0x74) instruction
	//SIR 8	TDI  (74);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x74);

	//SDR 8	TDI  (08);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x08;
	ast_jtag_tdi_xfer( 0, 8, &data);
	usleep(3000);

	//! Verify USERCODE
	if (debug)
		printf("Verify USERCODE \n");

	//! Shift in READ USERCODE(0xC0) instruction
	//SIR 8	TDI  (C0);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xC0);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (FFFFFFFF);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);

	if (debug)
		printf("CPLD USERCODE = 0x%x\n", dr_data[0]);


	//! Verify the Flash
	if (debug)
		printf("Starting to Verify Device . . . This will take a few seconds\n");

	//! Shift in LSC_INIT_ADDRESS(0x46) instruction
	//SIR 8	TDI  (46);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x46);

	//SDR 8	TDI  (04);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x04;
	ast_jtag_tdi_xfer( 0, 8, &data);
	usleep(3000);


	//! Shift in LSC_READ_INCR_NV(0x73) instruction
	//SIR 8	TDI  (73);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x73);
	usleep(3000);

	fseek(jed_fd, 0, SEEK_SET);
	jed_file_parse_header(jed_fd);

	if (debug)
		printf("Verify CONFIG 3198 \n");
	cmp_err = 0;
	row = 0;

	//for(row = 0; row < cur_dev->row_num; row++) {
	for(row = 0; row < 3198; row++) {
		memset(dr_data, 0, 16);
		memset(jed_data, 0, 16);
		parser_err = jed_file_parser(jed_fd, 128, jed_data);
		if (parser_err == -1) {
			jed_file_parse_header(jed_fd);
			parser_err = jed_file_parser(jed_fd, 128, jed_data);
			if (parser_err == -1) {
				goto err_handle;
			}
		}

		ast_jtag_tdo_xfer( 0, 128, dr_data);

		for(i = 0; i < 4; i++) {
			if(dr_data[i] != jed_data[i]) {
				cmp_err = 1;
				goto err_handle;
			}
		}

		//RUNTEST	IDLE	2 TCK	1.00E-003 SEC;
		usleep(3000);
	}
	cfg_err = 0;

	jed_file_parse_header(jed_fd);

	//! Verify the UFM

	//! Shift in LSC_INIT_ADDR_UFM(0x47) instruction
	//SIR	8	TDI  (47);
	//RUNTEST	IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x47);
	usleep(3000);


	//! Shift in LSC_READ_INCR_NV(0x73) instruction
	//SIR	8	TDI  (73);
	//RUNTEST	IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x73);
	usleep(3000);

	if (debug)
		printf("Verify the UFM 640 \n");
	//! Shift out Data Row
	for(row =0 ; row < 640; row++) {
		memset(dr_data, 0, 16);
		memset(jed_data, 0, 16);
		parser_err = jed_file_parser(jed_fd, 128, jed_data);
		if (parser_err == -1) {
			jed_file_parse_header(jed_fd);
			parser_err = jed_file_parser(jed_fd, 128, jed_data);
			if (parser_err == -1) {
				goto err_handle;
			}
		}

		ast_jtag_tdo_xfer( 0, 128, dr_data);

		//for(i = 0; i < (cur_dev->dr_bits /32); i++) {
		for(i = 0; i < 4; i++) {
			if(dr_data[i] != jed_data[i]) {
				cmp_err = 1;
				goto err_handle;
			}
		}
		//RUNTEST	IDLE	2 TCK	1.00E-003 SEC;
		usleep(3000);
	}
	ufm_err = 0;

	//! Verify USERCODE
	if (debug)
		printf("Verify USERCODE \n");

	//! Shift in READ USERCODE(0xC0) instruction
	//SIR 8	TDI  (C0);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xC0);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (FFFFFFFF);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);

	//! Read the status bit

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00003000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer( 0, 32, dr_data);


	//! Verify Feature Rows

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00010000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer( 0, 32, dr_data);

	//! Shift in LSC_READ_FEATURE (0xE7) instruction
	//SIR 8	TDI  (E7);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xE7);
	usleep(3000);

	//SDR 64	TDI  (0000000000000000)
	//		TDO  (0000000000000000);
	dr_data[0] = 0x00000000;
	dr_data[1] = 0x00000000;
	ast_jtag_tdo_xfer( 0, 64, dr_data);

	//! Shift in in LSC_READ_FEABITS(0xFB) instruction
	//SIR 8	TDI  (FB);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xFB);
	usleep(3000);

	//SDR 16	TDI  (0000)
	//		TDO  (0620)
	//		MASK (FFF2);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer( 0, 16, dr_data);
	if (debug)
		printf("read %x [0x0620] \n", dr_data[0] & 0xffff);

	//! Read the status bit

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00003000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);

	//! Verify Done Bit

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//	TDO  (04)
	//	MASK (C4);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);

	//! Exit the programming mode

	//! Shift in ISC DISABLE(0x26) instruction
	//SIR 8	TDI  (26);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x26);
	usleep(1000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF);
	//RUNTEST IDLE	2 TCK	1.00E-001 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);
	usleep(1000);

	//! Verify SRAM DONE Bit

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//		TDO  (04)
	//		MASK (84);
	tdo = ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);

	if (!cfg_err && !ufm_err)
		ret_err = 0;

err_handle:
	if (dr_data != NULL)
		free(dr_data);

	if (jed_data != NULL)
		free(jed_data);

	if(cmp_err) {
		if (debug)
			printf("Verify Error !!\n");
	} else {
		if (debug)
			printf("Verify Done !!\n");
	}

	if (parser_err) {
		if (debug)
			printf("Error while parsing JED file\n");
	} else {
		if (debug)
			printf("NO Error while parsing JED file\n");
	}

	return ret_err;

}

/*
 * TODO: Not tested at all, needs to be changed to handle
 * CPLD LCMXO2-200HC
 */
int lcmxo2_2000hc_cpld_erase(void)
{
	int i = 0;
	unsigned int tdo = 0;
	unsigned int *sdr_data;
	unsigned int data=0;
	unsigned int sdr_array = 0;

	//RUNTEST	IDLE	15 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle( 0, 0, 3);

	//! Check the IDCODE_PUB
	//SIR	8	TDI  (E0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, IDCODE_PUB);

	//SDR 32	TDI  (00000000)
	//		TDO  (01285043)
	//		MASK (FFFFFFFF);
	data = 0x00000000;
	ast_jtag_tdo_xfer( 0, 32, &data);

	if(data != 0x12BB043) {
		if (debug)
			printf("ID Fail : %08x [0x012BB043] \n",data);

		return -1;
	}

	//! Program Bscan register

	//! Shift in Preload(0x1C) instruction
	//SIR	8	TDI  (1C);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, PRELOAD);

	//SDR 424 TDI (FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF);
	sdr_array = 424/32 + 1;
	sdr_data = malloc(sdr_array * sizeof(unsigned int));
	memset(sdr_data, 0xff, sdr_array * sizeof(unsigned int));
	ast_jtag_tdi_xfer( 0, 424, sdr_data);
	free(sdr_data);

	//! Enable the Flash (Transparent Mode)
	//! Shift in ISC ENABLE X (0x74) instruction
	//SIR 8	TDI  (74);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x74);

	//SDR 8	TDI  (00);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x00;
	ast_jtag_tdi_xfer(0, 8, &data);
	usleep(3000);

	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8	TDI  (0E);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x0E);

	//SDR 8	TDI  (01);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	data = 0x01;
	ast_jtag_tdi_xfer(0, 8, &data);
	usleep(1000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//		TDO  (00)
	//		MASK (C0);
	tdo = ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);

	//! Shift in ISC ENABLE X (0x74) instruction
	//SIR 8	TDI  (74);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x74);

	//SDR 8	TDI  (08);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x08;
	ast_jtag_tdi_xfer( 0, 8, &data);
	usleep(3000);

	//! Check the Key Protection fuses

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00024040);
	data = 0x00000000;
	ast_jtag_tdo_xfer( 0, 32, &data);

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00010000);
	data = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, &data);

	//! Erase the Flash

	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8	TDI  (0E);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x0E);

	//SDR 8	TDI  (0E);
	//RUNTEST IDLE	2 TCK;
	data = 0x0E;
	ast_jtag_tdi_xfer( 0, 8, &data);

	//! Shift in LSC_CHECK_BUSY(0xF0) instruction
	//SIR 8	TDI  (F0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xF0);

	//LOOP 800 ;
	//RUNTEST IDLE	2 TCK	1.00E-002 SEC;
	//SDR 1	TDI  (0)
	//		TDO  (0);
	//ENDLOOP ;
	data = 0;
	for(i = 0; i <800 ; i++) {
		usleep(2000);
		ast_jtag_tdo_xfer( 0, 1, &data);
	}

	//! Read the status bit

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00003000);
	data = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, &data);

	//! Exit the programming mode

	//! Shift in ISC DISABLE(0x26) instruction
	//SIR 8	TDI  (26);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x26);
	usleep(3000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF);
	//RUNTEST IDLE	2 TCK	1.00E-001 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);

	usleep(1000);

	return 0;

}
