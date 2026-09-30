#include "utils/device_registry.hpp"

namespace event_emulator
{

static DeviceRegistration cbuRegistration("cbu", [] {
    return DeviceEventData{
        .sensorName = "CBU_1_1_STATUS_WORD_TEMPERATURE_FAULT",
        .powerRailName = "CBU_1_1_STATUS_DISCHARGE_NOT_ALLOWED",
        .fanName = "CBU_1_1_STATUS_FAN_FAILURE",
        .smcName = "CBU_1_1_HW_SIGNAL_ISHARE_CABLE_NOT_DETECTED",
        .sensorFailureName = "CBU_1_1_STATUS_TEMP_BUSBAR_CLIP_OVER_TEMP",
        .failureType = "HW_SIGNAL_ISHARE_CABLE_NOT_DETECTED",
        .powerFailureData = "STATUS_DISCHARGE_NOT_ALLOWED",
        .supportedEvents = {"reading-critical", "power-fault", "fan-failure",
                            "controller-failure"},
    };
});

} // namespace event_emulator
