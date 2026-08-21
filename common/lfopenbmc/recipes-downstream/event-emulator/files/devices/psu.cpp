#include "utils/device_registry.hpp"

namespace event_emulator
{

static DeviceRegistration psuRegistration("psu", [] {
    return DeviceEventData{
        .sensorName = "PSU_1_1_TEMP_ALARM_OUTLET",
        .powerRailName = "PSU_1_1_GENERAL_ALARM_PFC_CONVERTER_FAILURE",
        .fanName = "PSU_1_1_TEMP_ALARM_FAN_FAILURE",
        .smcName = "PSU_1_1_COMM_ALARM_PRIMARY_SECONDARY_MCU_FAULT",
        .sensorFailureName = "PSU_1_1_TEMP_ALARM_SENSOR_FAILURE",
        .failureType = "COMM_ALARM_PRIMARY_SECONDARY_MCU_FAULT",
        .powerFailureData = "GENERAL_ALARM_PFC_CONVERTER_FAILURE",
        .supportedEvents = {"reading-critical", "power-fault", "fan-failure",
                            "controller-failure", "sensor-failure"},
    };
});

} // namespace event_emulator
