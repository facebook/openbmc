#pragma once

#ifdef __cplusplus
#include <stdexcept>
#include <string>
#include <vector>

namespace hgx {

// Standard HTTP Exception
struct HTTPException : std::runtime_error {
  int errorCode = 0;
  HTTPException(int code)
      : std::runtime_error("HTTP Error: " + std::to_string(code)),
        errorCode(code) {}
};

struct TaskStatus {
  std::string resp{};
  // one of Starting, Stopping, Completed, Exception, and Running
  std::string state{};
  std::string status{};
  std::vector<std::string> messages{};
};

// Get a subpath after /redfish/v1
// Example, get on 192.168.31.1/redfish/v1/blah, the subpath = blah
std::string redfishGet(const std::string& subpath);
// Post a subpath after /redfish/v1
// Example, get on 192.168.31.1/redfish/v1/blah, the subpath = blah
std::string redfishPost(const std::string& subpath, std::string&& args);

// Return the version of the component
std::string version(const std::string& comp, bool returnJson = false);

// Initiate an update and return the task ID.
// User can call taskStatus to get the current
// status.
std::string updateNonBlocking(const std::string& path, bool returnJson = false);

// Initiate an update and wait till the task
// completes.
void update(const std::string& path);

// Get task status (JSON output)
TaskStatus getTaskStatus(const std::string& id);

// Get the current value of a sensor.
float sensor(const std::string& component, const std::string& name);

// Request for the value of the sensor, but return all the JSON output
std::string sensorRaw(const std::string& component, const std::string& name);
} // namespace hgx

// These are APIs to be used by C based libraries.
extern "C" {
#endif

int get_hgx_sensor(const char* component, const char* snr_name, float* value);
int get_hgx_ver(const char* component, char *version);

#ifdef __cplusplus
}
#endif
