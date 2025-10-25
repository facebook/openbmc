#pragma once

#include <string>
#include <unordered_map>

namespace redfish_client_daemon
{

class PersistMap
{
  public:
    // persist in memory
    PersistMap() = default;

    // persist to file if path is not empty, otherwise persist in memory
    explicit PersistMap(const std::string& path) : path(path)
    {
        load();
    };

    // no-op and return false if key exists and value matches, otherwise update
    // the map with given key and value and return true
    bool update(const std::string& key, const std::string& value);

  private:
    std::string path;
    std::unordered_map<std::string, std::string> map;

    void load();

    void flush();
};

} // namespace redfish_client_daemon
