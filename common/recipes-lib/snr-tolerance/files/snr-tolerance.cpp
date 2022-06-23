#include <syslog.h>
#include <iostream>
#include <string>
#include <ctime>
#include <chrono>
#include <map>
#include "snr-tolerance.h"

using namespace std;
using namespace std::chrono;

class sensor_failure_tolerance_info {
    public:
        seconds failure_timestamp;
        seconds tolerance_time;

        sensor_failure_tolerance_info (seconds fail_ts = seconds{0}, seconds tol_time = seconds{0}): failure_timestamp(fail_ts), tolerance_time(tol_time) {}
};

map<uint32_t, sensor_failure_tolerance_info> sensor_timestamp;

// Use to register sensor failure timestamp map for sensor failure tolerance policy
int register_sensor_failure_tolerance_policy(uint16_t fru, uint16_t snr_num, int tol_time) {
    uint32_t sensor_index = 0;

    if (tol_time < 0) {
        return -1;
    }

    //Set sensor index for sensor timestamp map
    sensor_index = (fru << 16) + snr_num;
    sensor_timestamp[sensor_index].tolerance_time = seconds {tol_time};

    return 0;
}

// Use to check if the failure sensor value should be tolerated
int update_sensor_poll_timestamp(uint16_t fru, uint16_t snr_num, bool available, float value) {
    uint32_t sensor_index = 0;
    int ret = SET_SENSOR_TO_CACHE;
    seconds snr_fail_timestamp = seconds {0}, snr_fail_tol_time = seconds {0};

    //Set sensor index for sensor timestamp map
    sensor_index = (fru << 16) + snr_num;

    // Don't check sensor failure timestamp if sensor timestamp map is not registered.
    auto sensor_ts = sensor_timestamp.find(sensor_index);
    if (sensor_ts == sensor_timestamp.end()) {
        return ret;
    }

    // Don't check sensor failure timestamp if sensor failure tolerance time is not set.
    snr_fail_tol_time = sensor_ts->second.tolerance_time;
    if (snr_fail_tol_time == seconds {0}) {
        return ret;
    }

    // Sensor reading is abnormal
    if ((available == false) || (value == 0)) {
        // Get current timestamp
        auto curr_timestamp = duration_cast <seconds> ((steady_clock::now()).time_since_epoch());

        // Get sensor failure timestamp
        snr_fail_timestamp = sensor_ts->second.failure_timestamp;
        if (snr_fail_timestamp == seconds {0}) {
            // Get failure sensor reading for the first time
            sensor_ts->second.failure_timestamp = curr_timestamp;
            ret = SKIP_SENSOR_FAILURE;
        } else if ((curr_timestamp - snr_fail_timestamp) < snr_fail_tol_time) {
            // Get failure sensor reading but still in the tolerance time
            ret = SKIP_SENSOR_FAILURE;
        } else {
            if ((available == true) && (value == 0)) {
                // Sensor value is still 0 after tolerance time, set sensor value to cached value
                sensor_ts->second.failure_timestamp = seconds {0};
            }
            ret = SET_SENSOR_TO_CACHE;
        }
    } else {
        // Sensor reading is normal
        sensor_ts->second.failure_timestamp = seconds {0};
        ret = SET_SENSOR_TO_CACHE;
    }

    return ret;
}
