#include <stdint.h>

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

#define COMMAND_TYPE_RAW_MODBUS         0x01
#define COMMAND_TYPE_SET_CONFIG         0x02
#define COMMAND_TYPE_DUMP_DATA_JSON     0x03
#define COMMAND_TYPE_PAUSE_MONITORING   0x04
#define COMMAND_TYPE_START_MONITORING   0x05
#define COMMAND_TYPE_DUMP_STATUS        0x06
#define COMMAND_TYPE_FORCE_SCAN         0x07

typedef struct rackmond_command {
  uint16_t type;
  union {
    raw_modbus_command raw_modbus;
    set_config_command set_config;
  };
} rackmond_command;
