#pragma once

#include <sdbusplus/bus.hpp>

#include <filesystem>

class CCIupdater
{
  public:
    CCIupdater(const std::filesystem::path imagePath, uint8_t eid) :
        imagePath(imagePath), eid(eid)
    {}

    CCIupdater() = delete;
    /** @brief Update CXL firmware using CCI over MCTP.
     *
     *  @return Success or Fail
     */
    bool updateFw();

    /** @brief Get CXL firmware version using CCI over MCTP.
     *
     *  @return Success or Fail
     */
    bool getFwVersion();

  private:
    /** The image path. */
    const std::filesystem::path imagePath;

    /** The MCTP EID for update*/
    uint8_t eid;
};
