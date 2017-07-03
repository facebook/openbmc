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
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <syslog.h>
#include "openbmc/pal.h"
#include "openbmc/gpio.h"
#include <facebook/bic.h>

#define FBY2_DEBUG


//  PLTRST,  PRDY, XDP_PRESENT
enum  MONITOR_EVENTS {
  JTAG_PLTRST_EVENT   = 0,
  JTAG_PRDY_EVENT,
  JTAG_XDP_PRESENT_EVENT,
  JTAG_EVENT_NUM,
};
static bool g_gpios_triggered[JTAG_EVENT_NUM] = {false, false, false};


static pthread_mutex_t triggered_mutex = PTHREAD_MUTEX_INITIALIZER;


int pin_initialize(const int fru)
{
#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s, fru=%d", __FUNCTION__, fru);
#endif
    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return ST_ERR;
    }

    /* Platform specific enables which are required for the ASD feature */

    /* needs to
     *     - enable DEBUG_EN
     *     - enable POWER_DEBUG_EN  (XDP_BIC_PWR_DEBUG_N?)
     *     - FM_JTAG_BIC_TCK_MUX_SEL_N  (select ?)
     *
     *     starts 1 thread monitoring GPIO pins
     *         PLTRST_BUF_N - both edge
     *         PRDY   - fallign edge
     *         XDP_PRSNT_IN_N - both edges   (if triggered, disable asd)
     */


    pthread_mutex_lock(&triggered_mutex);
    g_gpios_triggered[JTAG_PLTRST_EVENT] = false;
    g_gpios_triggered[JTAG_PRDY_EVENT] = false;
    g_gpios_triggered[JTAG_XDP_PRESENT_EVENT] = false;
    pthread_mutex_unlock(&triggered_mutex);

    return ST_OK;
}

int pin_deinitialize(const int fru)
{
#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s, fru=%d", __FUNCTION__, fru);
#endif

    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return ST_ERR;
    }

    // TBD

    return ST_OK;
}


int power_debug_assert(const int fru, const bool assert)
{
#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s, fru=%d, assert=%d", __FUNCTION__, fru, assert);
#endif

    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return ST_ERR;
    }

#if 0
    /* Active low. */
    gpio_write(&power_debug_en_gpio, assert ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH);
#endif

    return ST_OK;
}

int power_debug_is_asserted(const int fru, bool* asserted)
{
#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s, fru=%d", __FUNCTION__, fru);
#endif
#if 0
    gpio_value_en value;
    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      printf("%s: invalid fru: %d", __FUNCTION__, fru);
      return ST_ERR;
    }
    value = gpio_read(&power_debug_en_gpio);
    if (value == GPIO_VALUE_INVALID) {
      return ST_ERR;
    }
    *asserted = value == GPIO_VALUE_LOW ? true : false;
#endif
#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s, asserted=%d\n", __FUNCTION__, *asserted);
#endif

    return ST_OK;
}

int preq_assert(const int fru, const bool assert)
{
    int ret;

#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s, fru=%d, assert=%d", __FUNCTION__, fru, assert);
#endif

    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return ST_ERR;
    }

    /* active low */
    ret = bic_set_gpio(fru, XDP_BIC_PREQ_N, assert ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH);

    return ret;
}

int preq_is_asserted(const int fru, bool* asserted)
{
#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s, fru=%d", __FUNCTION__, fru);
#endif
    bic_gpio_t gpio;
    int ret;

    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return ST_ERR;
    }

    ret = bic_get_gpio(fru, &gpio);
    if (!ret) {
      /* active low */
      *asserted = gpio.xdp_bic_preq_n == GPIO_VALUE_LOW ? true : false;
    }

#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s asserted=%d\n", __FUNCTION__, *asserted);
#endif

    return ret;
}
#if 0
static void gpio_handle(gpio_poll_st *gpio)
{
#if 0
  size_t idx;
  pthread_mutex_lock(&triggered_mutex);
  /* Get the index of this GPIO in g_gpios */
  idx = (g_gpios - gpio) / sizeof(gpio_poll_st);
  if (idx < 3) {
    g_gpios_triggered[idx] = true;
  }
  pthread_mutex_unlock(&triggered_mutex);
#endif
}


static void *gpio_poll_thread(void *unused)
{
#if 0
  gpio_poll(g_gpios, 3, -1);
#endif
  return NULL;
}
#endif

int platform_reset_is_event_triggered(const int fru, bool* triggered)
{
#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s, fru=%d", __FUNCTION__, fru);
#endif

    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return ST_ERR;
    }
    pthread_mutex_lock(&triggered_mutex);
    *triggered = g_gpios_triggered[JTAG_PLTRST_EVENT];
    g_gpios_triggered[JTAG_PLTRST_EVENT] = false;
    pthread_mutex_unlock(&triggered_mutex);
    /* TODO */
    *triggered = false;

#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s %d", __FUNCTION__, *triggered);
#endif

    return ST_OK;
}

int platform_reset_is_asserted(const int fru, bool* asserted)
{
#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s, fru=%d", __FUNCTION__, fru);
#endif
    bic_gpio_t gpio;
    int ret;

    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return ST_ERR;
    }

    ret = bic_get_gpio(fru, &gpio);
    if (!ret) {
      /* active low */
      *asserted = gpio.pltrst_n == GPIO_VALUE_LOW ? true : false;
    }

#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s asserted=%d", __FUNCTION__, *asserted);
#endif

    return ST_OK;
}



int prdy_is_event_triggered(const int fru, bool* triggered)
{
#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s, fru=%d", __FUNCTION__, fru);
#endif

    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return ST_ERR;
    }

    pthread_mutex_lock(&triggered_mutex);
    *triggered = g_gpios_triggered[JTAG_PRDY_EVENT];
    g_gpios_triggered[JTAG_PRDY_EVENT] = false;
    pthread_mutex_unlock(&triggered_mutex);
    /* TODO */

#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s triggered=%d\n", __FUNCTION__, (int)*triggered);
#endif

    return ST_OK;
}

int prdy_is_asserted(const int fru, bool* asserted)
{
#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s, fru=%d", __FUNCTION__, fru);
#endif
    bic_gpio_t gpio;
    int ret;

    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return ST_ERR;
    }

    ret = bic_get_gpio(fru, &gpio);
    if (!ret) {
      /* active low */
      // todo - current BIC does not return PRDY pin to BMC
      //*asserted = gpio.prdy == GPIO_VALUE_LOW ? true : false;
    }

#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s asserted=%d\n", __FUNCTION__, (int)*asserted);
#endif

    return ST_OK;
}

int xdp_present_is_event_triggered(const int fru, bool* triggered)
{
#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s, fru=%d", __FUNCTION__, fru);
#endif

    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return ST_ERR;
    }
    /* TODO */
    pthread_mutex_lock(&triggered_mutex);
    *triggered = g_gpios_triggered[JTAG_XDP_PRESENT_EVENT];
    g_gpios_triggered[JTAG_XDP_PRESENT_EVENT] = false;
    pthread_mutex_unlock(&triggered_mutex);

#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s asserted=%d\n", __FUNCTION__, (int)*triggered);
#endif

    return ST_OK;
}

int xdp_present_is_asserted(const int fru, bool* asserted)
{
#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s, fru=%d", __FUNCTION__, fru);
#endif

#if 0
    gpio_value_en value;
    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return NULL;
    }
    value = gpio_read(xdp_present_gpio);
    if (value == GPIO_VALUE_INVALID) {
      return ST_ERR;
    }
    /* active-low */
    *asserted = value == GPIO_VALUE_LOW ? true : false;
#endif

#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s asserted=%d\n", __FUNCTION__, *asserted);
#endif

    return ST_OK;
}
