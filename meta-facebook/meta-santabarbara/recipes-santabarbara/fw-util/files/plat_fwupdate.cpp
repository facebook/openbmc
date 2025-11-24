#include <cstdio>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <syslog.h>
#include <gpiod.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include "spiflash.h"

using namespace std;

class SWBFlashComponent : public ExternalSPIComponent {
    private:
      std::string _gpio_line;
      bool _is_high_active;
    
    public:
      SWBFlashComponent(
        const std::string& fru,
        const std::string& comp,
        const std::string& gpio_line,
        bool is_high_active = true,
        const std::string& programmer_type = "ft2232_spi",
        const std::string& chip_params = "type=2232H,port=B")
        : ExternalSPIComponent(fru, comp, programmer_type, chip_params),
          _gpio_line(gpio_line),
          _is_high_active(is_high_active) {}
    
      int update(const std::string& image) override
      {
        int ret;
        gpiod_line* line = gpiod_line_find(_gpio_line.c_str());
        if (!line) {
          std::cerr << "Failed to find GPIO line: " << _gpio_line << std::endl;
          throw std::runtime_error("GPIO line not found");
        }
    
        if (gpiod_line_request_output(line, "fw-util", _is_high_active ? 1 : 0) != 0) {
          std::cerr << "Failed to request GPIO line as output" << std::endl;
          gpiod_line_close_chip(line);
          throw std::runtime_error("Failed to request GPIO line as output");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
        ret = ExternalSPIComponent::update(image);
    
        gpiod_line_set_value(line, _is_high_active ? 0 : 1);
        gpiod_line_release(line);
        gpiod_line_close_chip(line);
    
        return ret;
      }
    
      int dump(const std::string& image) override
      {
        int ret;
        gpiod_line* line = gpiod_line_find(_gpio_line.c_str());
        if (!line) {
          std::cerr << "Failed to find GPIO line: " << _gpio_line << std::endl;
          throw std::runtime_error("GPIO line not found");
        }
    
        if (gpiod_line_request_output(line, "fw-util", _is_high_active ? 1 : 0) != 0) {
          std::cerr << "Failed to request GPIO line as output" << std::endl;
          gpiod_line_close_chip(line);
          throw std::runtime_error("Failed to request GPIO line as output");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
        ret = ExternalSPIComponent::dump(image);
    
        gpiod_line_set_value(line, _is_high_active ? 0 : 1);
        gpiod_line_release(line);
        gpiod_line_close_chip(line);
    
        return ret;
      }
    };
    
SWBFlashComponent swb_flash("swb", "flash", "SPI_MUX_SEL", true);
