#include "platform.hpp"
#include <syslog.h>
#include <openbmc/kv.hpp>

#define CB_ASIC_SENSOR_START_ID 0x1A
#define CB_ASIC_SENSOR_END_ID 0x31
#define CB_ASIC_NVME_STATUS_OFFSET 0x03
#define CB_ASIC_NVME_NOT_READY_STATE 0x01
#define CB_ASIC_NVME_READY_STATE 0x02

namespace pldm
{
namespace responder
{
namespace platform
{

std::string get_sensor_name(uint16_t id)
{
  static const std::map<uint16_t, const std::string> sensor_map = {
    {0x01, "CB_SENSOR_ACCL_1"},
    {0x02, "CB_SENSOR_ACCL_2"},
    {0x03, "CB_SENSOR_ACCL_3"},
    {0x04, "CB_SENSOR_ACCL_4"},
    {0x05, "CB_SENSOR_ACCL_5"},
    {0x06, "CB_SENSOR_ACCL_6"},
    {0x07, "CB_SENSOR_ACCL_7"},
    {0x08, "CB_SENSOR_ACCL_8"},
    {0x09, "CB_SENSOR_ACCL_9"},
    {0x0A, "CB_SENSOR_ACCL_10"},
    {0x0B, "CB_SENSOR_ACCL_11"},
    {0x0C, "CB_SENSOR_ACCL_12"},
    {0x0D, "CB_SENSOR_ACCL_POWER_CABLE_1"},
    {0x0E, "CB_SENSOR_ACCL_POWER_CABLE_2"},
    {0x0F, "CB_SENSOR_ACCL_POWER_CABLE_3"},
    {0x10, "CB_SENSOR_ACCL_POWER_CABLE_4"},
    {0x11, "CB_SENSOR_ACCL_POWER_CABLE_5"},
    {0x12, "CB_SENSOR_ACCL_POWER_CABLE_6"},
    {0x13, "CB_SENSOR_ACCL_POWER_CABLE_7"},
    {0x14, "CB_SENSOR_ACCL_POWER_CABLE_8"},
    {0x15, "CB_SENSOR_ACCL_POWER_CABLE_9"},
    {0x16, "CB_SENSOR_ACCL_POWER_CABLE_10"},
    {0x17, "CB_SENSOR_ACCL_POWER_CABLE_11"},
    {0x18, "CB_SENSOR_ACCL_POWER_CABLE_12"},
    {0x19, "CB_SENSOR_FIO"},
    {0x1A, "CB_ACCL_1_DEV_1"},
    {0x1B, "CB_ACCL_1_DEV_2"},
    {0x1C, "CB_ACCL_2_DEV_1"},
    {0x1D, "CB_ACCL_2_DEV_2"},
    {0x1E, "CB_ACCL_3_DEV_1"},
    {0x1F, "CB_ACCL_3_DEV_2"},
    {0x20, "CB_ACCL_4_DEV_1"},
    {0x21, "CB_ACCL_4_DEV_2"},
    {0x22, "CB_ACCL_5_DEV_1"},
    {0x23, "CB_ACCL_5_DEV_2"},
    {0x24, "CB_ACCL_6_DEV_1"},
    {0x25, "CB_ACCL_6_DEV_2"},
    {0x26, "CB_ACCL_7_DEV_1"},
    {0x27, "CB_ACCL_7_DEV_2"},
    {0x28, "CB_ACCL_8_DEV_1"},
    {0x29, "CB_ACCL_8_DEV_2"},
    {0x2A, "CB_ACCL_9_DEV_1"},
    {0x2B, "CB_ACCL_9_DEV_2"},
    {0x2C, "CB_ACCL_10_DEV_1"},
    {0x2D, "CB_ACCL_10_DEV_2"},
    {0x2E, "CB_ACCL_11_DEV_1"},
    {0x2F, "CB_ACCL_11_DEV_2"},
    {0x30, "CB_ACCL_12_DEV_1"},
    {0x31, "CB_ACCL_12_DEV_2"},
    {0x80, "MC_SENSOR_SSD_1"},
    {0x81, "MC_SENSOR_SSD_2"},
    {0x82, "MC_SENSOR_SSD_3"},
    {0x83, "MC_SENSOR_SSD_4"},
  };

  if (sensor_map.find(id) !=sensor_map.end())
    return sensor_map.at(id);
  else
    return "Unknown";
}

std::string get_state_message(uint8_t offset, uint8_t state)
{
  static const std::vector <std::vector<std::string>> eventstate_map = {
    {"unknown", "present", "not present"},                                    // sensorOffset = 0: presentence
    {"unknown", "normal",  "alert"},                                          // sensorOffset = 1: status
    {"no power good", "power good fail", "power good fail (3V3 power fault)",
     "power good fail (12V power fault)", "power good fail (3V3 aux fault)",
     "power good fault", "no power good (3V3 power fault)",
     "no power good (12V power fault)", "no power good (3V3 aux fault)", 
     "power good fail (12V aux power fault)",
     "no power good (12V aux power fault)"},                                  // sensorOffset = 2: power status
    {"unknown", "nvme not ready", "nvme ready"},                              // sensorOffset = 3: nvme status
  };

  if (offset >= eventstate_map.size())
    return "offset not supported";
  else if (state >= eventstate_map[offset].size())
    return "state not supported";
  else
    return eventstate_map[offset][state];
}

std::string get_device_type(uint8_t type)
{
  static const std::map<uint8_t, const std::string> device_map = {
    {0x01, "CB_P0V8_VDD_1"},
    {0x02, "CB_P0V8_VDD_2"},
    {0x03, "CB_POWER_BRICK_0"},
    {0x04, "CB_P1V25_MONITOR"},
    {0x05, "CB_P12V_ACCL1_MONITOR"},
    {0x06, "CB_P12V_ACCL2_MONITOR"},
    {0x07, "CB_P12V_ACCL3_MONITOR"},
    {0x08, "CB_P12V_ACCL4_MONITOR"},
    {0x09, "CB_P12V_ACCL5_MONITOR"},
    {0x0A, "CB_P12V_ACCL6_MONITOR"},
    {0x0B, "CB_P12V_ACCL7_MONITOR"},
    {0x0C, "CB_P12V_ACCL8_MONITOR"},
    {0x0D, "CB_P12V_ACCL9_MONITOR"},
    {0x0E, "CB_P12V_ACCL10_MONITOR"},
    {0x0F, "CB_P12V_ACCL11_MONITOR"},
    {0x10, "CB_P12V_ACCL12_MONITOR"},
    {0x11, "CB_PESW_0"},
    {0x12, "CB_PESW_1"},
    {0x13, "CB_POWER_BRICK_1"},
  };

  if (device_map.find(type) != device_map.end())
    return device_map.at(type);
  else
    return "UNKNOWN_TYPE";
}

std::string get_board_info(uint8_t id)
{
  static const std::map<uint8_t, const std::string> board_map = {
    {0x00, "MAIN_SOURCE"},
    {0x01, "SECOND_SOURCE"},
  };

  if (board_map.find(id) != board_map.end())
    return board_map.at(id);
  else
    return "UNKNOWN_SOURCE";
}

std::string get_event_type(uint8_t type)
{
  static const std::map<uint8_t, const std::string> event_map = {
    {0x00, "UNUSE"},
    {0x01, "OVER_POWER"},
    {0x02, "OVER_VOLTAGE"},
    {0x03, "OVER_CURRENT"},
    {0x04, "UNDER_VOLTAGE"},
    {0x05, "OVER_TEMPERATURE"},
    // Switch CCR system error
    {0x06, "SYSTEM_ERROR"},
    {0x07, "PEX_FATAL_ERROR"},
    {0x08, "POR_BISR_TIMEOUT"},
    {0x09, "FLASH_SIGNATURE_FAIL"},
    {0x0A, "WATCHDOG_0_TIMEOUT_CPU_CORE_RESET"},
    {0x0B, "WATCHDOG_0_TIMEOUT_SYSTEM_RESET"},
    {0x0C, "WATCHDOG_1_TIMEOUT_CPU_CORE_RESET"},
    {0x0D, "WATCHDOG_1_TIMEOUT_SYSTEM_RESET"},
    {0x0E, "LOCAL_CPU_PARITY_ERROR"},
    {0x0F, "SECURE_BOOT_FAIL"},
    {0x10, "SBR_LOAD_FAIL"},
    {0x11, "STATION_0_FATAL_ERROR"},
    {0x12, "STATION_1_FATAL_ERROR"},
    {0x13, "STATION_2_FATAL_ERROR"},
    {0x14, "STATION_3_FATAL_ERROR"},
    {0x15, "STATION_4_FATAL_ERROR"},
    {0x16, "STATION_5_FATAL_ERROR"},
    {0x17, "STATION_6_FATAL_ERROR"},
    {0x18, "STATION_7_FATAL_ERROR"},
    {0x19, "STATION_8_FATAL_ERROR"},
    {0x1A, "PSB_STATION_FATAL_ERROR"},
    // Power brick status fault
    {0x1B, "OUTPUT_VOLTAGE_WARNING_FAULT"},
    {0x1C, "OUTPUT_CURRENT_WARNING_FAULT"},
    {0x1D, "INPUT_VOLTAGE_FAULT"},
    {0x1E, "POWER_GOOD_FAULT"},
    {0x1F, "POWER_OFF_FAULT"},
    {0x20, "TEMPERATURE_WARNING_FAULT"},
    {0x21, "CML_FAULT"},
    {0x22, "MFR_SPECIFIC_FAULT"},
    {0x23, "NO_LISTED_FAULT"},
  };

  if (event_map.find(type) != event_map.end())
    return event_map.at(type);
  else
    return "UNKNOWN_TYPE";
}

void set_sensor_state_work(uint16_t id, uint8_t offset, uint8_t state)
{
  if (id >= CB_ASIC_SENSOR_START_ID && id <= CB_ASIC_SENSOR_END_ID) {
    if (offset == CB_ASIC_NVME_STATUS_OFFSET) {
      uint8_t index = id - CB_ASIC_SENSOR_START_ID;
      char key[MAX_KEY_LEN] = {0};
      sprintf(key, "is_asic%d_ready", index);

      switch (state) {
        case CB_ASIC_NVME_NOT_READY_STATE:
	  if (kv_set(key, "0", 0, 0) != 0) {
	    syslog(LOG_WARNING, "Set ASIC nvme status fail, index: 0x%x, state: not ready", index);
	  }
          break;
        case CB_ASIC_NVME_READY_STATE:
	  if (kv_set(key, "1", 0, 0) != 0) {
            syslog(LOG_WARNING, "Set ASIC nvme status fail, index: 0x%x, state: ready", index);
          }
          break;
        default:
          break;
      }
    }
  }
}

bool is_record_event(uint16_t id, uint8_t offset, uint8_t state)
{
  if (id >= CB_ASIC_SENSOR_START_ID && id <= CB_ASIC_SENSOR_END_ID) {
    if (offset == CB_ASIC_NVME_STATUS_OFFSET) {
      return false;
    }
  }

  return true;
}

} // namespace platform
} // namespace responder
} // namespace pldm
