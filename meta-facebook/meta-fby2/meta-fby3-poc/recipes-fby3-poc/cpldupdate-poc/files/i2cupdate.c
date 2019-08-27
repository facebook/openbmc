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
#include "max10_i2c_update.h"

#define USAGE_MESSAGE \
        "Usage:\n" \
        "  %s --bb $filename.rpd   \n"\
        "  %s --sb slot[1|2|3|4] $filename.rpd \n\n"

static void usage(char *str )
{
  printf(USAGE_MESSAGE, str, str);
}

//We only update CFM0 in this project
#if 0
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
#endif

int main(int argc, char **argv) {
  int rpd_file;
  unsigned char *rpd_file_buff;
  int rpd_filesize;
  struct stat finfo;
  int readbytes;
  struct timeval start, end;
  int ret;
  bool bypass_update = false;
  bool show_ver = false;
  char input_file[80];
  int slot_id = 0;

  printf("================================================\n");
  printf("Update MAX10 internal flash via I2C interface,  v1.0\n");
  printf("Build date: %s %s \r\n", __DATE__, __TIME__);
  printf("================================================\n");

  gettimeofday(&start, NULL);

  if(argc < 3) {
    usage(argv[0]);
    return 0;
  }

  // extract parameter
  if ( strcmp(argv[1], "--bb") == 0 ) {
    bypass_update = false;
    if ( strcmp(argv[2], "--show") == 0 ) {
      show_ver = true;
    } else {
      strcpy(input_file, argv[2]);
    }
  } else if ( strcmp(argv[1], "--sb") == 0 ) {
    bypass_update = true;
    if ( (strcmp(argv[2], "slot1") == 0) ) {
      slot_id = 1;
    } else if ( (strcmp(argv[2], "slot2") == 0) ) {
      slot_id = 2;
    } else if ( (strcmp(argv[2], "slot3") == 0) ) {
      slot_id = 3;
    } else if ( (strcmp(argv[2], "slot4") == 0) ) {
      slot_id = 4;
    } else {
      printf("Cannot recognize the unknown slot: %s\n", argv[2]);
      return -1;
    }

    if ( strcmp(argv[3], "--show") == 0 ) {
      show_ver = true;
    } else {
      strcpy(input_file, argv[3]);
    }
  } else {
    printf("Cannot recognize the param - %s\n", argv[1]);
    return 0;
  }

  Max10Update_Init(bypass_update, slot_id);


  if ( show_ver == false) {
    // Open file
    printf("Open file: %s ", input_file);
    if ((rpd_file = open(input_file, O_RDONLY)) < 0) {
      printf(" Fail to open file: %s.\n ", input_file);
      return -2;
    }

    // Get file size
    fstat(rpd_file, &finfo);
    rpd_filesize = finfo.st_size;

    printf(",file size = %d bytes ", rpd_filesize);

    // allocate memory
    rpd_file_buff = malloc(rpd_filesize);
    if(rpd_file_buff == 0) {
      printf("allocate memory fail.\n");
      return -3;
    }

    //read rpt file into memory
    readbytes = read(rpd_file, rpd_file_buff, rpd_filesize);
    printf("read %d bytes.\n", readbytes);

    // close file
    close(rpd_file);

    Max10_Update_Rpd(rpd_file_buff, Sector_CFM0);
  } else {
    Max10_Show_Ver();
  }

  gettimeofday(&end, NULL);
  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));

  // free memory
  free(rpd_file_buff);
  return 0;
}
