#include "utils/device_registry.hpp"

namespace event_emulator
{

static DeviceRegistration psuRegistration("psu", [] {
    return DeviceEventData{
        .sensorPath = "/xyz/openbmc_project/sensor/"
                      "PSU_1_1_TEMP_ALARM_OUTLET",
        .powerRailPath = "/xyz/openbmc_project/state/power_rail/"
                         "PSU_1_1_GENERAL_ALARM_PFC_CONVERTER_FAILURE",
        .fanPath = "/xyz/openbmc_project/state/fan/"
                   "PSU_1_1_TEMP_ALARM_FAN_FAILURE",
        .smcPath = "/xyz/openbmc_project/state/smc/"
                   "PSU_1_1_COMM_ALARM_PRIMARY_SECONDARY_MCU_FAULT",
        .sensorFailurePath = "/xyz/openbmc_project/sensor/"
                             "PSU_1_1_TEMP_ALARM_SENSOR_FAILURE",
        .failureType = "COMM_ALARM_PRIMARY_SECONDARY_MCU_FAULT",
        .powerFailureData = "GENERAL_ALARM_PFC_CONVERTER_FAILURE",
        .supportedEvents = {"reading-critical", "power-fault", "fan-failure",
                            "controller-failure", "sensor-failure"},
    };
});

} // namespace event_emulator
