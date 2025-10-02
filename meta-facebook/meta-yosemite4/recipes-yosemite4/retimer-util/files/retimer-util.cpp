#include <openbmc/aries_common.h>
#include <openbmc/misc-utils.h>
#include <openbmc/plat.h>
#include <openbmc/kv.hpp>

#include <CLI/CLI.hpp>

#include <cstdio>
#include <iostream>
#include <string>

extern "C"
{
    extern void plat_rt_preinit(void*);
}

using namespace std;

#define RETIMER_SWITCH_BUS 0x0A
#define AL_RETIMER_ADDR 0x46

#define RETIMER_TYPE_BYTE 10
#define BIC_RX_GET_GPIO_CMD_STATUS_OFFSET 3

enum retimer_type {
    RETIMER_ASTERA = 0,
    RETIMER_KANDOU = 1,
    RETIMER_BROADCOM = 2,
    RETIMER_UNKNOWN = 0xFF
};

static constexpr auto RETIMER_ID = 0x0;
static constexpr auto HEARTBEAT = 0x1;
static constexpr auto CODE_LOAD = 0x2;

const char* PLDM_Prefix = "pldmtool raw -m";
const char* PLDM_RxPrefix = "pldmtool: Rx: ";
const char* BIC_Sensor_Monitor_Ctrl = "-d 0x80 0x3f 0x1 0x15 0xa0 0x00 0xE0 0x30 0x15 0xA0 0x00";
const char* BIC_Retimer_Type_GPIO_L7 = "-d 0x80 0x02 0x3a 0x5F 0xFF";
const char* BIC_Retimer_Type_GPIO_M2 = "-d 0x80 0x02 0x3a 0x61 0xFF";

static int mb_retimer_lock(uint8_t eid)
{
    std::string str_eid = std::to_string(eid);
    return single_instance_lock_blocked(std::string(str_eid + "_retimer").c_str());
}

static void mb_retimer_unlock(int lock)
{
    if (lock >= 0)
    {
        single_instance_unlock(lock);
    }
}

static bool lock_and_execute(uint8_t eid, std::function<void()> func)
{
    int lock = -1;
    std::string str_eid = std::to_string(eid);
    if ((lock = mb_retimer_lock(eid)) < 0)
    {
        std::cerr << "Cannot get retimer lock for BIC: " << str_eid << std::endl;
        return false;
    }
    func();
    mb_retimer_unlock(lock);
    return true;
}

int run_pldm_command(const char* cmd, uint8_t* rx_data_buf, int stat_byte)
{
    FILE *fp = NULL;
    fp = popen(cmd, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to execute command after retries\n");
        return -1;
    }

    return pldm_get_response(fp, rx_data_buf, stat_byte);
}

int BIC_set_sensor_monitor_state(uint8_t eid, uint8_t state)
{
    int rc;
    char command[1024];
    uint8_t pbRx[PLDM_RX_SIZE];

    sprintf(command,"%s %d %s %d", PLDM_Prefix, eid, BIC_Sensor_Monitor_Ctrl, state);

    rc = run_pldm_command(command, pbRx, BIC_RX_CMD_STATUS_OFFSET);
    if (rc == -1)
    {
        return -1;
    }
    return 0;
}

static int get_retimer_type(uint8_t eid)
{
    int rc;
    char command[1024];
    uint8_t RTM_TYPE_0, RTM_TYPE_1;
    uint8_t pbRx[PLDM_RX_SIZE];

    // Read GPIO RTM_TYPE_0 and RTM_TYPE_1 to check retimer type
    sprintf(command,"%s %d %s", PLDM_Prefix, eid, BIC_Retimer_Type_GPIO_L7);
    rc = run_pldm_command(command, pbRx, BIC_RX_GET_GPIO_CMD_STATUS_OFFSET);
    if (rc == -1)
    {
        return -1;
    }
    RTM_TYPE_0 = pbRx[RETIMER_TYPE_BYTE];

    sprintf(command,"%s %d %s", PLDM_Prefix, eid, BIC_Retimer_Type_GPIO_M2);
    rc = run_pldm_command(command, pbRx, BIC_RX_GET_GPIO_CMD_STATUS_OFFSET);
    if (rc == -1)
    {
        return -1;
    }
    RTM_TYPE_1 = pbRx[RETIMER_TYPE_BYTE];

    // RTM_TYPE_1 | RTM_TYPE_0 == 0101 => Astera
    // RTM_TYPE_1 | RTM_TYPE_0 == 0102 => No retimer
    // RTM_TYPE_1 | RTM_TYPE_0 == 0201 => Kandou
    // RTM_TYPE_1 | RTM_TYPE_0 == 0202 => Broadcom
    if (RTM_TYPE_1 == 1 && RTM_TYPE_0 == 1)
    {
        return RETIMER_ASTERA;
    }
    else if (RTM_TYPE_1 == 2 && RTM_TYPE_0 == 1)
    {
        return RETIMER_KANDOU;
    }
    else if (RTM_TYPE_1 == 2 && RTM_TYPE_0 == 2)
    {
        return RETIMER_BROADCOM;
    }
    else
    {
        return RETIMER_UNKNOWN;
    }

}

static void do_print_health(uint8_t eid)
{
    lock_and_execute(eid, [&]() {
        uint8_t health = 0;
        auto retimer_type = get_retimer_type(eid);

        if (retimer_type == RETIMER_ASTERA)
        {
            struct retimer_config config = {
                .slot_id = eid,
                .type = ARIES_PTX08,
                .retimer_bus = RETIMER_SWITCH_BUS, // 0'base
                .retimer_addr = AL_RETIMER_ADDR,   // 8 bits
                .retimer_width = 4,                // Yosemite4 only use 4
            };

            if (BIC_set_sensor_monitor_state(eid, 0))
            {
                throw runtime_error("failed to disable BIC sensor polling");
            }

            plat_rt_preinit((void*)(&config));
            AriesGetHealth(&health);

            if (BIC_set_sensor_monitor_state(eid, 1))
            {
                throw runtime_error("failed to enable BIC sensor polling");
            }

            std::cout << "heartbeat: "
                      << ((health & HEARTBEAT) ? "good" : "bad") << std::endl;
            std::cout << "code load status: "
                      << ((health & CODE_LOAD) ? "good" : "bad") << std::endl;
        }
        else
        {
            std::cerr << "Get retimer health failed: Invalid / not supporting retimer type.\n";
        }
    });
}

static void do_margin(uint8_t eid)
{
    lock_and_execute(eid, [&]() {
        auto retimer_type = get_retimer_type(eid);

        if (retimer_type == RETIMER_ASTERA)
        {
            struct retimer_config config = {
                .slot_id = eid,
                .type = ARIES_PTX08,
                .retimer_bus = RETIMER_SWITCH_BUS, // 0'base
                .retimer_addr = AL_RETIMER_ADDR,   // 8 bits
                .retimer_width = 4,                // Yosemite4 only use 4
            };

            if (BIC_set_sensor_monitor_state(eid, 0))
            {
                throw runtime_error("failed to disable BIC sensor polling");
            }

            plat_rt_preinit((void*)(&config));
            AriesMargin();

            if (BIC_set_sensor_monitor_state(eid, 1))
            {
                throw runtime_error("failed to enable BIC sensor polling");
            }

        }
        else
        {
            std::cerr << "Margin test failed: Invalid / not supporting retimer type.\n";
        }
    });
}

static void do_print_link(uint8_t eid)
{
    lock_and_execute(eid, [&]() {
        auto retimer_type = get_retimer_type(eid);

        if (retimer_type == RETIMER_ASTERA)
        {
            struct retimer_config config = {
                .slot_id = eid,
                .type = ARIES_PTX08,
                .retimer_bus = RETIMER_SWITCH_BUS, // 0'base
                .retimer_addr = AL_RETIMER_ADDR,   // 8 bits
                .retimer_width = 4,                // Yosemite4 only use 4
            };

            if (BIC_set_sensor_monitor_state(eid, 0))
            {
                throw runtime_error("failed to disable BIC sensor polling");
            }

            plat_rt_preinit((void*)(&config));
            AriesPrintState();

            if (BIC_set_sensor_monitor_state(eid, 1))
            {
                throw runtime_error("failed to enable BIC sensor polling");
            }

        }
        else
        {
            std::cerr << "Dump link status failed: Invalid / not supporting retimer type.\n";
        }
    });
}

int main(int argc, char* argv[])
{
    size_t eid;

    CLI::App app("Retimer Helper Utility");
    app.failure_message(CLI::FailureMessage::help);

    app.add_option("EID", eid, "BIC EID: 10 ~ 80")
        ->check(CLI::IsMember({10, 20, 30, 40, 50, 60, 70, 80}))
        ->required();
    app.require_subcommand(1);

    /* Get health */
    auto health = app.add_subcommand("health", "Get retimer health");
    health->callback([&]() { do_print_health(eid); });

    /* Perform margin Test */
    auto margin = app.add_subcommand("margin", "Perform margin test");
    margin->callback([&]() { do_margin(eid); });

    /* Dump link status */
    auto link = app.add_subcommand("link", "Dump link status");
    link->callback([&]() { do_print_link(eid); });

    CLI11_PARSE(app, argc, argv);

    return 0;
}
