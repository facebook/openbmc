#include "pldm-update.hpp"

#include <cstdlib>
#include <sdbusplus/bus.hpp>
#include <thread>

void wait_for_device_reactivation_and_fetch_info()
{
    // This is a placeholder implementation and should be overridden
    // by platform-specific code if needed.

    // remove and re-add the device to make pldmd detect it again.

    // Wait for the device to reactivate and fetch its information.
    std::cout << "Waiting for device reactivation..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "Start removing device information..." << std::endl;
    auto bus = sdbusplus::bus::new_default();
    try {
        auto remove = bus.new_method_call(
            "au.com.codeconstruct.MCTP1",
            "/au/com/codeconstruct/mctp1/networks/1/endpoints/10",
            "au.com.codeconstruct.MCTP.Endpoint1",
            "Remove");
        auto reply = bus.call(remove);
        reply.read();
        std::cout << "Device information removed successfully." << std::endl;
    } catch (const sdbusplus::exception_t& e) {
        std::cerr << "Device information removed information failed: " << e.what() << std::endl;
    }

    std::cout << "Start re-adding device..." << std::endl;
    if (!retry([&]() {
        try {
            auto add = bus.new_method_call(
                "au.com.codeconstruct.MCTP1",
                "/au/com/codeconstruct/mctp1/interfaces/mctpi2c9",
                "au.com.codeconstruct.MCTP.BusOwner1",
                "LearnEndpoint");
            std::vector<uint8_t> endpoint = {0x20};
            add.append(endpoint);
            auto reply = bus.call(add);
            reply.read();
            std::cout << "Device re-added successfully." << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Device re-added failed: " << e.what() << std::endl;
            return false;
        }
    }, 3 /* attempts */))
    {
        std::cerr << "Failed to re-add device 3 times." << std::endl;
        return;
    }

    // Fetch device version information
    std::cout << "Fetching device information..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    [[maybe_unused]] int ret = system("bash -c 'nohup /usr/libexec/fw-versions/ag-bic  > /dev/null 2>&1 &'");
    return;
}