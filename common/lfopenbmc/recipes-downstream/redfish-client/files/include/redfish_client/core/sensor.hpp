#pragma once

#include <optional>
#include <string>

namespace redfish_client::core
{

class Sensor
{
  public:
    Sensor() = default;
    virtual ~Sensor() = default;
    static std::optional<Sensor> parseSensor(const std::string& sensorJson);

    double getReading() const;

    const std::string& getSensorUnitText() const;

    std::optional<double> getMinValue() const;

    std::optional<double> getMaxValue() const;

    // Expose a "constructor" for testing purposes.
    static Sensor createTestSensor(double reading, const char* sensorUnit,
                                   std::optional<double> minValue,
                                   std::optional<double> maxValue);

  private:
    double reading = 0;
    std::string sensorUnit;
    std::optional<double> minValue;
    std::optional<double> maxValue;
};

} // namespace redfish_client::core
