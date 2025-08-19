/*
Copyright 2025-present Facebook. All Rights Reserved.
This program file is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; version 2 of the License.
This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.
You should have received a copy of the GNU General Public License
along with this program in a file named COPYING; if not, write to the
Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301 USA
*/

#include <fmt/base.h>
#include <xyz/openbmc_project/FruDevice/aserver.hpp>
#include <xyz/openbmc_project/FruDevice/error.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <regex>
#include <thread>

/* We use logging for two different kind of misbehaviours
 *
 * ERROR: Something went wrong inside find_serfmon.sh, we cannot advertise the
 * parsed serfmon in DBUS since it is most likely corrupted, so just abort
 *
 * WARNING: File in which the serfmon string should be, is not existent yet,
 * this can vary from the x86 not placing it yet on the serial console so the
 * server tried to to access the file before find_serfmon.sh created it, this is
 * not fatal as it can just be a miss-synchronization, most times just waiting a
 * few seconds will result in the file being created
 */

#define LOG_ERROR(fmt_str, ...) \
  fmt::print(stderr, "[ERROR] " fmt_str "\n", ##__VA_ARGS__)
#define LOG_WARNING(fmt_str, ...) \
  fmt::print(stderr, "[WARNING] " fmt_str "\n", ##__VA_ARGS__)

/* Time between serfmon file read retries */
#define SECONDS_BETWEEN_FILE_READ 5

/* Regex capture group indexes */
#define LENGTH_GROUP 0
#define SERIAL_GROUP 1
#define CRC_GROUP 2

/* Expected line format : !serfmon:11:FGN22111126:75\r
 *                        !serfmon:<length>:<serial>:<crc> */
static const std::regex serfmonPattern(
    R"(^!serfmon:([0-9]+):([A-Z0-9]+):([0-9]+)\r?$)",
    std::regex::ECMAScript);

class FruDevice
    : public sdbusplus::aserver::xyz::openbmc_project::FruDevice<FruDevice> {
 public:
  FruDevice(sdbusplus::async::context& ctx, const char* path)
      : sdbusplus::aserver::xyz::openbmc_project::FruDevice<FruDevice>(
            ctx,
            path) {
    readSerialFromFile();
    /* Entity-Manager's probe will look for these properties */
    set_property(product_product_name_t(), std::string("Darwin"));
    set_property(product_manufacturer_t(), std::string("Meta"));
  }
  auto get_property(serial_number_t) const {
    return serial_number_;
  }
  bool set_property(serial_number_t, auto serial_number) {
    std::swap(serial_number_, serial_number);
    return serial_number_ == serial_number;
  }

  auto get_property(product_product_name_t) const {
    return product_product_name_;
  }
  bool set_property(product_product_name_t, auto product_product_name) {
    std::swap(product_product_name_, product_product_name);
    return product_product_name_ == product_product_name;
  }

  auto get_property(product_manufacturer_t) const {
    return product_manufacturer_;
  }
  bool set_property(product_manufacturer_t, auto product_manufacturer) {
    std::swap(product_manufacturer_, product_manufacturer);
    return product_manufacturer_ == product_manufacturer;
  }

 private:
  /* This is where the Serfmon-Cache daemon writes the serial number fetched
   * from the console */
  const std::string filePath = "/mnt/data/userver_serial_number.txt";
  void readSerialFromFile() {
    while (true) {
      try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
          throw sdbusplus::xyz::openbmc_project::FruDevice::Error::
              FileNotFound();
        }

        std::string line;
        std::getline(file, line);
        file.close();

        std::smatch match;
        if (!std::regex_match(line, match, serfmonPattern)) {
          LOG_ERROR(
              "Malformed serfmon line: \"{}\". Expected format: !serfmon:<length>:<serial>:<crc>",
              line);
          throw sdbusplus::xyz::openbmc_project::FruDevice::Error::
              MalformedLine();
        }

        std::string serial = match[SERIAL_GROUP];
        serial_number_ = serial;

        // Success, exit the loop and advertise on DBUS
        break;
      } catch (
          const sdbusplus::xyz::openbmc_project::FruDevice::Error::FileNotFound&
              e) {
        LOG_WARNING(
            "Serfmon DBUS server didn't find the serfmon file, retrying in {} seconds...",
            SECONDS_BETWEEN_FILE_READ);
        std::this_thread::sleep_for(
            std::chrono::seconds(SECONDS_BETWEEN_FILE_READ));
        continue;
      } catch (const sdbusplus::xyz::openbmc_project::FruDevice::Error::
                   MalformedLine& e) {
        LOG_ERROR(
            "DBUS server parsed a serfmon string which does not respect the expected format, most likely corruption ",
            "happened inside find_serfmon.sh, aborting...");
        throw;
      }
    }
  }
};
/*
 *
 */

int main(int, char**) {
  sdbusplus::async::context ctx{};
  sdbusplus::server::manager_t manager{ctx, "/"};

  FruDevice p{ctx, "/meta/darwin/serfmon_cache_dbus_provider"};

  ctx.spawn([](sdbusplus::async::context& ctx) -> sdbusplus::async::task<> {
    ctx.get_bus().request_name("meta.darwin.serfmon_cache_dbus_provider");
    co_return;
  }(ctx));

  ctx.run();

  return 0;
}
