#ifndef __GPIOD_H__$
#define __GPIOD_H__$

#include <openbmc/libgpio.h>
#include <stdint.h>
#include <stdbool.h>

#define POLL_TIMEOUT        -1
#define POWER_ON_STR        "on"
#define POWER_OFF_STR       "off"
#define DEFER_LOG_TIME      (300 * 1000)

bool server_power_check(uint8_t power_on_time);

//Normal GPIO EVENT
void gpio_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void gpio_event_pson_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void gpio_event_pson_3s_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//SGPIO EVENT
void sgpio_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);


//DEBUG CARD EVENT
void usb_dbg_card_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void uart_select_handle(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//Power EVENT
void platform_reset_init(gpiopoll_pin_t *desc, gpio_value_t value);

void platform_reset_handle(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void pwr_reset_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void pwr_button_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void pwr_good_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

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

// HMC Ready event
void hmc_ready_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void hmc_ready_init(gpiopoll_pin_t *desc, gpio_value_t value);

//HSC EVENT
void sml1_pmbus_alert_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void oc_detect_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);
void uv_detect_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//PCH EVENT
void pch_thermtrip_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//MEMORY EVENT
void mem_thermtrip_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//Thread Monitor
void *iox_gpio_handle(void*);
void *gpio_timer(void*);

//GPIO Present Event
void gpio_present_init (gpiopoll_pin_t *desc, gpio_value_t value);
void gpio_present_handler (gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//SWB EVENT
void bic_ready_init (gpiopoll_pin_t *desc, gpio_value_t value);
void bic_ready_handler (gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

//GPU EVENT
void nv_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr);

#endif
