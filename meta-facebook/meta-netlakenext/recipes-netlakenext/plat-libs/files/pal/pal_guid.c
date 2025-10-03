#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <syslog.h>
#include <string.h>
#include <ctype.h>
#include "pal.h"
#include "pal_guid.h"

#define GUID_SIZE 16
#define OFFSET_SYS_GUID 0x17F0
#define OFFSET_DEV_GUID 0x1800

int
pal_get_guid(uint16_t offset, char *guid) {

  int fd = 0;
  ssize_t bytes_rd;
  char eeprom_path[128] = {0};

  errno = 0;

  snprintf(eeprom_path, sizeof(eeprom_path), EEPROM_PATH, I2C_SERVER_BUS, SERVER_FRU_ADDR);

  // Check if file is present
  if (access(eeprom_path, F_OK) == -1) {
      syslog(LOG_ERR, "pal_get_dev_guid: unable to access the %s file: %s",
          eeprom_path, strerror(errno));
      return errno;
  }

  // Open the file
  fd = open(eeprom_path, O_RDONLY);
  if (fd == -1) {
    syslog(LOG_ERR, "pal_get_dev_guid: unable to open the %s file: %s",
        eeprom_path, strerror(errno));
    return errno;
  }

  // seek to the offset
  lseek(fd, offset, SEEK_SET);

  // Read bytes from location
  bytes_rd = read(fd, guid, GUID_SIZE);
  if (bytes_rd != GUID_SIZE) {
    syslog(LOG_ERR, "pal_get_dev_guid: read to %s file failed: %s",
        eeprom_path, strerror(errno));
    goto err_exit;
  }

err_exit:
  close(fd);

  return errno;
}

static int
pal_set_guid(uint16_t offset, char *guid) {
  int fd = 0;
  ssize_t bytes_wr;
  char eeprom_path[128] = {0};

  errno = 0;

  snprintf(eeprom_path, sizeof(eeprom_path), EEPROM_PATH, I2C_SERVER_BUS, SERVER_FRU_ADDR);

  // Check for file presence
  if (access(eeprom_path, F_OK) == -1) {
      syslog(LOG_ERR, "pal_set_guid: unable to access the %s file: %s",
          eeprom_path, strerror(errno));
      return errno;
  }

  // Open file
  fd = open(eeprom_path, O_WRONLY);
  if (fd == -1) {
    syslog(LOG_ERR, "pal_set_guid: unable to open the %s file: %s",
        eeprom_path, strerror(errno));
    return errno;
  }

  // Seek the offset
  lseek(fd, offset, SEEK_SET);

  // Write GUID data
  bytes_wr = write(fd, guid, GUID_SIZE);
  if (bytes_wr != GUID_SIZE) {
    syslog(LOG_ERR, "pal_set_guid: write to %s file failed: %s",
        eeprom_path, strerror(errno));
    goto err_exit;
  }

err_exit:
  close(fd);
  return errno;
}

int
pal_get_dev_guid(uint8_t fru, char *guid) {
  char dev_guid[GUID_SIZE] = {0};

  pal_get_guid(OFFSET_DEV_GUID, dev_guid);

  memcpy(guid, dev_guid, GUID_SIZE);

  return 0;
}

int
pal_get_sys_guid(uint8_t fru, char *guid) {
  char sys_guid[GUID_SIZE] = {0};

  pal_get_guid(OFFSET_SYS_GUID, sys_guid);

  memcpy(guid, sys_guid, GUID_SIZE);

  return 0;
}

int
pal_set_dev_guid(uint8_t fru, char *str) {
  char dev_guid[GUID_SIZE] = {0};

  pal_populate_guid(dev_guid, str);

  return pal_set_guid(OFFSET_DEV_GUID, dev_guid);
}

int
pal_set_sys_guid(uint8_t fru, char *str) {
  char sys_guid[GUID_SIZE] = {0};

  pal_populate_guid(sys_guid, str);

  return pal_set_guid(OFFSET_SYS_GUID, sys_guid);
}
