#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <facebook/bic.h>

#define USAGE_MESSAGE \
    "Usage:\n" \
    "  %s slot[0|1|2|3] --[1ou|2ou|bb] --[write|read] $image  \n"

int main(int argc, char **argv) {
  struct timeval start, end;
  int ret;
  int fru_size = 0;
  uint8_t intf;
  uint8_t slot_id = 1;
  uint8_t action = 0;
  printf("================================================\n");
  printf("Get the remote FRU of BIC,  v1.0\n");
  printf("Build date: %s %s \r\n", __DATE__, __TIME__);
  printf("================================================\n");

  if ( argc != 5 ) {
    printf(USAGE_MESSAGE, argv[0]);
    return 0;
  }

  gettimeofday(&start, NULL);

  if ( (strcmp(argv[1], "slot0") == 0) ) {
    slot_id = 0;
  } else if ( (strcmp(argv[1], "slot1") == 0) ) {
    slot_id = 1;
  } else if ( (strcmp(argv[1], "slot2") == 0) ) {
    slot_id = 2;
  } else if ( (strcmp(argv[1], "slot3") == 0) ) {
    slot_id = 3;
  } else {
    printf("Cannot recognize the unknown slot: %s\n", argv[1]);
    return -1;
  }

  if ( (strcmp(argv[2], "--1ou") == 0) ) {
    intf = 0x05;
  } else if ( (strcmp(argv[2], "--2ou") == 0) ) {
    intf = 0x15;
  } else if ( (strcmp(argv[2], "--bb") == 0) ) {
    intf = 0x10;
  } else {
    printf("Cannot recognize the unknown inteface: %s\n", argv[2]);
    return -1;
  }

  if ( (strcmp(argv[3], "--write") == 0) ) {
    action = 1;
  } else if ( (strcmp(argv[3], "--read") == 0) ) {
    action = 0;
  } else {
    printf("Cannot recognize the action of %s\n", argv[3]);
    return -1;
  }


  if ( action == 1 ) {
    bic_write_fruid_param(slot_id, 0, argv[4], intf);
  } else {
    bic_read_fruid_param(slot_id, 0, argv[4], &fru_size, intf);
  }

  gettimeofday(&end, NULL);
  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));

  return 0;
}
