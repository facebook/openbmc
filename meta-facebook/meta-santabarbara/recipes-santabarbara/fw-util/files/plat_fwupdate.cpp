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
#include <vector>
#include "spiflash.h"

using namespace std;

struct GpioConfig
{
    std::string name;
    bool isActiveHigh;
};

class SWBFlashComponent : public ExternalSPIComponent {
    private:
      std::vector<GpioConfig> gpioConfigs;
    
    public:
      SWBFlashComponent(
        const std::string& fru,
        const std::string& comp,
        const std::vector<GpioConfig>& configs,
        const std::string& programmerType = "ft2232_spi",
        const std::string& chipParams = "type=2232H,port=B")
        : ExternalSPIComponent(fru, comp, programmerType, chipParams),
          gpioConfigs(configs) {}
    
    private:
      std::vector<gpiod_line*> requestGpioLines()
      {
        std::vector<gpiod_line*> lines;

        for (const auto& config : gpioConfigs)
        {
            gpiod_line* line = gpiod_line_find(config.name.c_str());
            if (!line)
            {
                for (auto* l : lines)
                {
                    gpiod_line_release(l);
                    gpiod_line_close_chip(l);
                }
                std::cerr << "Failed to find GPIO line: " << config.name
                          << std::endl;
                throw std::runtime_error("GPIO line not found");
            }

            int value = config.isActiveHigh ? 1 : 0;

            if (gpiod_line_request_output(line, "fw-util", value) != 0)
            {
                gpiod_line_close_chip(line);
                for (auto* l : lines)
                {
                    gpiod_line_release(l);
                    gpiod_line_close_chip(l);
                }
                std::cerr << "Failed to request GPIO line: " << config.name
                          << std::endl;
                throw std::runtime_error(
                    "Failed to request GPIO line as output");
            }

            lines.push_back(line);
        }

        return lines;
      }

      void releaseGpioLines(std::vector<gpiod_line*>& lines)
      {
        for (size_t i = 0; i < lines.size(); i++)
        {
            int value = gpioConfigs[i].isActiveHigh ? 0 : 1;
            gpiod_line_set_value(lines[i], value);
            gpiod_line_release(lines[i]);
            gpiod_line_close_chip(lines[i]);
        }
        lines.clear();
      }
    
    public:
      int update(const std::string& image) override
      {
        auto lines = requestGpioLines();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
        int ret = ExternalSPIComponent::update(image);
    
        releaseGpioLines(lines);
        return ret;
      }
    
      int dump(const std::string& image) override
      {
        auto lines = requestGpioLines();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
        int ret = ExternalSPIComponent::dump(image);
    
        releaseGpioLines(lines);
        return ret;
      }
    };
    
SWBFlashComponent swbFlashBrcm("swb_brcm", "flash",
                               {{"SPI_MUX_SEL", true}});

SWBFlashComponent swbFlashAeb1("swb_aeb1", "flash",
                               {{"SCO1_SPI_SEL", true},
                                {"SPI_PROG_PL12_EN_N", false},
                                {"SPI_PROG_PL34_EN_N", true}});

SWBFlashComponent swbFlashAeb2("swb_aeb2", "flash",
                               {{"SPI_PROG_PL12_SEL", true},
                                {"SCO2_SPI_SEL", true},
                                {"SPI_PROG_PL12_EN_N", false},
                                {"SPI_PROG_PL34_EN_N", true}});

SWBFlashComponent swbFlashAeb3("swb_aeb3", "flash",
                               {{"SCO3_SPI_SEL", true},
                                {"SPI_PROG_PL12_EN_N", true},
                                {"SPI_PROG_PL34_EN_N", false}});

SWBFlashComponent swbFlashAeb4("swb_aeb4", "flash",
                               {{"SPI_PROG_PL34_SEL", true},
                                {"SCO4_SPI_SEL", true},
                                {"SPI_PROG_PL12_EN_N", true},
                                {"SPI_PROG_PL34_EN_N", false}});