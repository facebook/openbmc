/*
Copyright 2022-present Facebook. All Rights Reserved.
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

#include <xyz/openbmc_project/FruDevice/aserver.hpp>
#include <xyz/openbmc_project/FruDevice/error.hpp>
#include <fstream>

class FruDevice
    : public sdbusplus::aserver::xyz::openbmc_project::FruDevice<FruDevice> {
 public:
  FruDevice(sdbusplus::async::context& ctx, const char* path)
      : sdbusplus::aserver::xyz::openbmc_project::FruDevice<FruDevice>(
            ctx,
            path) {
    readSerialFromFile();
    /* Entity-Manager's probe will look for this property */
    set_property(product_product_name_t(), std::string("Darwin"));
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

 private:
  /* This is where the Serfmon-Cache daemon writes the serial number fetched
   * from the console */
  const std::string filePath = "/mnt/data/userver_serial_number.txt";
  void readSerialFromFile() {
    std::ifstream file(filePath);
    if (!file.is_open()) {
      throw sdbusplus::xyz::openbmc_project::FruDevice::Error::FileNotFound();
    }

    std::string serial;
    std::getline(file, serial);
    file.close();

    serial_number_ = serial;
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
