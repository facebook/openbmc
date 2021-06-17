/*
 * syseventgen-util
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <fstream>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <iostream>
#include "CLI/CLI.hpp"
#include <openbmc/pal.h>
#include <openbmc/sdr.h>
#include <nlohmann/json.hpp>
#include "syseventgen-util.h"

#define BOOKMARK_BEGIN(FRU, TYPE) syslog(LOG_CRIT, "SEL Injection in FRU: %d beginning of type: %s", FRU, TYPE)
#define BOOKMARK_END(FRU) syslog(LOG_CRIT, "SEL Injection in FRU: %d ending", FRU)

float get_sensor_value(uint8_t fru, int snr_num) {
    float snr_value;
    int ret;

    if (!pal_sensor_is_cached(fru, snr_num)) {
      usleep(50);
      ret = sensor_raw_read(fru, snr_num, &snr_value);
    } else {
      ret = sensor_cache_read(fru, snr_num, &snr_value);
    }

    if (ret < 0) {
        std::cout << "Error reading sensor value with code: " << ret << std::endl;
    }
    return snr_value;
}


void sensor_callback(uint8_t fru, std::string snr_str, const char* payload, unsigned int delay) {
    // Interpret the sensor string
    char* snr_end;
    int snr_num = strtol(snr_str.c_str(), &snr_end, 0);
    if (!snr_end || *snr_end != '\0') {
        std::cout << "Invalid sensor configuration" << std::endl;
        return;
    }

    // going to set an arbitrarily smaller number
    int ret;
    float curr_value = get_sensor_value(fru, snr_num);
    float curr_thresh;
    char util_call[256];

    // store the current threshold to properly restore it
    thresh_sensor_t snr = {};
    ret = sdr_get_snr_thresh(fru, snr_num, &snr);
    if (ret) {
        std::cout << "Failed to get a threshold value" << std::endl;
        return;
    }

    // check to make sure it has a threshold
    if (!(snr.flag & GETMASK(UCR_THRESH))) {
        std::cout << "No threshold in specified sensor" << std::endl;
        return;
    }
    curr_thresh = snr.ucr_thresh;

    float buffer = 5;
    float target_value;
    if (curr_value > buffer || curr_value < 0) {
        target_value = curr_value - buffer;
    } else {
        target_value = 0;
    }

    // start the bookend log
    BOOKMARK_BEGIN(fru, "sensor");

    sprintf(util_call, payload, target_value);
    std::cout << "Calling: " << util_call << std::endl;
    ret = system(util_call);
    if (ret) {
        std::cout << "Error in setting threshold: " << ret << std::endl;
        return;
    }

    // wait the delay to have the log take effect
    usleep(delay * 1000); // convert us to ms

    // set the threshold back
    char fru_name[16];
    ret = pal_get_fru_name(fru, fru_name);
    if (ret) {
        std::cout << "Error getting the fru name" << std::endl;
        return;
    }
    sprintf(util_call, "threshold-util %s --set %s UCR %f", fru_name, snr_str.c_str(), curr_thresh);
    std::cout << "Calling: " << util_call << std::endl;
    ret = system(util_call);
    if (ret) {
        std::cout << "Error in returning threshold to original value" << std::endl;
        return;
    }

    // wait the delay again and send the ending log
    usleep(delay * 1000);
    BOOKMARK_END(fru);
}

void ipmi_callback() {
    std::cout << "called an ipmi function" << std::endl;
}

int main(int argc, char **argv)
{
    /*
     * JSON setup
     */

    std::ifstream fileStream("/etc/syseventgen.conf");
    nlohmann::json jsonFile = nlohmann::json::parse(fileStream);

    // get the delay from the JSON file
    unsigned int delay = jsonFile["delay"].get<unsigned int>();

    /*
     * CLI app setup
     */
    CLI::App app{"System Event Generation Util"};
    app.set_help_flag();
    app.set_help_all_flag("-h,--help");

    // get all of the options from the json and add them as subcommands
    for (auto& elem : jsonFile.items()) {
        // if this is the delay, then its not an option
        auto& key = elem.key();
        auto& val = elem.value();
        if (key.find("delay") == std::string::npos) {
            auto command = app.add_subcommand(key, val["tag"].get<std::string>());
            command->callback([&]() {
                switch (val["sel_type"].get<int>()) {
                    case 0: // sensor
                        sensor_callback(
                            val["sel_subtypes"][0].get<uint8_t>(),
                            val["sel_subtypes"][1].get<std::string>(),
                            val["payload"].get<std::string>().append(" %f").c_str(),
                            delay);
                        break;
                    case 1: // ipmi
                        ipmi_callback();
                        break;
                    case 2: // gpio
                        break;
                    case 3: // ncsi
                        break;
                }
            });
        }
    }
    app.require_subcommand(0, 1);
    CLI11_PARSE(app, argc, argv);
}
