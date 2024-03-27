#ifndef __GPIOD_H__$
#define __GPIOD_H__$

#include <openbmc/libgpio.h>
#include <stdint.h>
#include <stdbool.h>

#define POLL_TIMEOUT        -1
#define POWER_ON_STR        "on"
#define POWER_OFF_STR       "off"
#define DEFER_LOG_TIME      (300 * 1000)

struct delayed_prochot_log {
  gpiopoll_pin_t *desc;
  gpio_value_t last;
  gpio_value_t curr;
};

#define DELAY_LOG_CPU_PROCHOT_MS 100
#define DELAY_LOG_POWER_FAULT_MS 100
#define DELAY_CB_SET_PLDM_RECEIVER 10

bool server_power_check(uint8_t power_on_time);

int gpiod_plat_thread_create(void);
void gpiod_plat_thread_finish(void);

int get_gpios_common_list(struct gpiopoll_config** list);
int get_gpios_plat_list(struct gpiopoll_config** list);
void prochot_reason(char *reason);

//Thread Monitor
void *iox_gpio_handle(void*);
void *gpio_timer(void*);


//Normal GPIO EVENT
void gpio_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void gpio_event_pson_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void gpio_event_pson_3s_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//SGPIO EVENT
void sgpio_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//GPIO Presence EVENT
void gpio_present_init (gpiopoll_pin_t *desc, gpio_value_t value);
void gpio_present_handler (gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

void present_init (char* desc, gpio_value_t value);
void present_handle (char* desc, gpio_value_t value);

void enable_init (char* desc, gpio_value_t value);
void enable_handle (char* desc, gpio_value_t value);

void tpm_sync_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void tpm_irq_sync_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//DEBUG CARD EVENT
void usb_dbg_card_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void uart_select_handle(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//Power EVENT
void platform_reset_init(gpiopoll_pin_t *desc, gpio_value_t value);
void pwr_good_init(gpiopoll_pin_t *desc, gpio_value_t value);

void platform_reset_handle(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void pwr_reset_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void pwr_button_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void pwr_good_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

void pex_fw_ver_handle(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//CPU EVENT
void cpu_caterr_init(gpiopoll_pin_t *desc, gpio_value_t value);
void cpu_skt_init(gpiopoll_pin_t *desc, gpio_value_t value);
void cpu_vr_hot_init(gpiopoll_pin_t *desc, gpio_value_t value);

void cpu0_pvccin_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void cpu1_pvccin_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void cpu0_pvccd_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void cpu1_pvccd_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void cpu_prochot_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void cpu_thermtrip_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void cpu_error_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//HSC EVENT
void sml1_pmbus_alert_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void oc_detect_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void uv_detect_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//PCH EVENT
void pch_thermtrip_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//MEMORY EVENT
void mem_thermtrip_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//SWB EVENT
void bic_ready_init (gpiopoll_pin_t *desc, gpio_value_t value);
void bic_ready_handler (gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//GPU EVENT
void hmc_ready_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void hmc_ready_init(gpiopoll_pin_t *desc, gpio_value_t value);
void nv_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void gpu_present_init (char* desc, gpio_value_t value);
void gpu_present_handle (char* desc, gpio_value_t value);

void cpld_shut_down_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

#endif
