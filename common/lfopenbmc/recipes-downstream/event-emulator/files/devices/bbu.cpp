#include "utils/device_registry.hpp"

namespace event_emulator
{

static DeviceRegistration bbuRegistration("bbu", [] {
    return DeviceEventData{
        .sensorPath = "/xyz/openbmc_project/sensor/"
                      "BBU_1_1_BATTERY_STATUS_OVER_TEMP_ALARM",
        .powerRailPath = "/xyz/openbmc_project/state/power_rail/"
                         "BBU_1_1_PFC_ALARM_AC_NOT_OK",
        .fanPath = "/xyz/openbmc_project/state/fan/"
                   "BBU_1_1_STATUS_FAN_FAILURE",
        .smcPath = "/xyz/openbmc_project/state/smc/"
                   "BBU_1_1_STATUS_CELL_BALANCING_FAILURE",
        .sensorFailurePath = "/xyz/openbmc_project/sensor/"
                             "BBU_1_1_TEMP_ALARM_SENSOR_FAILURE",
        .failureType = "STATUS_CELL_BALANCING_FAILURE",
        .powerFailureData = "PFC_ALARM_AC_NOT_OK",
        .supportedEvents = {"reading-critical", "power-fault", "fan-failure",
                            "controller-failure", "sensor-failure"},
    };
});

} // namespace event_emulator
