#ifndef _PIN_HANDLER_H_
#define _PIN_HANDLER_H_

#include <poll.h>
#include "asd_common.h"
#include "mem_helper.h"
#include "logging.h"
#include <openbmc/libgpio.h>

#define POLL_GPIO (POLLPRI + POLLERR)

typedef enum
{
    READ_TYPE_MIN = -1,
    READ_TYPE_PROBE = 0,
    READ_TYPE_PIN,
    READ_TYPE_MAX  // Insert new read cfg indices before
                   // READ_STATUS_INDEX_MAX
} ReadType;

typedef enum
{
    WRITE_CONFIG_MIN = -1,
    WRITE_CONFIG_BREAK_ALL = 0,
    WRITE_CONFIG_RESET_BREAK,
    WRITE_CONFIG_REPORT_PRDY,
    WRITE_CONFIG_REPORT_PLTRST,
    WRITE_CONFIG_REPORT_MBP,
    WRITE_CONFIG_MAX  // Insert before WRITE_EVENT_CONFIG_MAX
} WriteConfig;

typedef struct event_configuration
{
    bool break_all;
    bool reset_break;
    bool report_PRDY;
    bool report_PLTRST;
    bool report_MBP;
} event_configuration;

typedef enum
{
    BMC_TCK_MUX_SEL = 0,
    BMC_PREQ_N,
    BMC_PRDY_N,
    BMC_RSMRST_B,
    BMC_CPU_PWRGD,
    BMC_PLTRST_B,
    BMC_SYSPWROK,
    BMC_PWR_DEBUG_N,  // sampled at power on for early break.
    BMC_DEBUG_EN_N,
    BMC_XDP_PRST_IN,
    POWER_BTN,
    RESET_BTN
} Target_Control_GPIOS;
#define NUM_GPIOS 10

// Maps from ASD Protocol pin definitions to BMC GPIOs
static const Target_Control_GPIOS ASD_PIN_TO_GPIO[] = {
    BMC_CPU_PWRGD,    // PIN_PWRGOOD
    BMC_PREQ_N,       // PIN_PREQ
    RESET_BTN,        // PIN_RESET_BUTTON
    POWER_BTN,        // PIN_POWER_BUTTON
    BMC_PWR_DEBUG_N,  // PIN_EARLY_BOOT_STALL
    BMC_SYSPWROK,     // PIN_SYS_PWR_OK
    BMC_PRDY_N,       // PIN_PRDY
    BMC_TCK_MUX_SEL,  // PIN_TCK_MUX_SELECT
};

#define NUM_DBUS_FDS 1
typedef struct pollfd target_fdarr_t[NUM_GPIOS + NUM_DBUS_FDS];

typedef STATUS (*TargetHandlerEventFunctionPtr)(void*, ASD_EVENT*);

typedef enum
{
    PIN_NONE,
    PIN_GPIO,
    PIN_DBUS,
    PIN_GPIOD
} Pin_Type;

typedef struct Target_Control_GPIO
{
    char name[32];
    int number;
    TargetHandlerEventFunctionPtr handler;
    int fd;
    gpio_value_t direction;
    gpio_edge_t edge;
    bool active_low;
    Pin_Type type;
} Target_Control_GPIO;

typedef struct Target_Control_Handle
{
    event_configuration event_cfg;
    bool initialized;
    Target_Control_GPIO gpios[NUM_GPIOS];
    bool is_master_probe;
    uint8_t fru;
} Target_Control_Handle;

void read_pin_value(uint8_t fru, Target_Control_GPIO gpio, int* value, STATUS* result);
void write_pin_value(uint8_t fru, Target_Control_GPIO gpio, int value, STATUS* result);
void get_pin_events(Target_Control_GPIO gpio, short* events);
STATUS bypass_jtag_message(uint8_t fru, struct asd_message* s_message);
STATUS pin_hndlr_init_target_gpios_attr(Target_Control_Handle *state);
STATUS pin_hndlr_init_asd_gpio(Target_Control_Handle *state);
STATUS pin_hndlr_deinit_asd_gpio(Target_Control_Handle *state);
short pin_hndlr_pin_events(Target_Control_GPIO gpio);
STATUS pin_hndlr_read_gpio_event(Target_Control_Handle* state, struct pollfd poll_fd, ASD_EVENT* event);
STATUS pin_hndlr_provide_GPIOs_list(Target_Control_Handle* state, target_fdarr_t* fds, int* num_fds);
#endif // _PIN_HANDLER_H_
