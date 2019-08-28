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
#include <openbmc/pal.h>
#include <openbmc/libgpio.h>

/* Output pins */
gpio_desc_t *debug_en_gpio = NULL;
#define DEBUG_EN_GPIO "GPIOB1"

#define TCK_MUX_SEL_GPIO "GPIOR2"
gpio_desc_t *tck_mux_sel_gpio = NULL;

#define POWER_DEBUG_EN_GPIO "GPIOP6"
gpio_desc_t *power_debug_en_gpio = NULL;

#define PREQ_GPIO "GPIOP5"
gpio_desc_t *preq_gpio = NULL;

/* Input pins */
static void gpio_handle(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr);
static void *gpio_poll_thread(void *unused);
static struct gpiopoll_config g_gpios[3] = {
  {"RST_BMC_PLTRST_BUF_N", "0", GPIO_EDGE_BOTH, gpio_handle, NULL},
  {"BMC_PRDY_N", "1", GPIO_EDGE_FALLING, gpio_handle, NULL},
  {"BMC_XDP_PRSNT_IN_N", "2", GPIO_EDGE_BOTH, gpio_handle, NULL}
};
static gpiopoll_desc_t *polldesc = NULL;
static bool g_gpios_triggered[3] = {false, false, false};
static pthread_mutex_t triggered_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t poll_thread;

/* In FBTP, PLTRST_BUF_N Pin of the CPU is directly connected to
 * the BMC GPIOR5. We need to monitor this and notify ASD. */
static gpio_desc_t *platrst_gpio = NULL;

/* In FBTP, PRDY Pin of the CPU is directly connected to the
 * BMC GPIOR3. We need to monitor this and notify ASD. */
static gpio_desc_t *prdy_gpio = NULL;

/* In FBTP, GPIOR4 is connected to external circuitry which allows us
 * to detect if an XDP connector is connected to the target CPU. Thus
 * allowing us to disable ASD to avoid any conflicts on the JTAG bus.
 * NOTE: In FBTP this is currently impossible since the JTAG interface
 * is disabled in hardware as soon as an XDP connector is detected. But
 * have the functionality anyway since it does provide value in detecting
 * when an XDP is connected */
static gpio_desc_t *xdp_present_gpio = NULL;

int pin_initialize(const int fru)
{
    if (fru != FRU_MB)
        return ST_ERR;

    /* Platform specific enables which are required for the ASD feature */

    /* In FBTP, DEBUG_EN Pin of the CPU is directly connected to the BMC
     * GPIOB1 which needs to be set to 0 (active-low) to enable debugging.
     * XXX Note, this should have been set by default or done before the CPU
     * was started. This most probably has no effect if the CPU is already
     * running.
     * */
    debug_en_gpio = gpio_open_by_shadow("BMC_DEBUG_EN_N");
    if (!debug_en_gpio) {
      goto bail;
    }
    if (gpio_set_direction(debug_en_gpio, GPIO_DIRECTION_OUT) ||
        gpio_set_value(debug_en_gpio, GPIO_VALUE_LOW)) {
      goto debug_en_bail;
    }

    /* In FBTP, POWER_DEBUG_EN Pin of the CPU is directly connected to the
     * BMC GPIOP6 which needs to be set to 0 (active-low) to enable power-debugging
     * needed to debug early-boot-stalls. */
    power_debug_en_gpio = gpio_open_by_shadow("BMC_PWR_DEBUG_N");
    if (!power_debug_en_gpio) {
      goto debug_en_bail;
    }
    if (gpio_set_direction(power_debug_en_gpio, GPIO_DIRECTION_OUT))
    {
      goto power_dbg_bail;
    }

    /* In FBTP, JTAG_TCK Pin of the CPU is directly connected to the BMC (JTAG_TCK).
     * But that output can be connected to the CPU (default) or a second
     * destination (currently unconnected in DVT which may change in future).
     * This choice is made based on the output value of GPIOR2 thus,
     * here we are selecting it to go to the CPU. Potentially unnecessary but
     * we want to be future proofed. */
    tck_mux_sel_gpio = gpio_open_by_shadow("BMC_TCK_MUX_SEL");
    if (!tck_mux_sel_gpio) {
      goto power_dbg_bail;
    }
    if (gpio_set_direction(tck_mux_sel_gpio, GPIO_DIRECTION_OUT) ||
        gpio_set_value(tck_mux_sel_gpio, GPIO_VALUE_LOW)) {
      goto tck_mux_bail;
    }

    /* In FBTP, PREQ Pin of the CPU is directly connected to the BMC 
     * GPIOP5. It appears that the AST2500(or FBTP?) needs to have this 
     * set as an input until it's time to trigger otherwise it will cause a 
     * PREQ event For other setups, you may be able to set it to an output right away */
    preq_gpio = gpio_open_by_shadow("BMC_PREQ_N");
    if (!preq_gpio) {
      goto tck_mux_bail;
    }
    if (gpio_set_direction(preq_gpio, GPIO_DIRECTION_OUT)) {
      goto preq_bail;
    }

    /* Start the GPIO polling threads just once */
    if (polldesc == NULL) {
      polldesc = gpio_poll_open(g_gpios, 3);
      if (!polldesc) {
        goto preq_bail;
      }
      if (pthread_create(&poll_thread, NULL, gpio_poll_thread, NULL)) {
        goto poll_bail;
      }
    } else {
      pthread_mutex_lock(&triggered_mutex);
      g_gpios_triggered[0] = false;
      g_gpios_triggered[1] = false;
      g_gpios_triggered[2] = false;
      pthread_mutex_unlock(&triggered_mutex);
    }

    return ST_OK;
poll_bail:
    gpio_poll_close(polldesc);
    polldesc = NULL;
preq_bail:
    gpio_close(preq_gpio);
    preq_gpio = NULL;
tck_mux_bail:
    gpio_close(tck_mux_sel_gpio);
    tck_mux_sel_gpio = NULL;
power_dbg_bail:
    gpio_close(power_debug_en_gpio);
    power_debug_en_gpio = NULL;
debug_en_bail:
    gpio_close(debug_en_gpio);
    debug_en_gpio = NULL;
bail:
    return ST_ERR;
}

int pin_deinitialize(const int fru)
{
    if (fru != FRU_MB)
        return ST_ERR;

    if (gpio_set_direction(debug_en_gpio, GPIO_DIRECTION_IN)) {
        return ST_ERR;
    }
    gpio_close(debug_en_gpio);
    debug_en_gpio = NULL;
    gpio_close(power_debug_en_gpio);
    power_debug_en_gpio = NULL;

    if (gpio_set_direction(tck_mux_sel_gpio, GPIO_DIRECTION_IN)) {
        return ST_ERR;
    }
    gpio_close(tck_mux_sel_gpio);
    tck_mux_sel_gpio = NULL;

    if (gpio_set_direction(preq_gpio, GPIO_DIRECTION_IN)) {
        return ST_ERR;
    }
    gpio_close(preq_gpio);
    preq_gpio = NULL;

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
    gpio_value_t value;
    if (fru != FRU_MB)
      return ST_ERR;
    if (gpio_get_value(power_debug_en_gpio, &value)) {
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
    if (gpio_set_direction(preq_gpio, GPIO_DIRECTION_OUT)) {
      return ST_ERR;
    }
    /* Active low. */
    gpio_set_value(preq_gpio, assert ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH);
    return ST_OK;
}

int preq_is_asserted(const int fru, bool* asserted)
{
    gpio_value_t value;
    if (fru != FRU_MB)
      return ST_ERR;
    if (gpio_get_value(preq_gpio, &value)) {
      return ST_ERR;
    }
    /* Active low. */
    *asserted = value == GPIO_VALUE_LOW ? true : false;
    return ST_OK;
}

static void gpio_handle(gpiopoll_pin_t *gpio, gpio_value_t last, gpio_value_t curr)
{
  size_t idx;
  const struct gpiopoll_config *cfg = gpio_poll_get_config(gpio);
  if (!cfg) {
    return;
  }
  idx = atoi(cfg->description);
  if (idx < 3) {
    pthread_mutex_lock(&triggered_mutex);
    g_gpios_triggered[idx] = true;
    pthread_mutex_unlock(&triggered_mutex);
  }
}

static void *gpio_poll_thread(void *unused)
{

  gpio_poll(polldesc, -1);
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
  gpio_value_t value;

  if (!platrst_gpio) {
    platrst_gpio = gpio_open_by_shadow("RST_BMC_PLTRST_BUF_N");
    if (!platrst_gpio) {
      return ST_ERR;
    }
  }
  if (gpio_get_value(platrst_gpio, &value)) {
    return ST_ERR;
  }
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
  gpio_value_t value;
  if (!prdy_gpio) {
    prdy_gpio = gpio_open_by_shadow("BMC_PRDY_N");
    if (!prdy_gpio) {
      return ST_ERR;
    }
  }
  if (gpio_get_value(prdy_gpio, &value)) {
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
    gpio_value_t value;
    if (!xdp_present_gpio) {
      xdp_present_gpio = gpio_open_by_shadow("BMC_XDP_PRSNT_IN_N");
      if (!xdp_present_gpio) {
        return ST_ERR;
      }
    }
    if (gpio_get_value(xdp_present_gpio, &value)) {
      return ST_ERR;
    }
    /* active-low */
    *asserted = value == GPIO_VALUE_LOW ? true : false;
    return ST_OK;
}

int tck_mux_select_assert(const int fru, bool assert)
{
  int ret = gpio_set_value(tck_mux_sel_gpio, assert ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW);
  if (ret != 0)
    return ST_ERR;
  return ST_OK;
}

