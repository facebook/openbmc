#include "utils/device_registry.hpp"

namespace event_emulator
{

static DeviceRegistration cbuRegistration("cbu", [] {
    return DeviceEventData{
        .sensorPath = "/xyz/openbmc_project/sensor/"
                      "CBU_1_1_STATUS_WORD_TEMPERATURE_FAULT",
        .powerRailPath = "/xyz/openbmc_project/state/power_rail/"
                         "CBU_1_1_STATUS_DISCHARGE_NOT_ALLOWED",
        .fanPath = "/xyz/openbmc_project/state/fan/"
                   "CBU_1_1_STATUS_FAN_FAILURE",
        .smcPath = "/xyz/openbmc_project/state/smc/"
                   "CBU_1_1_HW_SIGNAL_ISHARE_CABLE_NOT_DETECTED",
        .sensorFailurePath = "/xyz/openbmc_project/sensor/"
                             "CBU_1_1_STATUS_TEMP_BUSBAR_CLIP_OVER_TEMP",
        .failureType = "HW_SIGNAL_ISHARE_CABLE_NOT_DETECTED",
        .powerFailureData = "STATUS_DISCHARGE_NOT_ALLOWED",
        .supportedEvents = {"reading-critical", "power-fault", "fan-failure",
                            "controller-failure"},
    };
});

} // namespace event_emulator
