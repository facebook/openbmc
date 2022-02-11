#ifndef _RACKMOND_H_
#define _RACKMOND_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <termios.h>

#define RACKMON_IPC_SOCKET "/var/run/rackmond.sock"
#define RACKMON_STAT_STORE "/var/log/psu-status.log"

//would've been nice to have thrift

// Raw modbus command
// Response is just the raw response data
typedef struct raw_modbus_command {
  uint16_t length;
  uint16_t expected_response_length;
  uint32_t custom_timeout; // 0 for default
  char data[1];
} raw_modbus_command;

// only store new value if different from most recent
// (for watching changes to status flags registers)
#define MONITOR_FLAG_ONLY_CHANGES 0x1

/*
 * "monitor_interval" doesn't refer to time interval, it defines a section
 * or register space in PSU's register map.
 *  - "begin" defines the register start address
 *  - "len" is the register count
 *  - "keep" tells rackmond how many sets of the data (register values)
 *    need to be stored in memory.
 */
typedef struct monitor_interval {
  uint16_t begin;
  uint16_t len;
  uint16_t keep; // How long of a history to keep?
  uint16_t flags;
} monitor_interval;

typedef struct monitoring_config {
  uint16_t num_intervals;
  monitor_interval intervals[1];
} monitoring_config;

typedef struct set_config_command {
  monitoring_config config;
} set_config_command;

enum {
  COMMAND_TYPE_NONE = 0,
  COMMAND_TYPE_RAW_MODBUS,
  COMMAND_TYPE_SET_CONFIG,
  COMMAND_TYPE_DUMP_DATA_JSON,
  COMMAND_TYPE_PAUSE_MONITORING,
  COMMAND_TYPE_START_MONITORING,
  COMMAND_TYPE_DUMP_STATUS,
  COMMAND_TYPE_FORCE_SCAN,
  COMMAND_TYPE_DUMP_DATA_INFO,
  COMMAND_TYPE_MAX,
};

typedef struct rackmond_command {
  uint16_t type;
  union {
    raw_modbus_command raw_modbus;
    set_config_command set_config;
  };
} rackmond_command;

typedef struct {
  monitor_interval* i;
  void* mem_begin;
  size_t mem_pos;
} reg_range_data_t;

typedef struct {
  uint8_t addr;
  uint32_t crc_errors;
  uint32_t timeout_errors;
  speed_t baudrate;
  bool supports_baudrate;
  int consecutive_failures;
  bool timeout_mode;
  time_t last_comms;
  reg_range_data_t range_data[1];
} psu_datastore_t;

// write buf to send response text the back to client
typedef struct {
  char* buffer;
  size_t len;
  size_t pos;
  int fd;
} write_buf_t;

int buf_printf(write_buf_t* buf, const char* format, ...);

#endif /* _RACKMOND_H_ */
