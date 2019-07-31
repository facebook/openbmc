#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <openbmc/obmc-i2c.h>
#include "max10_i2c_update.h"

#define I2C_FILE_NAME "/dev/i2c-9"
#define CPLD_ADDR 0x80

int set_i2c_register(int file,
                     unsigned char addr,
                     int reg,
                     int value)
{
    int ret;
    uint8_t txbuf[8] = {0};
    uint8_t rxbuf[8] = {0};
    uint8_t txlen = 0;
    uint8_t rxlen = 0;

#if 0
    if (ioctl(file, I2C_SLAVE, addr) < 0) {
      /* ERROR HANDLING; you can check errno to see what went wrong */
      exit(1);
    }
#endif

    // write register  4 bytes
    txbuf[0] = (reg >> 24 ) & 0xFF;
    txbuf[1] = (reg >> 16 ) & 0xFF;
    txbuf[2] = (reg >> 8 )  & 0xFF;
    txbuf[3] = (reg >> 0 )  & 0xFF;
    txbuf[4] = (value >> 0 ) & 0xFF;
    txbuf[5] = (value >> 8 ) & 0xFF;
    txbuf[6] = (value >> 16 )  & 0xFF;
    txbuf[7] = (value >> 24 )  & 0xFF;
    txlen = 8;

    ret = i2c_rdwr_msg_transfer(file, addr, txbuf, txlen, rxbuf, rxlen);
    if ( ret ) {
        printf("%s() write register fails. ret=%d\n", __func__, ret);
    }

    return (ret == 0)?8:0;
}

int get_i2c_register(int file,
                            unsigned char addr,
                            int reg,
                            int *val)
{
    int ret;
    uint8_t txbuf[8] = {0};
    uint8_t rxbuf[8] = {0};
    uint8_t txlen = 0;
    uint8_t rxlen = 0;
    unsigned int readval;


    // write register  4 bytes
    txbuf[0] = (reg >> 24 ) & 0xFF;
    txbuf[1] = (reg >> 16 ) & 0xFF;
    txbuf[2] = (reg >> 8 )  & 0xFF;
    txbuf[3] = (reg >> 0 )  & 0xFF;
    txlen = 4;
    rxlen = 4;

    ret = i2c_rdwr_msg_transfer(file, addr, txbuf, txlen, rxbuf, rxlen);
    if ( ret ) {
        printf("%s() get register fails. ret=%d\n", __func__, ret);
    }

    readval =  (rxbuf[0] << 0) ;
    readval |= (rxbuf[1] << 8) ;
    readval |= (rxbuf[2] << 16);
    readval |= (rxbuf[3] << 24);

    *val = readval;
    return (ret == 0)?4:0;
}

#define USAGE_MESSAGE \
    "Usage:\n" \
    "  %s [rpd_filename] [flash sector] [cfm_start_addr] [cfm_end_addr]  \n" \
    "  flash sector = {cfm0, cfm1, cfm2, ufm0, ufm1} \r\n" \
    "  example: %s app1_cfm1.rpd cfm2 0x10000  0x6FFFF \n\n"

static void usage(char *str )
{
	printf(USAGE_MESSAGE, str, str);

}

SectorType_t parse_sectorType(char* str)
{
	SectorType_t type = Sector_NotSet;

	if( strcmp(str, "cfm0") == 0)
		type = Sector_CFM0;
	else if( strcmp(str, "cfm1") == 0)
		type = Sector_CFM1;
	else if( strcmp(str, "cfm2") == 0)
		type = Sector_CFM2;
	else if( strcmp(str, "ufm0") == 0)
		type = Sector_UFM0;
	else if( strcmp(str, "ufm1") == 0)
		type = Sector_UFM1;
	else
		printf(" ERROR:  sector type[%s] not supported. \n", str);

	return type;
}

int main(int argc, char **argv) {
    int i2c_file;
    int rpd_file;
    unsigned char* rpd_file_buff;
    int cfm_start_addr, cfm_end_addr;

    int rpd_filesize;
    struct stat finfo;
    int readbytes;
    struct timeval start, end;
    SectorType_t sectorType = Sector_NotSet;
    int ret;

    printf("================================================\n");
    printf("Update MAX10 internal flash via I2C interface,  v1.0\n");
    printf("Build date: %s %s \r\n", __DATE__, __TIME__);
    printf("================================================\n");
	
    gettimeofday(&start, NULL);

    if(argc != 5)
    {
    	usage(argv[0]);
    	return 0;
    }

    // extract parameter
    // sector type
    sectorType = parse_sectorType(argv[2]);
    if(sectorType == Sector_NotSet)
    {
        printf("\r\n sector type shuold be one of these[cfm0, cfm1, cfm2, ufm0, ufm1]. \r\n");
    	return 0;
    }

    sscanf(argv[3], "%X", &cfm_start_addr);
    sscanf(argv[4], "%X", &cfm_end_addr);


    // Open file
    printf("Open file: %s ", argv[1]);
    if ((rpd_file = open(argv[1], O_RDONLY)) < 0)
    {
        printf(" Fail to open file: %s .\r\n ", argv[1]);
        return -2;
    }

    // Get file size
    fstat(rpd_file, &finfo);
    rpd_filesize = finfo.st_size;

    printf(",file size = %d bytes ", rpd_filesize);

    // allocate memory
    rpd_file_buff = malloc(rpd_filesize);
    if(rpd_file_buff == 0)
    {
        printf("allocate memory fail. \r\n");
       	return -3;
    }

    //read rpt file into memory 
    readbytes = read(rpd_file, rpd_file_buff, rpd_filesize);
    printf("read %d bytes. \n", readbytes);

    // close file
    close(rpd_file);

    // Open a connection to the I2C userspace control file.
    if ((i2c_file = open(I2C_FILE_NAME, O_RDWR)) < 0) {
        perror("Unable to open i2c control file");
        exit(1);
    }

    Max10Update_Init(i2c_file, CPLD_ADDR);

// TEST  trig reconfig
#if 0
	Max10_TrigReconfig();
#endif

#if 0    
	Max10_Test();
	
	while(1)
	 ;
#endif

//	cfm_start_addr = 0x1C800;   // CFM1
//	cfm_end_addr   = 0x2AFFF;

//	cfm_start_addr = 0x2B000;   // CFM0
//	cfm_end_addr   = cfm_start_addr + rpd_filesize;

    printf("cfm_start_addr = 0x%X, cfm_end_addr = 0x%X.\n", cfm_start_addr  , cfm_end_addr);

    Max10_Update_Rpd(rpd_file_buff, sectorType, cfm_start_addr, cfm_end_addr);

    gettimeofday(&end, NULL);
    printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));

    ret = close(i2c_file);

    // free memory
    free(rpd_file_buff);
    return 0;
}
