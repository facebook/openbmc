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

#include <ctime>
#include <stack>
#include <regex>
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
#include <openbmc/ipmi.h>
#include <nlohmann/json.hpp>
#include "syseventgen-util.h"

#define BOOKMARK_BEGIN(FRU, TYPE) syslog(LOG_CRIT, "SEL Injection in FRU: %d beginning of type: %s", FRU, TYPE)
#define BOOKMARK_END(FRU) syslog(LOG_CRIT, "SEL Injection in FRU: %d ending", FRU)

using time_pair = std::pair<std::string, std::string>;

bool verbose;

std::string hex_int_to_string(int hex) {
    char hex_c[10];
    sprintf(hex_c, "0X%02X", hex);
    return std::string(hex_c);
}

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


void sensor_callback(unsigned int delay,
                     uint8_t fru,
                     const std::string& snr_str,
                     const std::string& bound = "UCR") {
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
    float curr_thresh = 0;

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

    float buffer = 5;
    float target_value = 0;
    if (bound == "UCR") {
        curr_thresh = snr.ucr_thresh;
        if (curr_value > buffer || curr_value < 0) {
            target_value = curr_value - buffer;
        } else {
            target_value = 0;
        }
    } else if (bound == "LCR") {
        curr_thresh = snr.lcr_thresh;
        target_value = std::min(curr_thresh + 5, snr.ucr_thresh);
    } else {
        std::cout << "Invalid or unsupported bound type" << std::endl;
        return;
    }

    // get the fru name
    char fru_name[16];
    ret = pal_get_fru_name(fru, fru_name);
    if (ret) {
        std::cout << "Error getting the fru name" << std::endl;
        return;
    }

    // start the bookend log
    BOOKMARK_BEGIN(fru, "sensor");

    std::string set_cmd = "threshold-util " +
                          std::string(fru_name) +
                          " --set " +
                          snr_str +
                          " " +
                          bound +
                          " " +
                          std::to_string(target_value);

    std::string clr_cmd = "threshold-util " +
                          std::string(fru_name) +
                          " --set " +
                          snr_str +
                          " " +
                          bound +
                          " " +
                          std::to_string(curr_thresh);

    if (verbose) {
        std::cout << "Calling: " << set_cmd << std::endl;
    }
    ret = system(set_cmd.c_str());
    if (ret) {
        std::cout << "Error in setting threshold: " << ret << std::endl;
        return;
    }

    // wait the delay to have the log take effect
    usleep(delay * 1000); // convert us to ms

    // set the threshold back
    if (verbose) {
        std::cout << "Calling: " << clr_cmd << std::endl;
    }
    ret = system(clr_cmd.c_str());
    if (ret) {
        std::cout << "Error in returning threshold to original value" << std::endl;
        return;
    }

    // wait the delay again and send the ending log
    usleep(delay * 1000);
    BOOKMARK_END(fru);
}

void ipmi_callback(
        unsigned int delay,
        const std::string& payload,
        int call_type,
        uint8_t fru,
        const std::string& sel_data) {
    BOOKMARK_BEGIN(fru, "ipmi");

    // Decern between calling payload and constructing call
    if (payload.empty()) {
        std::string ipmi_call = "ipmi-util ";
        ipmi_call += (hex_int_to_string(fru) + " ");

        switch (call_type) {
            case 0: // oem add ras sel
                ipmi_call += (hex_int_to_string(NETFN_OEM_REQ << 2) + " ");
                ipmi_call += (hex_int_to_string(CMD_OEM_ADD_RAS_SEL) + " ");
                break;
            case 1: // sensor alert imm msg
                ipmi_call += (hex_int_to_string(NETFN_SENSOR_REQ << 2) + " ");
                ipmi_call += (hex_int_to_string(CMD_SENSOR_ALERT_IMMEDIATE_MSG) + " ");
                ipmi_call += "0x00 0x00 0x00 0x00 0x00 ";
                break;
            case 2: // storage add sel
                ipmi_call += (hex_int_to_string(NETFN_STORAGE_REQ << 2) + " ");
                ipmi_call += (hex_int_to_string(CMD_STORAGE_ADD_SEL) + " ");
                break;
            case 3: // sensor add plat event msg
                ipmi_call += (hex_int_to_string(NETFN_SENSOR_REQ << 2) + " ");
                ipmi_call += (hex_int_to_string(CMD_SENSOR_PLAT_EVENT_MSG) + " ");
                break;
        }
        ipmi_call += sel_data;

        if (verbose) {
            std::cout << "Calling ipmi command: " << ipmi_call << std::endl;
        }

        FILE* file_stream = popen(ipmi_call.c_str(), "r");
        if (!file_stream) {
            std::cout << "IPMI call failed" <<std::endl;
        }

        if (verbose) {
            ssize_t read_size;
            size_t line_size = 256;
            char* line_buffer = NULL;
            while ((read_size = getline(&line_buffer, &line_size, file_stream)) != -1) {
                std::cout << "Received payload: " << line_buffer << std::endl;
            }

        }

    } else {
        if (verbose) {
            std::cout << "Calling ipmi command: " << payload << std::endl;
        }

        FILE* file_stream = popen(payload.c_str(), "r");
        if (!file_stream) {
            std::cout << "IPMI call failed" <<std::endl;
        }

        if (verbose) {
            ssize_t read_size;
            size_t line_size = 256;
            char* line_buffer = NULL;
            while ((read_size = getline(&line_buffer, &line_size, file_stream)) != -1) {
                std::cout << "Received payload: " << line_buffer << std::endl;
            }

        }
    }

    usleep(delay * 1000);
    BOOKMARK_END(fru);
}

void bic_gpio_callback(unsigned int delay, uint8_t fru, unsigned int gpio_num) {
    // get the current value
    std::string bic_format = std::to_string(gpio_num) + " \\w+: (\\d)";
    std::regex bic_regex(bic_format);
    std::smatch reg_match;

    char fru_name[16], cmd[128];
    if (pal_get_fru_name(fru, fru_name)) {
        std::cout << "Invalid FRU" << std::endl;
        return;
    }
    sprintf(cmd, "bic-util %s --get_gpio", fru_name);
    FILE* file_stream = popen(cmd, "r");

    if (!file_stream) {
        std::cout << "Could not get current gpio values" << std::endl;
        return;
    }

    ssize_t read_size;
    size_t line_size = 256;
    char* line_buffer = NULL;
    int current_value = -1;
    while ((read_size = getline(&line_buffer, &line_size, file_stream)) != -1) {
        std::string line = line_buffer;
        if (std::regex_search(line, reg_match, bic_regex)) {
            current_value = stoi(reg_match[1].str());
            if (verbose) {
                std::cout << "Found current value " << std::to_string(current_value)
                    << std::endl;
            }
            break;
        }
    }

    if (current_value == -1) {
        std::cout << "Specified GPIO value does not exist" << std::endl;
        return;
    }

    // write a to a file the new value
    BOOKMARK_BEGIN(fru, "bic-gpio");

    int ret = system("mkdir -p /tmp/gpio");
    if (ret) {
        std::cout << "failed to make gpio directory" << std::endl;
        return;
    }

    std::string bic_file = "/tmp/gpio/bic"
        + std::to_string(fru)
        + "_"
        + std::to_string(gpio_num);
    std::ofstream tmp_file;

    tmp_file.open(bic_file);
    if (!tmp_file.is_open()) {
        std::cout << "Failed to open file " << bic_file << std::endl;
    }

    tmp_file << (!current_value ? "1" : "0");
    tmp_file.close();

    if (verbose) {
        std::cout << "Creating temp file " << bic_file << " with value "
            << (!current_value ? "1" : "0") << std::endl;
    }

    // wait the delay
    usleep(delay * 1000);

    // delete the file
    if (remove(bic_file.c_str()) != 0)
        std::cout << "Could not delete tmp file" << std::endl;

    // wait for de-asserts
    usleep(delay * 1000);
    BOOKMARK_END(fru);
}

void gpio_callback(unsigned int delay, unsigned int gpio_num) {
    // get the current value
    std::string gpio_filepath = "/sys/class/gpio/gpio"
        + std::to_string(gpio_num) + "/value";

    std::ifstream gpio_file;

    gpio_file.open(gpio_filepath);
    if (!gpio_file.is_open()) {
        std::cout << "Failed to open origonal gpio file" << std::endl;
        return;
    }

    // write a to a file the new value
    BOOKMARK_BEGIN(0, "gpio");

    std::string tmp_dir = "/tmp/gpio/false/gpio"
        + std::to_string(gpio_num);

    std::string create_dir = "mkdir -p " + tmp_dir;

    int ret = system(create_dir.c_str());
    if (ret) {
        std::cout << "Unable to create tmp dir" << std::endl;
        return;
    }

    std::string tmp_filepath = tmp_dir +
        + "/value";
    std::ofstream tmp_file;

    tmp_file.open(tmp_filepath);
    if (!tmp_file.is_open()) {
        std::cout << "Failed to open file " << tmp_filepath << std::endl;
    }

    tmp_file << (gpio_file.get() == '1'  ? "0" : "1");
    tmp_file.close();
    gpio_file.close();

    // wait the delay
    usleep(delay * 1000);

    // delete the file
    if (remove(tmp_filepath.c_str()) != 0)
        std::cout << "Could not delete tmp file" << std::endl;

    // wait for de-asserts
    usleep(delay * 1000);
    BOOKMARK_END(0);
}

void del_logs(std::vector<time_pair> time_pairs) {
    for (time_pair period : time_pairs) {
        std::string buffer = "log-util all --clear -s '" +
                             period.first +
                             "' -e '" +
                             period.second +
                             "'";
        if (verbose) {
            std::cout << "Calling: " << buffer << std::endl;
        }

        int ret = system(buffer.c_str());
        if (ret)
            std::cout << "Error calling log-util: " << ret << std::endl;
    }
}

void print_logs(std::vector<time_pair> time_pairs) {
    for (time_pair period : time_pairs) {
        std::string buffer = "log-util all --print -s '" +
                             period.first +
                             "' -e '" +
                             period.second +
                             "'";
        if (verbose) {
            std::cout << "Calling: " << buffer << std::endl;
        }

        int ret = system(buffer.c_str());
        if (ret)
            std::cout << "Error calling log-util: " << ret << std::endl;
    }
}

std::vector<time_pair> find_sels() {
    std::vector<time_pair> found_times;
    std::stack<std::string> match_times;
    char cmd[] = "log-util all --print";
    std::regex log_regex(R"((\d+)(\s+)(\w+)(\s+)(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})(\s+)((\w|\-)+)(\s+)(.+))");
    std::smatch reg_match;

    FILE* file_stream = popen(cmd, "r");

    // check for valid file
    if (!file_stream)
        std::cout << "Did not find any log files" << std::endl;

    ssize_t read_size;
    size_t line_size = 256;
    char* line_buffer = NULL;
    while ((read_size = getline(&line_buffer, &line_size, file_stream)) != -1) {
        std::string line = line_buffer;
        line_buffer = NULL;
        if (std::regex_search(line, reg_match, log_regex)) {
            if (reg_match[7].str().compare("syseventgen-util") == 0) {
                if (reg_match[10].str().find("beginning") != std::string::npos) {
                    match_times.push(reg_match[5].str());
                } else if (reg_match[10].str().find("ending") != std::string::npos) {
                    if (match_times.empty()) {
                        found_times.push_back(std::make_pair(reg_match[5].str(), reg_match[5].str()));
                    } else {
                        found_times.push_back(std::make_pair(match_times.top(), reg_match[5].str()));
                        match_times.pop();
                    }
                }
            }
        }
    }

    // there is a case where we don't have an ending log
    while (!match_times.empty()) {
        found_times.push_back(std::make_pair(match_times.top(), match_times.top()));
        match_times.pop();
    }

    return found_times;
}

void clear_sel() {
    std::vector<time_pair> del_times = find_sels();
    del_logs(del_times);
}

void print_sel() {
    std::vector<time_pair> print_times = find_sels();
    print_logs(print_times);
}

int main(int argc, char **argv)
{
    /*
     * JSON setup
     */

    std::ifstream fileStream("/etc/syseventgen.conf");
    if (!fileStream.is_open()) {
        std::cout << "Configuration file needed." << std::endl;
        std::cout << "Please create one and place it at /etc/syseventgen.conf" << std::endl;
        return -1;
    }
    nlohmann::json jsonFile = nlohmann::json::parse(fileStream);

    // get the delay from the JSON file
    unsigned int delay = jsonFile["delay"].get<unsigned int>();

    /*
     * CLI app setup
     */
    CLI::App app{"System Event Generation Util"};
    app.set_help_flag();
    app.set_help_all_flag("-h,--help");

    app.add_flag("-v,--verbose", verbose, "Turn on verbose outputs");

    app.add_subcommand("clear", "Get rid of all injected SEL's")->callback([&]() {
        clear_sel();
    });
    app.add_subcommand("print", "Print all of the injected SEL blocks")->callback([&]() {
        print_sel();
    });

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
                        try {
                            sensor_callback(
                                delay,
                                val["sel_subtypes"][0].get<uint8_t>(),
                                val["sel_subtypes"][1].get<std::string>(),
                                val["sel_subtypes"][2].get<std::string>());
                        } catch (const nlohmann::detail::type_error& e) {
                            sensor_callback(
                                delay,
                                val["sel_subtypes"][0].get<uint8_t>(),
                                val["sel_subtypes"][1].get<std::string>());
                        }
                        break;
                    case 1: // ipmi
                        ipmi_callback(
                            delay,
                            val.value("payload", ""),
                            val["sel_subtypes"][0].get<int>(),
                            val["sel_subtypes"][1].get<uint8_t>(),
                            val["sel_subtypes"][2].get<std::string>());
                        break;
                    case 2: // bic gpio
                        bic_gpio_callback(
                        delay,
                        val["sel_subtypes"][0].get<uint8_t>(),
                        val["sel_subtypes"][1].get<unsigned int>());
                        break;
                    case 3: // gpio
                        gpio_callback(
                            delay,
                            val["sel_subtypes"][0].get<unsigned int>());
                        break;
                }
            });
        }
    }
    app.require_subcommand(0, 1);
    CLI11_PARSE(app, argc, argv);
    if (app.get_subcommands().size() == 0) {
        std::cout << app.help() << std::endl;
    }
}
