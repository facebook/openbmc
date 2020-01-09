/*
 * lattice_cpld
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "jedecfile.h"
#include "jtag_common.h"
#include "lattice_cpld.h"

static cpld_device_t device_list[] = {
    {
		.name = "LC LCMXO2-1200HC",
		.dev_id = 0x012BA043,
		.page_bits = 128,
		.num_of_cfg_pages = 2175,
		.num_of_ufm_pages = 512,
	},
	{
		.name = "LC LCMXO3-4300C",
		.dev_id = 0xE12BD043,
		.page_bits = 128,
		.num_of_cfg_pages = 9212,
		.num_of_ufm_pages = 2048,
	}
};

static jtag_object_t *global_jtag_object = NULL;

jtag_object_t *jtag_hardware_mode_init(const char *dev_name)
{
    jtag_object_t *temp_object = jtag_init(dev_name, JTAG_XFER_HW_MODE, 0);
    if(NULL == temp_object){
        printf("%s(%d) - failed to init jtag hardware mode\n", __FILE__, __LINE__);
        return NULL;
    }
    global_jtag_object = temp_object;
    return temp_object;
}

static int write_instruction_register(unsigned char endstate, unsigned int *buf, int length)
{
    return jtag_write_instruction_register(global_jtag_object, endstate, buf, length);
}

int write_data_register(unsigned char endstate, unsigned int *buf, int length)
{
    return jtag_write_data_register(global_jtag_object, endstate, buf, length);
}

int write_onebyte_instruction(unsigned char endstate, unsigned char instruction)
{
    unsigned int temp = instruction;
    return write_instruction_register(endstate, &temp, LATTICE_INSTRUCTION_LENGTH);
}

int write_onebyte_opcode(unsigned char endstate, unsigned char opcode)
{
    unsigned int temp = opcode;
    return write_data_register(endstate, &temp, LATTICE_INSTRUCTION_LENGTH);
}


int read_data_register(unsigned char endstate, unsigned int *buf, int length)
{
    return jtag_read_data_register(global_jtag_object, endstate, buf, length);
}

static int read_device_id(unsigned int *deviceId)
{
    unsigned int dr_data;
    int ret;
    ret = write_onebyte_instruction(JTAG_STATE_IDLE, CPD_IDCODE_PUB);
    if(ret < 0){
        printf("%s(%d) - failed to write IDCODE_PUB to cpld\n", __FILE__, __LINE__);
        return -1;
    }

    ret = read_data_register(JTAG_STATE_IDLE, &dr_data, BITS_OF_UNSIGNED_INT);
    if(ret < 0){
        printf("%s(%d) - failed to read data from cpld\n", __FILE__, __LINE__);
        return -1;
    }
    *deviceId = dr_data;
	return 0;
}

static int read_status_register(unsigned int *status, int ustime)
{
    int ret;
    unsigned int dr_data = 0;
    enum jtag_endstate endstate;

    ret = jtag_get_status(global_jtag_object, &endstate);
    if(ret < 0){
        printf("%s(%d) - jtag_get_status failure\n", __FILE__, __LINE__);
        return -1;
    }

    ret = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_READ_STATUS);
    if(ret < 0){
        printf("%s(%d) - failed to write LSC_READ_STATUS command to cpld\n", __FILE__, __LINE__);
        return -1;
    }
    
    usleep(ustime);
    
    ret = read_data_register(0, &dr_data, 32);
    if(ret < 0){
        printf("%s(%d) - failed to write data from cpld\n", __FILE__, __LINE__);
        return -1;
    }
	*status = dr_data;

	return 0;
}

static int read_busy_register(unsigned int *status, int ustime)
{
    int ret;
    unsigned int dr_data = 0;
    ret = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_CHECK_BUSY);
    if(ret < 0){
        printf("%s(%d) - failed to write LSC_READ_STATUS command to cpld\n", __FILE__, __LINE__);
        return -1;
    }
    
    usleep(ustime);
    
    ret = read_data_register(0, &dr_data, 8);
    if(ret < 0){
        printf("%s(%d) - failed to write data from cpld\n", __FILE__, __LINE__);
        return -1;
    }
	*status = dr_data;

	return 0;
}

int run_test_idle(unsigned char reset, unsigned char endstate, unsigned char tck)
{
    return jtag_run_test_idle(global_jtag_object, reset, endstate, tck);
}


int check_cpld_status(unsigned int options, int seconds)
{
    unsigned int status = 0;
    int ret;
    int read_status_time = 3000;
    int check_time = 1000;
    int counter = seconds*(1000*1000/(read_status_time+check_time));
    
    while(counter--){
        ret = read_status_register(&status, read_status_time);
        if(ret < 0){
            printf("%s(%d) - failed to read cpld status\n", __FILE__, __LINE__);
            return -1;
        }

        if((status&options) == 0){
            return 0;
        }
        run_test_idle(0, JTAG_STATE_IDLE, 2);
        usleep(check_time);
    }
    printf("%s(%d) - check cpld status timeout 0x%08x\n", __FILE__, __LINE__, status);
    return -1;
}

int check_cpld_busy(unsigned int options, int seconds)
{
    unsigned int status;
    int ret;
    int read_status_time = 3000;
    int check_time = 1000;
    int counter = seconds*(1000*1000/(read_status_time+check_time));
    
    while(counter--){
        ret = read_busy_register(&status, read_status_time);
        if(ret < 0){
            printf("%s(%d) - failed to read cpld status\n", __FILE__, __LINE__);
            return -1;
        }

        if((status&options) == 0){
            return 0;
        }
        usleep(check_time);
    }
    printf("%s(%d) - check cpld status timeout 0x%08x\n", __FILE__, __LINE__, status);
    return -1;
}

cpld_device_t *scan_cpld_device()
{
    unsigned int tempId;
    int ret = 0;
    int i;
    cpld_device_t *pcpld_device = NULL;
    
    ret = read_device_id(&tempId);
    if(ret < 0){
        printf("%s(%d) - failed to read cpld device id \n", __FILE__, __LINE__);
        return NULL;
    } 

    for(i = 0; i < sizeof(device_list)/sizeof(cpld_device_t); i++){
        if(tempId == device_list[i].dev_id){
            printf("Find CPLD %s (ID: %08x) via JTAG master\n", \
                device_list[i].name, device_list[i].dev_id);
            pcpld_device = &device_list[i];
            goto end_of_func;
        }
    }

    printf("Unsupport device id 0x%08x\n", tempId);

end_of_func:
	return pcpld_device;
}

int preload()
{
    int bits = 664;
    int rc;
    unsigned int *dr_data;
    int buffer_size = bits/BITS_OF_UNSIGNED_INT;

    dr_data = (unsigned int *)malloc(buffer_size);
    if(NULL == dr_data){
        printf("Failed to allocate memory\n");
        return -1;
    }

    rc = write_onebyte_instruction(JTAG_STATE_IDLE, CPD_PRELOAD);
    if(rc < 0){
        printf("Failed to write instruction CPD_PRELOAD\n");
        goto end_of_func;
    }
    memset(dr_data, 0xff, buffer_size);
    
    rc = write_data_register(JTAG_STATE_IDLE, dr_data, bits);
    if(rc < 0){
        printf("Failed to write data register\n");
        goto end_of_func;
    }

end_of_func:
    if(dr_data) free(dr_data);
    return rc;
}

int enter_configuration_mode(int mode)
{
    int rc;
    unsigned char inst;

    if(mode == CPLD_TRANSPARENT_MODE){
        printf("Transparent Mode\n");
        inst = ISC_ENABLE_X;
    }else{
        printf("Offline Mode\n");
        inst = ISC_ENABLE;
    }
    
    rc = write_onebyte_instruction(JTAG_STATE_IDLE, inst);
    if(rc < 0){
        goto end_of_func;
    }

    rc = write_onebyte_opcode(JTAG_STATE_IDLE, 0x8);
    if(rc < 0){
        goto end_of_func;
    }

    run_test_idle(0, JTAG_STATE_IDLE, 2);
    usleep(3000);

    /* Check status in 10s */
    rc = check_cpld_status(CPLD_STATUS_FAIL_BIT|0x40, 10);
    if(rc < 0){
        goto end_of_func;
    }

    /* Check status in 10s */
    rc = check_cpld_status(CPLD_STATUS_BUSY_BIT, 10);
    if(rc < 0){
        goto end_of_func;
    }

end_of_func:
    return rc;
}

int exit_configuration_mode()
{
    int rc;
    rc = write_onebyte_instruction(JTAG_STATE_IDLE, ISC_NOOP);
    if(rc < 0){
        printf("%s(%d) - failed to write instruction ISC_NOOP\n", __FUNCTION__, __LINE__);
        return -1;
    } 
    usleep(1000);
    rc = write_onebyte_instruction(JTAG_STATE_IDLE, ISC_DISABLE);
    if(rc < 0){
        printf("%s(%d) - failed to write instruction ISC_DISABLE\n", __FUNCTION__, __LINE__);
        return -1;
    } 
    
    usleep(1000);
    rc = write_onebyte_instruction(JTAG_STATE_IDLE, ISC_NOOP);
    if(rc < 0){
        printf("%s(%d) - failed to write instruction ISC_NOOP\n", __FUNCTION__, __LINE__);
        return -1;
    }

    return rc;
}

/*
 * option - select the cpld space need to be erased
 * num_of_loops - wait loops for erase
 */
int erase_cpld(unsigned char option, int num_of_loops)
{
    int rc;
    int i;
    unsigned int dr_data;
    rc = write_onebyte_instruction(JTAG_STATE_IDLE, ISC_ERASE);
    if(rc < 0){
        printf("%s(%d) - failed to write instruction ISC_ERASE\n", __FUNCTION__, __LINE__);
        return -1;
    } 

    rc = write_onebyte_opcode(JTAG_STATE_IDLE, option);
    if(rc < 0){
        printf("%s(%d) - failed to write opcode %02x\n",  __FUNCTION__, __LINE__, option);
        return -1;
    }

	rc = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_CHECK_BUSY);
    if(rc < 0){
        printf("%s(%d) - failed to write instruction LSC_CHECK_BUSY\n",  __FUNCTION__, __LINE__);
        return -1;
    }

	for (i = 0; i < num_of_loops ; i++) {
		usleep(2000);
		rc = read_data_register(JTAG_STATE_IDLE, &dr_data, 1);
        if(rc < 0) return rc;
	}

    rc = check_cpld_status(CPLD_STATUS_BUSY_BIT|CPLD_STATUS_FAIL_BIT, 5);
    if(rc < 0){
        printf("%s(%d) - check cpld status failure\n",  __FUNCTION__, __LINE__);
        return -1;
    }

    return 0;
}

int program_configuration(int bytes_per_page, int num_of_pages, progress_func_t progress)
{
    int rc = 0;
    int row;
    int i;
    unsigned int *page_buffer;
    int used_pages = 0;

    /* 
     * To reduce the CPLD program time, only program the used pages. 
     */
    used_pages = jedec_get_cfg_fuse()/(bytes_per_page*BITS_OF_ONE_BYTE);
    if(used_pages == 0){
        printf("%s(%d) - JEDEC file is damaged!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if(used_pages > num_of_pages){
        printf("%s(%d) - JEDEC file isn't used for this CPLD, used pages %d, actual pages %d!\n", 
            __FUNCTION__, __LINE__, used_pages, num_of_pages);
        return -1;
    }
    
    page_buffer = (unsigned int *)malloc(bytes_per_page);
    if(NULL == page_buffer){
        printf("%s(%d) - failed to allocate page buffer\n",  __FUNCTION__, __LINE__);
        return -1;
    }

    rc = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_INIT_ADDRESS);
    if(rc < 0){
        printf("%s(%d) - failed to write instruction LSC_INIT_ADDRESS\n",  __FUNCTION__, __LINE__);
        goto end_of_func;
    }

    rc = read_data_register(JTAG_STATE_IDLE, page_buffer, BITS_OF_UNSIGNED_INT);
    if(rc < 0){
        printf("%s(%d) - failed to write opcode\n",  __FUNCTION__, __LINE__);
        goto end_of_func;
    }
    
	for (row = 0 ; row < used_pages; row++){
        enum jtag_endstate endstate;
        rc = jtag_get_status(global_jtag_object, &endstate);
        if(endstate != JTAG_STATE_IDLE){
            rc = -1;
            break;
        }
        usleep(1000);

        for (i = 0; i < bytes_per_page/sizeof(unsigned int); i++) {
            page_buffer[i] = jedec_get_long(row*bytes_per_page/sizeof(unsigned int)+i);
        }

        rc = write_onebyte_instruction(JTAG_STATE_PAUSEIR, LSC_PROG_INCR_NV);
        if(rc < 0){
            printf("%s(%d) - failed to write instruction LSC_PROG_INCR_NV\n",  __FUNCTION__, __LINE__);
            goto end_of_func;
        }

        rc = write_data_register(JTAG_STATE_IDLE, page_buffer, bytes_per_page*BITS_OF_ONE_BYTE);
        if(rc < 0){
            printf("%s(%d) - failed to write data to cpld\n",  __FUNCTION__, __LINE__);
            goto end_of_func;
        }
        
        run_test_idle(0, JTAG_STATE_IDLE, 2);
		usleep(1000);

        rc = write_onebyte_instruction(JTAG_STATE_PAUSEIR, LSC_CHECK_BUSY);
        if(rc < 0){
            printf("%s(%d) - failed to write instruction LSC_CHECK_BUSY\n",  __FUNCTION__, __LINE__);
            goto end_of_func;
        }

		for (i = 0; i < 10; i++) {
			usleep(3000);
			page_buffer[0] = 0;
			read_data_register(JTAG_STATE_IDLE, page_buffer, 1);
			if (page_buffer[0] == 0) break;
		}

		if (page_buffer[0] != 0){
			printf("%S(%d) - Prgram failure row %d: failure data [%08x] \n", row, page_buffer[0]);
            rc = -1;
            break;
        } else {
            if(NULL != progress){
		        progress((row*100)/used_pages);
            }
        }
	}

end_of_func:
    if(page_buffer) free(page_buffer);
    return rc;
}

int program_user_code(unsigned int code)
{
    int rc;
    unsigned int user_code = code;

    rc = write_onebyte_instruction(JTAG_STATE_IDLE, CPD_USERCODE);
    if(rc < 0){
        printf("%s(%d) - failed to write onebyte instruction CPD_USERCODE\n", __FUNCTION__, __LINE__);
        return -1;
    }

    rc = write_data_register(JTAG_STATE_IDLE, &user_code, sizeof(code)*BITS_OF_UNSIGNED_INT);
    if(rc < 0){
        printf("%s(%d) - failed to write user code to data register\n", __FUNCTION__, __LINE__);
        return -1;
    }

    rc = write_onebyte_instruction(JTAG_STATE_IDLE, ISC_PROGRAM_USERCODE);
    if(rc < 0){
        printf("%s(%d) - failed to write instruction ISC_PROGRAM_USERCODE\n", __FUNCTION__, __LINE__);
        return -1;
    }
    usleep(2000);

    rc = check_cpld_status(CPLD_STATUS_BUSY_BIT|CPLD_STATUS_FAIL_BIT, 1);
    if(rc < 0){
        printf("%s(%d) - failed to check cpld status\n", __FUNCTION__, __LINE__);
        return -1;
    }
    
    return 0;   
}

int program_feature_row(unsigned int a0, unsigned int a1)
{
    int rc = 0;
    int i;
    unsigned int dr_data[2];

    rc = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_INIT_ADDRESS);
    if(rc < 0){
        printf("%s(%d) - failed to write onebyte instruction LSC_INIT_ADDRESS\n", __FUNCTION__, __LINE__);
        return -1;
    }

    rc = write_onebyte_opcode(JTAG_STATE_IDLE, 2);
    if(rc < 0){
        printf("%s(%d) - failed to write onebyte opcode\n", __FUNCTION__, __LINE__);
        return -1;
    }
    usleep(3000);

    rc = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_PROG_FEATURE);
    if(rc < 0){
        printf("%s(%d) - failed to write onebyte instruction LSC_PROG_FEATURE\n", __FUNCTION__, __LINE__);
        return -1;
    }

    dr_data[0] = a0;
    dr_data[1] = a1;
    rc = write_data_register(JTAG_STATE_IDLE, dr_data, 64);
    if(rc < 0){
        printf("%s(%d) - failed to write data register\n", __FUNCTION__, __LINE__);
        return -1;
    }

    rc = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_CHECK_BUSY);
    if(rc < 0){
        printf("%s(%d) - failed to write onebyte instruction\n", __FUNCTION__, __LINE__);
        return -1;
    }

	for (i = 0; i < 10; i++) {
		usleep(3000);
		dr_data[0] = 0;
		read_data_register(JTAG_STATE_IDLE, dr_data, 1);
	}

    rc = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_READ_FEATURE);
    if(rc < 0){
        printf("%s(%d) - failed to write onebyte instruction\n", __FUNCTION__, __LINE__);
        return -1;
    }

	dr_data[0] = 0x00000000;
	dr_data[1] = 0x00000000;
	rc = read_data_register(0, dr_data, 64);
    if(rc < 0){
        printf("%s(%d) - failed to read data register\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if((dr_data[0] != a0)||(dr_data[1] != a1)){
        printf("%s(%d) - feature row data dismatch, dr_data: %08x, %08x program data: %08x, %08x\n", \
            __FUNCTION__, __LINE__, dr_data[0], dr_data[1], a0, a1);
        return -1;
    }

    return 0;
}

int program_feabits(unsigned short feabits)
{
    int rc;
    int i;
    unsigned int dr_data = feabits;
    rc = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_PROG_FEABITS);
    if(rc < 0){
        printf("%s(%d) - failed to write onebyte instruction LSC_INIT_ADDRESS\n", __FUNCTION__, __LINE__);
        return -1;
    }

    rc = write_data_register(JTAG_STATE_IDLE, &dr_data, sizeof(feabits)*BITS_OF_UNSIGNED_INT);
    if(rc < 0){
        printf("%s(%d) - failed to write feabits 0x%04x\n", __FUNCTION__, __LINE__, feabits);
        return -1;
    }

    rc = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_CHECK_BUSY);
    if(rc < 0){
        printf("%s(%d) - failed to write onebyte instruction LSC_CHECK_BUSY\n", __FUNCTION__, __LINE__);
        return -1;
    }

	for (i = 0; i < 10; i++) {
		usleep(3000);
		dr_data = 0;
		rc = read_data_register(JTAG_STATE_IDLE, &dr_data, 1);
        if(rc < 0) return rc;
	}

    rc = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_READ_FEABITS);
    if(rc < 0){
        printf("%s(%d) - failed to write onebyte instruction LSC_READ_FEABITS\n", __FUNCTION__, __LINE__);
        return -1;
    }

	dr_data = 0x0;
	rc = read_data_register(JTAG_STATE_IDLE, &dr_data, sizeof(feabits)*BITS_OF_UNSIGNED_INT);
    if(dr_data != feabits) {
        printf("%s(%d) - %04x [%04x]\n", __FUNCTION__, __LINE__, dr_data&0xfff2, feabits);
        return -1;
    }

    return 0;
}

int programe_done()
{
    int i;
    int rc;
    unsigned int dr_data;

    rc = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_PROGRAM_DONE);
    if(rc < 0){
        printf("%s(%d) - failed to write onebyte instruction LSC_PROGRAM_DONE\n", __FUNCTION__, __LINE__);
        return -1;
    }

	rc = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_CHECK_BUSY);
    if(rc < 0){
        printf("%s(%d) - failed to write onebyte instruction LSC_CHECK_BUSY\n", __FUNCTION__, __LINE__);
        return -1;
    }

	for (i = 0; i < 10; i++) {
		usleep(3000);
		dr_data = 0;
		rc = read_data_register(0, &dr_data, 1);
        if(rc < 0) return rc;
	}

    return 0;
}


int verify_configuration(int bytes_per_page, int num_of_pages)
{
    int rc;
    int i;
    int row;
    int used_pages;
    unsigned int jed_data;
    unsigned int *page_buffer;

    /* 
     * To reduce the CPLD program time, only verify the used pages. 
     */
    used_pages = jedec_get_cfg_fuse()/(bytes_per_page*BITS_OF_ONE_BYTE);
    if(used_pages == 0){
        printf("%s(%d) - JEDEC file is damaged!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if(used_pages > num_of_pages){
        printf("%s(%d) - JEDEC file isn't used for this CPLD, used pages %d, total pages %d!\n", 
            __FUNCTION__, __LINE__, used_pages, num_of_pages);
        return -1;
    }
    
    page_buffer = (unsigned int *)malloc(bytes_per_page);
    if(NULL == page_buffer){
        printf("%s(%d) - failed to allocate page buffer\n",  __FUNCTION__, __LINE__);
        return -1;
    }

    rc = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_INIT_ADDRESS);
    if(rc < 0){
        printf("%s(%d) - failed to write onebyte instruction LSC_INIT_ADDRESS\n", __FUNCTION__, __LINE__);
        goto end_of_func;
    }

    rc = write_onebyte_opcode(JTAG_STATE_IDLE, 4);
    if(rc < 0){
        printf("%s(%d) - failed to write opcode\n", __FUNCTION__, __LINE__);
        goto end_of_func;
    }
    usleep(3000);

    rc = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_READ_INCR_NV);
    if(rc < 0){
        printf("%s(%d) - failed to write onebyte instruction LSC_READ_INCR_NV\n", __FUNCTION__, __LINE__);
        goto end_of_func;
    }
    
    run_test_idle(0, JTAG_STATE_IDLE, 2);
    usleep(3000);

    printf("Verify config pages: %d \n", used_pages);   
    
    for (row = 0 ; row < used_pages; row++) {
        memset(page_buffer, 0, bytes_per_page);
        read_data_register(JTAG_STATE_IDLE, page_buffer, bytes_per_page*BITS_OF_ONE_BYTE);
        for (i = 0; i < bytes_per_page/sizeof(unsigned int); i++) {
            jed_data = jedec_get_long(row*bytes_per_page/sizeof(unsigned int)+i);
            if (page_buffer[i] != jed_data) {
                printf("Row %d, Column %d - JED : %08x, SDR : %08x\n", row, i, jed_data, page_buffer[i]);
                rc = -1;
                break;
            }
        }
        if (rc < 0) break;
        usleep(3000);
    }
end_of_func:
    if(page_buffer) free(page_buffer);
    return rc;
}


/*
 * Force the MachXO3 to reconfigure. Transmitting
 * a REFRESH command reconfigures the
 * MachXO3 in the same fashion as asserting
 * PROGRAMN.
 * refer to MachXO3ProgrammingandConfigurationUsageGuide page 58
 */
int transmit_refresh()
{
    int rc;
    rc = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_REFRESH);
    if(rc < 0){
        printf("%s(%d) - failed to write instruction LSC_REFRESH\n", __FUNCTION__, __LINE__);
        return -1;
    }
    return rc;
}
