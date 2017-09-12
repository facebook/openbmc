/*
Copyright (c) 2017, Intel Corporation
Copyright (c) 2017, Facebook Inc.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 
    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "pin_interface.h"
#include "SoftwareJTAGHandler.h"
#include <pthread.h>
#include "openbmc/pal.h"
#include "openbmc/gpio.h"

/* Output pins */
#define DEBUG_EN_GPIO "GPIOB1"
gpio_st debug_en_gpio;

#define TCK_MUX_SEL_GPIO "GPIOR2"
gpio_st tck_mux_sel_gpio;

#define POWER_DEBUG_EN_GPIO "GPIOP6"
gpio_st power_debug_en_gpio;

#define PREQ_GPIO "GPIOP5"
gpio_st preq_gpio;

/* Input pins */
static void gpio_handle(gpio_poll_st *gpio);
static void *gpio_poll_thread(void *unused);
static gpio_poll_st g_gpios[3] = {
  {{0, 0}, GPIO_EDGE_BOTH, 0, gpio_handle, "GPIOR5", "RST_BMC_PLTRST_BUF_N"},
  {{0, 0}, GPIO_EDGE_FALLING, 0, gpio_handle, "GPIOR3", "BMC_PRDY_N"},
  {{0, 0}, GPIO_EDGE_BOTH, 0, gpio_handle, "GPIOR4", "BMC_XDP_PRSNT_IN_N"},
};
static bool g_gpios_triggered[3] = {false, false, false};
static pthread_mutex_t triggered_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t poll_thread;

/* In FBTP, PLTRST_BUF_N Pin of the CPU is directly connected to
 * the BMC GPIOR5. We need to monitor this and notify ASD. */
static gpio_st *platrst_gpio = &g_gpios[0].gs;

/* In FBTP, PRDY Pin of the CPU is directly connected to the
 * BMC GPIOR3. We need to monitor this and notify ASD. */
static gpio_st *prdy_gpio    = &g_gpios[1].gs;

/* In FBTP, GPIOR4 is connected to external circuitry which allows us
 * to detect if an XDP connector is connected to the target CPU. Thus
 * allowing us to disable ASD to avoid any conflicts on the JTAG bus.
 * NOTE: In FBTP this is currently impossible since the JTAG interface
 * is disabled in hardware as soon as an XDP connector is detected. But
 * have the functionality anyway since it does provide value in detecting
 * when an XDP is connected */
static gpio_st *xdp_present_gpio = &g_gpios[2].gs;

int pin_initialize(const int fru)
{
    int gpio;
    static bool gpios_polling = false;

    if (fru != FRU_MB)
        return ST_ERR;

    /* Platform specific enables which are required for the ASD feature */

    /* In FBTP, DEBUG_EN Pin of the CPU is directly connected to the BMC
     * GPIOB1 which needs to be set to 0 (active-low) to enable debugging.
     * XXX Note, this should have been set by default or done before the CPU
     * was started. This most probably has no effect if the CPU is already
     * running.
     * */
    gpio = gpio_num(DEBUG_EN_GPIO);
    if (gpio_export(gpio) ||
       gpio_open(&debug_en_gpio, gpio) != 0 ||
       gpio_change_direction(&debug_en_gpio, GPIO_DIRECTION_OUT)) {
      return ST_ERR;
    }
    gpio_write(&debug_en_gpio, GPIO_VALUE_LOW);

    /* In FBTP, POWER_DEBUG_EN Pin of the CPU is directly connected to the
     * BMC GPIOP6 which needs to be set to 0 (active-low) to enable power-debugging
     * needed to debug early-boot-stalls. */
    gpio = gpio_num(POWER_DEBUG_EN_GPIO);
    if (gpio_export(gpio) ||
       gpio_open(&power_debug_en_gpio, gpio) != 0 ||
       gpio_change_direction(&power_debug_en_gpio, GPIO_DIRECTION_IN)) {
      return ST_ERR;
    }

    /* In FBTP, JTAG_TCK Pin of the CPU is directly connected to the BMC (JTAG_TCK).
     * But that output can be connected to the CPU (default) or a second
     * destination (currently unconnected in DVT which may change in future).
     * This choice is made based on the output value of GPIOR2 thus,
     * here we are selecting it to go to the CPU. Potentially unnecessary but
     * we want to be future proofed. */
    gpio = gpio_num(TCK_MUX_SEL_GPIO);
    if (gpio_export(gpio) ||
       gpio_open(&tck_mux_sel_gpio, gpio) != 0 ||
       gpio_change_direction(&tck_mux_sel_gpio, GPIO_DIRECTION_OUT)) {
      return ST_ERR;
    }
    gpio_write(&tck_mux_sel_gpio, GPIO_VALUE_LOW);

    /* In FBTP, PREQ Pin of the CPU is directly connected to the BMC 
     * GPIOP5. It appears that the AST2500(or FBTP?) needs to have this 
     * set as an input until it's time to trigger otherwise it will cause a 
     * PREQ event For other setups, you may be able to set it to an output right away */
    gpio = gpio_num(PREQ_GPIO);
    if (gpio_export(gpio) ||
       gpio_open(&preq_gpio, gpio) != 0 ||
       gpio_change_direction(&preq_gpio, GPIO_DIRECTION_IN)) {
      return ST_ERR;
    }

    /* Start the GPIO polling threads just once */
    if (gpios_polling == false) {
      if (gpio_poll_open(g_gpios, 3)) {
        return ST_ERR;
      }

      pthread_create(&poll_thread, NULL, gpio_poll_thread, NULL);
      gpios_polling = true;
    } else {
      pthread_mutex_lock(&triggered_mutex);
      g_gpios_triggered[0] = false;
      g_gpios_triggered[1] = false;
      g_gpios_triggered[2] = false;
      pthread_mutex_unlock(&triggered_mutex);
    }

    return ST_OK;
}

int pin_deinitialize(const int fru)
{
    if (fru != FRU_MB)
        return ST_ERR;

    if (gpio_change_direction(&debug_en_gpio, GPIO_DIRECTION_IN)) {
        return ST_ERR;
    }
    gpio_close(&debug_en_gpio);
    gpio_close(&power_debug_en_gpio);

    if (gpio_change_direction(&tck_mux_sel_gpio, GPIO_DIRECTION_IN)) {
        return ST_ERR;
    }
    gpio_close(&tck_mux_sel_gpio);

    if (gpio_change_direction(&preq_gpio, GPIO_DIRECTION_IN)) {
        return ST_ERR;
    }
    gpio_close(&preq_gpio);

    /* Leave the GPIO polling threads alone. */
    //gpio_poll_close(g_gpios, 3);

    //pthread_join(poll_thread, NULL);

    return ST_OK;
}


int power_debug_assert(const int fru, const bool assert)
{
    return ST_OK;
}

int power_debug_is_asserted(const int fru, bool* asserted)
{
    gpio_value_en value;
    if (fru != FRU_MB)
      return ST_ERR;
    value = gpio_read(&power_debug_en_gpio);
    if (value == GPIO_VALUE_INVALID) {
      return ST_ERR;
    }
    *asserted = value == GPIO_VALUE_LOW ? true : false;
    return ST_OK;
}

int preq_assert(const int fru, const bool assert)
{
    if (fru != FRU_MB) {
      return ST_ERR;
    }
    /* The direction needs to be changed on demand to avoid
     * sending an unnecessary PREQ event at initialization.
     * See comment in pin_initialize() */
    if (gpio_change_direction(&preq_gpio, GPIO_DIRECTION_OUT)) {
      return ST_ERR;
    }
    /* Active low. */
    gpio_write(&preq_gpio, assert ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH);
    return ST_OK;
}

int preq_is_asserted(const int fru, bool* asserted)
{
    gpio_value_en value;
    if (fru != FRU_MB)
      return ST_ERR;
    value = gpio_read(&preq_gpio);
    if (value == GPIO_VALUE_INVALID) {
      return ST_ERR;
    }
    /* Active low. */
    *asserted = value == GPIO_VALUE_LOW ? true : false;
    return ST_OK;
}

static void gpio_handle(gpio_poll_st *gpio)
{
  size_t idx;
  pthread_mutex_lock(&triggered_mutex);
  /* Get the index of this GPIO in g_gpios */
  idx = (gpio - g_gpios);
  if (idx < 3) {
    g_gpios_triggered[idx] = true;
  }
  pthread_mutex_unlock(&triggered_mutex);
}

static void *gpio_poll_thread(void *unused)
{
  gpio_poll(g_gpios, 3, -1);
  return NULL;
}

int platform_reset_is_event_triggered(const int fru, bool* triggered)
{
    if (fru != FRU_MB) {
      return ST_ERR;
    }
    pthread_mutex_lock(&triggered_mutex);
    *triggered = g_gpios_triggered[0];
    g_gpios_triggered[0] = false;
    pthread_mutex_unlock(&triggered_mutex);
    return ST_OK;
}

int platform_reset_is_asserted(const int fru, bool* asserted)
{
    gpio_value_en value;
    if (fru != FRU_MB)
      return ST_ERR;
    value = gpio_read(platrst_gpio);
    if (value == GPIO_VALUE_INVALID) {
      return ST_ERR;
    }
    /* active-high */
    *asserted = value == GPIO_VALUE_HIGH ? true : false;
    return ST_OK;
}



int prdy_is_event_triggered(const int fru, bool* triggered)
{
    if (fru != FRU_MB)
        return ST_ERR;
    pthread_mutex_lock(&triggered_mutex);
    *triggered = g_gpios_triggered[1];
    g_gpios_triggered[1] = false;
    pthread_mutex_unlock(&triggered_mutex);
    return ST_OK;
}

int prdy_is_asserted(const int fru, bool* asserted)
{
    gpio_value_en value;
    if (fru != FRU_MB)
      return ST_ERR;
    value = gpio_read(prdy_gpio);
    if (value == GPIO_VALUE_INVALID) {
      return ST_ERR;
    }
    /* active-low */
    *asserted = value == GPIO_VALUE_LOW ? true : false;
    return ST_OK;
}

int xdp_present_is_event_triggered(const int fru, bool* triggered)
{
    if (fru != FRU_MB) {
      return ST_ERR;
    }
    /* TODO */
    pthread_mutex_lock(&triggered_mutex);
    *triggered = g_gpios_triggered[2];
    g_gpios_triggered[2] = false;
    pthread_mutex_unlock(&triggered_mutex);
    return ST_OK;
}

int xdp_present_is_asserted(const int fru, bool* asserted)
{
    gpio_value_en value;
    if (fru != FRU_MB)
      return ST_ERR;
    value = gpio_read(xdp_present_gpio);
    if (value == GPIO_VALUE_INVALID) {
      return ST_ERR;
    }
    /* active-low */
    *asserted = value == GPIO_VALUE_LOW ? true : false;
    return ST_OK;
}

int tck_mux_select_assert(const int fru, bool assert)
{
  gpio_write(&tck_mux_sel_gpio, assert ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW);
  return ST_OK;
}

