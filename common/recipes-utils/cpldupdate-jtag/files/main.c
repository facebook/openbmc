/*
 * main - CPLD upgrade utility via BMC's JTAG master
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
#include <stdbool.h>
#include <sys/mman.h>
#include <pthread.h>
#include "jedecfile.h"
#include "jtag_common.h"
#include "jtag_interface.h"
#include "lattice_cpld.h"

typedef struct {
    int programming_mode;           /* Transparent mode or offline mode */
    int freqency;                   /* if this value is 0, use the default freqency */
    int program;                    /* enable/disable program  */
    int verification;               /* enable/disable verification flag */
    int read;                       /* enable/disable read flag */
    int erase;                      /* enable/disable erase flag */
    int debug;                      /* enable/disable debug flag */
    int refresh;                    /* refresh device */
    jtag_object_t *pjtag_object;    /* hardware interface (JTAG interface) */
    cpld_device_t *pcpld_device;    /* CPLD device related */
}cpld_t;

static void usage(FILE *fp, int argc, char **argv)
{
    fprintf(fp,
            "Usage: %s [options]\n\n"
            "Options:\n"
            " -h | --help                   Print this message\n"
            " -e | --erase                  Erase cpld\n"
            " -p | --program                Program cpld and verify\n"
            " -v | --verify                 verifiy cpld image with file\n"
            " -d | --debug                  debug mode\n"
            " -f | --frequency              frequency\n"
            " -o | --option                 Program option\n"
            " -m | --mode                   Offline mode\n"
            "",
            argv[0]);
}

static const char short_options [] = "dhmeRp:v:f:o:";

static const struct option
    long_options [] = {
    { "help",       no_argument,        NULL,    'h' },
    { "erase",      no_argument,        NULL,    'e' },
    { "program",    required_argument,  NULL,    'p' },
    { "verify",     required_argument,  NULL,    'v' },
    { "debug",      no_argument,        NULL,    'd' },
    { "fequency",   required_argument,  NULL,    'f' },
    { "option",     required_argument,  NULL,    'o' },
    { "mode",       no_argument,        NULL,    'm' },   
    { 0, 0, 0, 0 }
};

static unsigned short prog_option = 0;
static int debug = 1;

static int print_progess_bar(long pecent)
{
    int i = 0;
    static int prev_pecent = 0;

    if(prev_pecent == pecent){
        /* To avoid the duplicated print */
        return 0;
    }else{
        prev_pecent = pecent;
    }

    printf("\033[?251");
    printf("\r");

    for(i = 0; i < pecent / 2; i++) {
        printf("\033[;44m \033[0m");
    }

    for(i = pecent / 2; i < 50; i++) {
        printf("\033[47m \033[0m");
    }

    printf(" [%d%%]", pecent);
    fflush(stdout);
    if(pecent == 100) {
        printf("\n");
    }

    return 0;
}

static void printf_pass()
{
    printf("+=======+\n" );
    printf("| PASS! |\n" );
    printf("+=======+\n\n" );
}

static void printf_failure()
{
    printf("+=======+\n" );
    printf("| FAIL! |\n" );
    printf("+=======+\n\n" );
}

int check_file(const char *file_name)
{
    FILE *fp = NULL;
    int rc;
    static int is_checked = 0;

    if(is_checked){
        return 0;
    }
    
    fp = fopen(file_name, "rb");
    if (!fp) {
        printf("%s(%d) - failed to open file %s.\n", __FILE__, __LINE__, file_name);
        return -1;
    }

    jedec_init();
    rc = jedec_read_file(fp);
    if(rc < 0){
        printf("%s(%d) - failed to parse jed file %s.\n", __FILE__, __LINE__, file_name);
        goto end_of_func;
    }
    printf("JED file parser\n");
    printf("Device %s: %d Fuses\n", jedec_get_device(), jedec_get_length());    
    printf("Version : %s Date %s\n", jedec_get_version(), jedec_get_date());
    printf("-----------------------------------------------------------------\n");
    
    if(jedec_calc_checksum() != jedec_get_checksum()){
        printf("%s(%d) - Checksum doesn't match, JED file is corrupt.\n", __FILE__, __LINE__);
        rc = -1;
        goto end_of_func;
    }
    is_checked = 1;
    return 0;

end_of_func:
    jedec_uninit();
    if(fp) fclose(fp);
    return rc;
}

int cpld_init(cpld_t *pcpld)
{
    if(NULL == pcpld) {
        return -1;
    }

    if(NULL != pcpld->pjtag_object){
        return 0;
    }

    pcpld->pjtag_object = jtag_hardware_mode_init("/dev/jtag0");
    if(NULL == pcpld->pjtag_object) {
        return -1;
    }

    return 0;
}

int cpld_program(cpld_t *pcpld, const char *file_name)
{
    int rc = 0;
    int option = CPLD_OPCODE_ERASE_CONF_FLASH_BIT|CPLD_OPCODE_ERASE_UFM;
    rc = cpld_init(pcpld);
    if(rc < 0){
        return -1;
    }

    rc = check_file(file_name);
    if(rc < 0){
        printf("JED file %s is invalid file\n", file_name);
        return -1;
    }

    /* Reset and put the JTAG state machine in IDLE status */
    run_test_idle(1, JTAG_STATE_IDLE, 3);
    usleep(3000);

    pcpld->pcpld_device = scan_cpld_device();
    if(NULL == pcpld->pcpld_device){
        printf("%s(%d) - Failed to find CPLD\n", __FUNCTION__, __LINE__);
        rc = -1;
        goto end_of_func;
    }

    /* Check if the CPLD is ready to accept the command */
    rc = check_cpld_status(CPLD_STATUS_BUSY_BIT, 10);
    if(rc < 0){
        goto end_of_func;
    }

    rc = preload();
    if(rc < 0){
        printf("%s(%d) - failed to enter configuration mode\n", __FUNCTION__, __LINE__);
        goto end_of_func;
    }

    rc = enter_configuration_mode(pcpld->programming_mode);
    if(rc < 0){
        printf("%s(%d) - failed to enter configuration mode\n", __FUNCTION__, __LINE__);
        goto end_of_func;
    }

    printf("Erase/Program the Flash ...\n");

    rc = erase_cpld(option, 1500);
    if(rc < 0){
        printf("%s(%d) - failed to erase cpld\n", __FUNCTION__, __LINE__);
        goto end_of_func;
    }

    rc = program_configuration(pcpld->pcpld_device->page_bits/BITS_OF_ONE_BYTE, 
         pcpld->pcpld_device->num_of_cfg_pages,
         print_progess_bar);
    if(rc < 0){
        printf("%s(%d) - failed to program configuration\n", __FUNCTION__, __LINE__);
        goto end_of_func;
    }

    rc = program_user_code(0);
    if(rc < 0){
        printf("%s(%d) - failed to program user code\n", __FUNCTION__, __LINE__);
        goto end_of_func;
    }

    if(option&CPLD_OPCODE_ERASE_FEATURE_BIT){
        rc = program_feature_row(0, 0);
        if(rc < 0){
            printf("%s(%d) - failed to program feature row\n", __FUNCTION__, __LINE__);
            goto end_of_func;
        }

        rc = program_feabits(0x620);
        if(rc < 0){
            printf("%s(%d) - failed to program feabits\n", __FUNCTION__, __LINE__);
            goto end_of_func;
        }
    }

    rc = programe_done();
    if(rc < 0){
        printf("%s(%d) - failed to program done\n", __FUNCTION__, __LINE__);
        goto end_of_func;
    }

    rc = exit_configuration_mode();

end_of_func:
    if(rc == 0) print_progess_bar(100);
    return rc;
}

int cpld_erase(cpld_t *pcpld, unsigned char flash_block){
    int rc;
    unsigned int dr_data;
    unsigned int status;
    int i;
    
    rc = cpld_init(pcpld);
    if(rc < 0){
        return -1;
    }

    /* 1. JTAG go to IDLE status */
    run_test_idle(1, JTAG_STATE_IDLE, 3);
    usleep(3000);

    /* 2. Get device ID, and check if the CPLD id is supported */
    pcpld->pcpld_device = scan_cpld_device();
    if(NULL == pcpld->pcpld_device){
        return -1;
    }

    run_test_idle(0, JTAG_STATE_IDLE, 3);
    usleep(3000);
    
    /* 3. Check if the CPLD is ready to accept the command */
    rc = check_cpld_status(CPLD_STATUS_BUSY_BIT, 10);
    if(rc < 0){
        goto end_of_func;
    }

    rc = enter_configuration_mode(pcpld->programming_mode);
    if(rc < 0){
        goto end_of_func;
    }

    rc = erase_cpld(flash_block, 1500);

    rc = exit_configuration_mode();

end_of_func:
    return rc;
}

int cpld_verify(cpld_t *pcpld, const char *file_name)
{
    int rc = 0;
    
    rc = cpld_init(pcpld);
    if(rc < 0){
        return -1;
    }

    rc = check_file(file_name);
    if(rc < 0){
        printf("%s(%d) - JED file %s is invalid file\n", __FUNCTION__, __LINE__, file_name);
        return -1;
    }

    /* Put the CPLD to IDLE status */
    run_test_idle(1, JTAG_STATE_IDLE, 3);
    usleep(3000);

    pcpld->pcpld_device = scan_cpld_device();
    if(NULL == pcpld->pcpld_device){
        printf("%s(%d) - Failed to find CPLD\n", __FUNCTION__, __LINE__);
        rc = -1;
        goto end_of_func;
    }
    
    rc = preload();
    if(rc < 0){
        printf("%s(%d) - Failed to preload data\n", __FUNCTION__, __LINE__);
        goto end_of_func;
    }

    if(pcpld->programming_mode == CPLD_TRANSPARENT_MODE){
        write_onebyte_instruction(JTAG_STATE_IDLE, ISC_ENABLE_X);
        write_onebyte_opcode(JTAG_STATE_IDLE, 8);
        usleep(3000);
    }else{
        write_onebyte_instruction(JTAG_STATE_IDLE, ISC_ENABLE);
        write_onebyte_opcode(JTAG_STATE_IDLE, 0);
        usleep(3000);

        write_onebyte_instruction(JTAG_STATE_IDLE, ISC_ERASE);
        write_onebyte_opcode(JTAG_STATE_IDLE, 1);
        usleep(1000);

        write_onebyte_instruction(JTAG_STATE_IDLE, ISC_NOOP);
        write_onebyte_instruction(JTAG_STATE_IDLE, ISC_ENABLE);
        write_onebyte_opcode(JTAG_STATE_IDLE, 8);
        usleep(3000);
    }

    printf("Starting to Verify Device . . . This will take a few seconds\n");
    rc = verify_configuration(pcpld->pcpld_device->page_bits/BITS_OF_ONE_BYTE, 
         pcpld->pcpld_device->num_of_cfg_pages);
    if(rc < 0){
        printf("%s(%d) - Failed to verify CPLD!\n", __FUNCTION__, __LINE__);
        rc = -1;
        goto end_of_func;
    }
    
    rc = exit_configuration_mode();

end_of_func:
    return rc;
}

int cpld_read(cpld_t *pcpld, const char *file_name)
{
    int i;
    unsigned int *dr_data;
    unsigned int jed_data;
    unsigned int row  = 0;
    int bytes_per_page = 0;
    int rc = 0;
    
    rc = cpld_init(pcpld);
    if(rc < 0){
        return -1;
    }

    rc = check_file(file_name);
    if(rc < 0){
        printf("%s(%d) - JED file %s is invalid file\n", file_name);
        return -1;
    }

    run_test_idle(1, JTAG_STATE_IDLE, 15);
    usleep(3000);

    pcpld->pcpld_device = scan_cpld_device();
    if(NULL == pcpld->pcpld_device){
        printf("%s(%d) - Failed to find CPLD\n", __FILE__, __LINE__);
        rc = -1;
        goto end_of_func;
    }
    
    rc = preload();
    if(rc < 0){
        printf("%s(%d) - Failed to Preload data\n", __FILE__, __LINE__);
        goto end_of_func;
    }

    write_onebyte_instruction(JTAG_STATE_IDLE, ISC_ENABLE);
    write_onebyte_opcode(JTAG_STATE_IDLE, 0);
    usleep(3000);

    write_onebyte_instruction(JTAG_STATE_IDLE, ISC_ERASE);
    write_onebyte_opcode(JTAG_STATE_IDLE, 1);
    usleep(1000);
    write_onebyte_instruction(JTAG_STATE_IDLE, ISC_NOOP);

    write_onebyte_instruction(JTAG_STATE_IDLE, ISC_ENABLE);
    write_onebyte_opcode(JTAG_STATE_IDLE, 8);
    usleep(3000);

    printf("Starting to read Device . . . This will take a few seconds\n");

    write_onebyte_instruction(JTAG_STATE_IDLE, LSC_INIT_ADDRESS);
    write_onebyte_opcode(JTAG_STATE_IDLE, 4);
    usleep(3000);

    rc = write_onebyte_instruction(JTAG_STATE_IDLE, LSC_READ_INCR_NV);
    if(rc < 0){
        printf("%s(%d) - failed to write instruction LSC_READ_INCR_NV\n", __FILE__, __LINE__);
        goto end_of_func;
    }
    run_test_idle(0, JTAG_STATE_IDLE, 2);
    usleep(3000);

    printf("Read config pages: %d \n", pcpld->pcpld_device->num_of_cfg_pages);
    bytes_per_page = (pcpld->pcpld_device->page_bits/ 8);
    
    FILE *cpld_fp = fopen("cpld_data", "w");
    FILE *jed_fp = fopen("jed_data", "w");
    fseek(cpld_fp, 0, SEEK_SET);
    fseek(jed_fp, 0, SEEK_SET);
    for (row = 0 ; row < pcpld->pcpld_device->num_of_cfg_pages; row++) {
        memset(dr_data, 0, bytes_per_page);
        read_data_register(JTAG_STATE_IDLE, dr_data, pcpld->pcpld_device->page_bits);
        for (i = 0; i < bytes_per_page/sizeof(unsigned int); i++) {
            jed_data = jedec_get_long(row*bytes_per_page/sizeof(unsigned int)+i);
            fwrite(&dr_data[i], 1, 4, cpld_fp);
            fwrite(&jed_data, 1, 4, jed_fp);
        }
        usleep(3000);
    }
    fclose(cpld_fp);
    fclose(jed_fp);

    rc = exit_configuration_mode();

end_of_func:
    if(dr_data) free(dr_data);
    return rc;
}

int main(int argc, char *argv[]){
    char option;
    char in_name[100]  = "";
    char out_name[100] = "";

    cpld_t cpld;
    int rc;

    printf( "\n******************************************************************\n" );
    printf( "                 Celestica Corp.\n" );
    printf( "\n             cpldprog-v1.0.0 Copyright 2019.\n");  
    printf( "\nFor daisy chain programming of all in-system programmable devices \n" );
    printf( "********************************************************************\n\n" );

    if(argc <= 2) {
        usage(stdout, argc, argv);
        exit(EXIT_SUCCESS);
    }
    memset(&cpld, 0, sizeof(cpld));
    cpld.programming_mode = CPLD_TRANSPARENT_MODE; /* Default is transparent mode */
    
    while ((option = getopt_long(argc, argv, short_options, long_options, NULL)) != (char) -1) {
        switch (option) {
        case 'h':
            usage(stdout, argc, argv);
            exit(EXIT_SUCCESS);
            break;
        case 'e':
            cpld.erase = 1;
            break;
        case 'p':
            cpld.program = 1;
            strcpy(in_name, optarg);
            if (!strcmp(in_name, "")) {
                printf("No input file name!\n");
                usage(stdout, argc, argv);
                exit(EXIT_FAILURE);
            }
            break;
        case 'v':
            cpld.verification = 1;
            strcpy(in_name, optarg);
            if (!strcmp(in_name, "")) {
                printf("No input file name!\n");
                usage(stdout, argc, argv);
                exit(EXIT_FAILURE);
            }
            break;
        case 'r':
            cpld.read = 1;
            strcpy(in_name, optarg);
            if (!strcmp(in_name, "")) {
                printf("No input file name!\n");
                usage(stdout, argc, argv);
                exit(EXIT_FAILURE);
            }
            break;
        case 'd':
            cpld.debug = 1;
            break;
        case 'f':
            cpld.freqency = atoi(optarg);
            break;
        case 'o':
            prog_option = atoi(optarg);
            printf("program option 0x%04x\n", prog_option);
            break;
        case 'm':
            cpld.programming_mode = CPLD_OFFLINE_MODE;
            break;

        case 'R':
            cpld.refresh = 1;
            break;

        default:
            usage(stdout, argc, argv);
            exit(EXIT_FAILURE);
        }
    }

    if (cpld.erase) {
        rc = cpld_erase(&cpld, CPLD_OPCODE_ERASE_CONF_FLASH_BIT);
        if(rc < 0) {
            printf("Failed to erase cpld\n");
            goto end_of_func;
        }
    }

    if (cpld.program) {
        rc = cpld_program(&cpld, in_name);
        if(rc < 0) {
            printf("Failed to program cpld\n");            
            goto end_of_func;
        }
        goto cpld_verification;
    }

    if (cpld.verification) {
cpld_verification:
        rc = cpld_verify(&cpld, in_name);
        if(rc < 0) {
            printf("Failed to verify cpld\n");
        }
        goto end_of_func;
    }

    if (cpld.read) {
        rc = cpld_read(&cpld, out_name);
        if(rc < 0) {
            printf("Failed to read cpld\n");
        }
        goto end_of_func;
    }

    if(cpld.refresh)
    {
        rc = transmit_refresh();
        if(rc < 0){
            printf("%s(%d) - failed to transmit refesh\n", __FUNCTION__, __LINE__);
            printf_failure();
            return -1;
        }
    }

end_of_func:
    /*
     * The return value is kept consistent the ispvm tool.
     */
    if(rc == 0) {
        printf_pass();
        return 0;
    }else{
        printf_failure();
        return 1;
    }
}
