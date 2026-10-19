/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */
#include <amd-apml.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <systemd/sd-event.h>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

constexpr auto APML_DEV = "/dev/sbtsi-4c";

/**
 * @brief CPU temperature threshold for the first phase of the project.
 * Based on recent project testing, a unified threshold will be adopted for the first phase;
 * in the second phase, FRU and JSON configuration files may be utilized to implement project-specific settings.
 */
constexpr int CPU_TEMP_THRESHOLD = 85;
constexpr auto BOOT_PROGRESS_SERVICE = "xyz.openbmc_project.State.Host";
constexpr auto BOOT_PROGRESS_PATH = "/xyz/openbmc_project/state/host0";
constexpr auto BOOT_PROGRESS_INTERFACE = "xyz.openbmc_project.State.Boot.Progress";
constexpr auto OS_START_STAGE = "xyz.openbmc_project.State.Boot.Progress.ProgressStages.OSStart";
constexpr uint8_t TSI_TPR_THLD_REG = 0x07;// According to AMD PPR Specification, SBTSI::HiTempInt) offset 0x07

/**
 * @brief Set CPU temperature threshold using APML
 */
bool setCpuTempThreshold(int threshold) {
    int fd = open(APML_DEV, O_RDWR);
    if (fd < 0) {
        lg2::error("Failed to open {DEV}: {ERR}", "DEV", APML_DEV, "ERR", strerror(errno));
        return false;
    }

    struct apml_message msg = {};
    msg.cmd = APML_REG;

    *reinterpret_cast<uint32_t*>(&msg.data_in.reg_in[REG_OFF_INDEX]) = TSI_TPR_THLD_REG;
    msg.data_in.reg_in[REG_VAL_INDEX] = static_cast<uint8_t>(threshold);
    msg.data_in.reg_in[RD_FLAG_INDEX] = 0; // 0 write.

    if (ioctl(fd, SBRMI_IOCTL_CMD, &msg) < 0) {
        lg2::error("Failed to perform ioctl on {DEV}: {ERR}", "DEV", APML_DEV, "ERR", strerror(errno));
        close(fd);
        return false;
    }

    if (msg.fw_ret_code != 0) {
        lg2::error("APML command failed with fw code: {CODE}", "CODE", static_cast<uint32_t>(msg.fw_ret_code));
        close(fd);
        return false;
    }

    lg2::info("Successfully set CPU temperature threshold to {THLD}C.", "THLD", threshold);
    close(fd);
    return true;
}

/**
 * @brief Judge Boot Progress stage
 */
void processBootProgress(const std::string& stage) {
    if (stage == OS_START_STAGE) {
        lg2::info("BIOS POST Completed (OSStart stage detected). Setting CPU temperature threshold.");
        setCpuTempThreshold(CPU_TEMP_THRESHOLD);
    }
}

/**
 * @brief D-Bus PropertiesChanged callback.
 */
void onBootProgressChanged(sdbusplus::message_t& msg) {
    std::string interface;
    std::map<std::string, std::variant<std::string>> properties;
    std::vector<std::string> invalidatedProperties; // match sa{sv}as signature as

    try {
        msg.read(interface, properties, invalidatedProperties);
    } catch (const sdbusplus::exception::exception& e) {
        lg2::error("Failed to read D-Bus message: {ERR}", "ERR", e.what());
        return;
    }

    auto it = properties.find("BootProgress");
    if (it != properties.end()) {
        if (const auto* stage = std::get_if<std::string>(&it->second)) {
            processBootProgress(*stage);
        }
    }
}

int main() {
    auto bus = sdbusplus::bus::new_default();

    // Create and bind sd_event loop
    sd_event* event = nullptr;
    if (sd_event_default(&event) < 0) {
        lg2::error("Failed to create sd_event loop");
        return 1;
    }
    bus.attach_event(event, SD_EVENT_PRIORITY_NORMAL);

    lg2::info("CPU Threshold Monitor Daemon started. Monitoring BootProgress...");

    // Register a listener for propertiesChanged signal
    sdbusplus::bus::match_t match(
        bus,
        sdbusplus::bus::match::rules::propertiesChanged(
            BOOT_PROGRESS_PATH, BOOT_PROGRESS_INTERFACE),
        onBootProgressChanged);

    // Daemon startup queries current status to prevent startup when Host is already in OSStart
    try {
        auto method = bus.new_method_call(
            BOOT_PROGRESS_SERVICE, BOOT_PROGRESS_PATH,
            "org.freedesktop.DBus.Properties", "Get");
        method.append(BOOT_PROGRESS_INTERFACE, "BootProgress");

        auto reply = bus.call(method);
        std::variant<std::string> bootProgressVal;
        reply.read(bootProgressVal);

        if (const auto* stage = std::get_if<std::string>(&bootProgressVal)) {
            processBootProgress(*stage);
        }
    } catch (const std::exception& e) {
        lg2::warning("Initial BootProgress property query failed: {ERR}", "ERR", e.what());
    }

    // Enter the event loop until the service is stopped externally
    int ret = sd_event_loop(event);

    sd_event_unref(event);
    return ret;
}