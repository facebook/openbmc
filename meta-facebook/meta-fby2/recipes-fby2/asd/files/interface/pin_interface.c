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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "openbmc/pal.h"
#include "openbmc/gpio.h"
#include <facebook/bic.h>
#include <facebook/fby2_sensor.h>

//#define FBY2_DEBUG

//  PLTRST,  PRDY, XDP_PRESENT
enum  MONITOR_EVENTS {
  JTAG_PLTRST_EVENT   = 0,
  JTAG_PRDY_EVENT,
  JTAG_XDP_PRESENT_EVENT,
  JTAG_EVENT_NUM,
};
static bool g_gpios_triggered[JTAG_EVENT_NUM] = {false, false, false};


static pthread_mutex_t triggered_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t poll_thread;

static void *gpio_poll_thread(void *arg);

static int sph_pin_conf(uint8_t slot_id) {
  int ret = 0, retry = 5, bus = 0;
  uint8_t tbuf[256] = {0x18, 0x52, 0x05, 0xD4, 0x05, 0x8E, 0x8, 0x08, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0xB, 0xB, 0x8, 0x8, 0x8, 0x8, 0x8, 0xB};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 21;
  uint8_t rlen = 0;

  bus = pal_dev_jtag_gpio_to_bus(slot_id);

  tbuf[2] = bus;

  // Read existing FPGA Output pins status.
  while (retry >= 0) {
    ret =  bic_ipmb_wrapper(slot_id, tbuf[0] >> 2, tbuf[1], &tbuf[2], tlen-2, rbuf, &rlen);
    if (ret == 0) 
     break;
    retry--;
  }

  if (ret) {
    syslog(LOG_ERR, "%s: BIC no response!\n", __FUNCTION__);
    return ret;
  }

  return 0;
}

int pin_initialize(const int fru)
{
  static bool gpios_polling = false;
  int ret = 0;

#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s, fru=%d", __FUNCTION__, fru);
#endif

  if (fby2_get_slot_type(fru) == SLOT_TYPE_GPV2) {
    ret = sph_pin_conf(fru);
    if (ret != 0) {
      syslog(LOG_ERR, "%s: pin_conf failed, fru=%d", __FUNCTION__, fru);
      return ST_ERR;
    }
  } else {
    // For Twinlake ASD
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
    // enable DEBUG_EN  = XDP_PRSNT_OUT_N pin
    // active low
    if (bic_set_gpio(fru, XDP_PRSNT_OUT_N, GPIO_VALUE_LOW)) {
      syslog(LOG_ERR, "%s: assert XDP_PRSNT_OUT_N failed, fru=%d",
             __FUNCTION__, fru);
      return ST_ERR;
    }

    // enable FM_JTAG_BIC_TCK_MUX_SEL_N = FM_BIC_JTAG_SEL_N pin
    // active low
    if (bic_set_gpio(fru, FM_JTAG_BIC_TCK_MUX_SEL_N, GPIO_VALUE_LOW)) {
      syslog(LOG_ERR, "%s: assert FM_JTAG_BIC_TCK_MUX_SEL_N failed, fru=%d",
             __FUNCTION__, fru);
      return ST_ERR;
    };

    // set FM_BIC_JTAG_SEL_N = 1 to enable ASD
    if (bic_set_gpio(fru, FM_BIC_JTAG_SEL_N, GPIO_VALUE_HIGH)) {
      syslog(LOG_ERR, "%s: assert FM_BIC_JTAG_SEL_N failed, fru=%d",
             __FUNCTION__, fru);
      return ST_ERR;
    };


    /* Start the GPIO polling threads just once */
    if (gpios_polling == false) {
        pthread_create(&poll_thread, NULL, gpio_poll_thread, (void *)fru);
        gpios_polling = true;
    } else {
        pthread_mutex_lock(&triggered_mutex);
        g_gpios_triggered[JTAG_PLTRST_EVENT] = false;
        g_gpios_triggered[JTAG_PRDY_EVENT] = false;
        g_gpios_triggered[JTAG_XDP_PRESENT_EVENT] = false;
        pthread_mutex_unlock(&triggered_mutex);
    }
  }

  return ST_OK;
}

int pin_deinitialize(const int fru)
{
#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s, fru=%d", __FUNCTION__, fru);
#endif
    if (fby2_get_slot_type(fru) != SLOT_TYPE_GPV2) {
      if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
        syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
        return ST_ERR;
      }

      // disable DEBUG_EN  = XDP_PRSNT_OUT_N pin
      // active low
      if (bic_set_gpio(fru, XDP_PRSNT_OUT_N, GPIO_VALUE_HIGH)) {
        syslog(LOG_ERR, "%s: de-assert XDP_PRSNT_OUT_N failed, fru=%d",
               __FUNCTION__, fru);
        return ST_ERR;
      }


      // disable FM_JTAG_BIC_TCK_MUX_SEL_N = FM_BIC_JTAG_SEL_N pin
      // active low
      if (bic_set_gpio(fru, FM_JTAG_BIC_TCK_MUX_SEL_N, GPIO_VALUE_HIGH)) {
        syslog(LOG_ERR, "%s: de-assert FM_JTAG_BIC_TCK_MUX_SEL_N failed, fru=%d",
               __FUNCTION__, fru);
        return ST_ERR;
      };

      // set FM_BIC_JTAG_SEL_N = 0 to disable ASD
      if (bic_set_gpio(fru, FM_BIC_JTAG_SEL_N, GPIO_VALUE_LOW)) {
        syslog(LOG_ERR, "%s: assert FM_BIC_JTAG_SEL_N failed, fru=%d",
               __FUNCTION__, fru);
        return ST_ERR;
      };
    }

    return ST_OK;
}


// power_debug_assert  pin = "XDP_BIC_PWR_DEBUG_N", xdp_bic_pwr_debug_n
int power_debug_assert(const int fru, const bool assert)
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
    ret = bic_set_gpio(fru, XDP_BIC_PWR_DEBUG_N, assert ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH);

    return ret;
}

// power_debug_assert  pin = "XDP_BIC_PWR_DEBUG_N",  xdp_bic_pwr_debug_n
int power_debug_is_asserted(const int fru, bool* asserted)
{
    uint8_t gpio;
    int ret;

    if (fby2_get_slot_type(fru) != SLOT_TYPE_GPV2) {
      if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
        syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
        return ST_ERR;
      }

      ret = bic_get_gpio_status(fru, XDP_BIC_PWR_DEBUG_N, &gpio);
      if (!ret) {
        /* active low */
        *asserted = gpio == GPIO_VALUE_LOW ? true : false;
      }
#ifdef FBY2_DEBUG
    if (*asserted)
      syslog(LOG_DEBUG, "%s, fru=%d asserted", __FUNCTION__, fru);
#endif
      return ret;
    } else {
      return 0;
    }
}

static int sph_preq(uint8_t slot_id, uint8_t *gpio) {
  int ret = 0, retry = 5, bus = 0;
  uint8_t tbuf[256] = {0x18, 0x52, 0x05, 0xD4, 0x05, 0x8E};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 6;
  uint8_t rlen = 0;

  bus = pal_dev_jtag_gpio_to_bus(slot_id);
  tbuf[2] = bus;

  // Read existing FPGA Output pins status.
  while (retry >= 0) {
    ret =  bic_ipmb_wrapper(slot_id, tbuf[0]>>2, tbuf[1], &tbuf[2], tlen-2, rbuf, &rlen);
    if (ret == 0) 
     break;
    retry--;

  }

  *gpio = (1 & (rbuf[2] >> 6));

  return 0;
}

static int
sph_preq_set(uint8_t slot_id, uint8_t select) {
  int i = 0, ret = 0, retry = 5, bus = 0;
  uint8_t tbuf[256] = {0x18, 0x52, 0x05, 0xD4, 0x05, 0x8D};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 6;
  uint8_t rlen = 0;

  bus = pal_dev_jtag_gpio_to_bus(slot_id);
  tbuf[2] = bus;

  // Read existing FPGA Output pins status.
  while (retry >= 0) {
    ret =  bic_ipmb_wrapper(slot_id, tbuf[0]>>2, tbuf[1], &tbuf[2], tlen-2, rbuf, &rlen);
    if (ret == 0) 
     break;
    retry--;
  }

  for (i = 0; i < rlen; i++) {
    tbuf[6 + i] = rbuf[i];
  }
  tlen += (rlen - 1);
  {
    tbuf[8] =  tbuf[8] & 0x7F;
    if( select == 0 )
      tbuf[8] |= 0 << 7;
    else
      tbuf[8] |= 1 << 7;
  
    retry = 5;
    while (retry >= 0) {
      ret = bic_ipmb_wrapper(slot_id, tbuf[0]>>2, tbuf[1], &tbuf[2], tlen-2, rbuf, &rlen);
      if (ret == 0)
        break;

      retry--;
    }
    if (ret) {
      syslog(LOG_ERR, "%s: BIC no response!\n", __FUNCTION__);
      return ret;
    }
  }

  return 0;
}

// preq pin = ""XDP_BIC_PREQ_N"", xdp_bic_preq_n
int preq_assert(const int fru, const bool assert)
{
    int ret;

#ifdef FBY2_DEBUG
    syslog(LOG_DEBUG, "%s, fru=%d, assert=%d", __FUNCTION__, fru, assert);
#endif
    if (fby2_get_slot_type(fru) != SLOT_TYPE_GPV2) {
      if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
        syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
        return ST_ERR;
      }

      /* active low */
      ret = bic_set_gpio(fru, XDP_BIC_PREQ_N, assert ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH);
    } else {
      ret = sph_preq_set(fru, assert ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH);
    }

    return ret;
}

// preq pin = ""XDP_BIC_PREQ_N"", xdp_bic_preq_n
int preq_is_asserted(const int fru, bool* asserted)
{
    uint8_t gpio;
    int ret;
    if (fby2_get_slot_type(fru) != SLOT_TYPE_GPV2) {
      if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
        syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
        return ST_ERR;
      }

      ret = bic_get_gpio_status(fru, XDP_BIC_PREQ_N, &gpio);
      if (!ret) {
        /* active low */
        *asserted = gpio == GPIO_VALUE_LOW ? true : false;
      }
    } else {
      ret = sph_preq(fru, &gpio);
      if (!ret) {
        /* active low */
        *asserted = gpio == GPIO_VALUE_LOW ? true : false;
      }
    }

#ifdef FBY2_DEBUG
    if (*asserted)
      syslog(LOG_DEBUG, "%s, fru=%d asserted", __FUNCTION__, fru);
#endif

    return ret;
}

static int
sph_prdy_get(uint8_t slot_id, uint8_t* gpio) {
  int ret = 0, retry = 5, bus = 0;
  uint8_t tbuf[256] = {0x18, 0x52, 0x05, 0xD4, 0x05, 0x8E};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 6;
  uint8_t rlen = 0;

  bus = pal_dev_jtag_gpio_to_bus(slot_id);
  tbuf[2] = bus;

  // Read existing FPGA Output pins status.
  while (retry >= 0) {
    ret =  bic_ipmb_wrapper(slot_id, tbuf[0]>>2, tbuf[1], &tbuf[2], tlen-2, rbuf, &rlen);
    if (ret == 0) 
     break;
    retry--;
  }

  *gpio = ( 1 & (rbuf[2] >> 6)  );

  return 0;
}

// open a socket and waits for communcation from ipmid
static void *gpio_poll_thread(void *fru)
{
  int sock, msgsock, n, len, gpio_pin, ret=0;
  size_t t;
  struct sockaddr_un server, client;
  unsigned char req_buf[256];
  char sock_path[64] = {0};

  if ((sock = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
  {
    syslog(LOG_ERR, "ASD_BIC: socket() failed\n");
    exit (1);
  }

  server.sun_family = AF_UNIX;
  sprintf(sock_path, "%s_%d", SOCK_PATH_ASD_BIC, (int)fru);
  strcpy(server.sun_path, sock_path);
  unlink (server.sun_path);
  len = strlen (server.sun_path) + sizeof (server.sun_family);
  if (bind (sock, (struct sockaddr *) &server, len) == -1)
  {
    syslog(LOG_ERR, "ASD_BIC: bind() failed, errno=%d", errno);
    exit (1);
  }
  syslog(LOG_DEBUG, "ASD_BIC: Socket has name %s", server.sun_path);
  if (listen (sock, 5) == -1)
  {
    syslog(LOG_ERR, "ASD_BIC: listen() failed, errno=%d", errno);
    exit (1);
  }

  while (1) {
    t = sizeof (client);
    if ((msgsock = accept(sock, (struct sockaddr *) &client, &t)) < 0) {
      ret = errno;
      syslog(LOG_WARNING, "ASD_BIC: accept() failed with ret: %x, errno: %x\n",
             msgsock, ret);
      sleep(1);
      continue;
    }

    n = recv(msgsock, req_buf, sizeof(req_buf), 0);
    if (n <= 0) {
        syslog(LOG_WARNING, "ASD_BIC: recv() failed with %d\n", n);
        sleep(1);
        continue;
    } else {
#ifdef FBY2_DEBUG
        syslog(LOG_DEBUG, "message received, %d %d", req_buf[0], req_buf[1]);
#endif
        gpio_pin = req_buf[0];
        pthread_mutex_lock(&triggered_mutex);
        switch (gpio_pin) {
          case PLTRST_N:
#ifdef FBY2_DEBUG
            syslog(LOG_DEBUG, "ASD_BIC: PLTRST_N event");
#endif
            g_gpios_triggered[JTAG_PLTRST_EVENT] = true;;
            break;
          case  XDP_BIC_PRDY_N:
#ifdef FBY2_DEBUG
            syslog(LOG_DEBUG, "ASD_BIC: PRDY event, trigger type=%d", req_buf[1]);
#endif
            // only record falling edge
            if (req_buf[1] == 0)
                g_gpios_triggered[JTAG_PRDY_EVENT] = true;;
            break;
          case  XDP_PRSNT_IN_N:
#ifdef FBY2_DEBUG
            syslog(LOG_DEBUG, "ASD_BIC: XDP_PRESENT event, trigger type=%d", req_buf[1]);
#endif
            // triggers on falling edge
            if (req_buf[1] == 0)
                g_gpios_triggered[JTAG_XDP_PRESENT_EVENT] = true;;
            break;
          default:
            syslog(LOG_ERR, "ASD BIC: unknown GPIO pin # received, %d", gpio_pin);
        }
        pthread_mutex_unlock(&triggered_mutex);
    }
    close(msgsock);
  }
  close(sock);

  return NULL;
}


int platform_reset_is_event_triggered(const int fru, bool* triggered)
{
    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return ST_ERR;
    }
    pthread_mutex_lock(&triggered_mutex);
    *triggered = g_gpios_triggered[JTAG_PLTRST_EVENT];
    g_gpios_triggered[JTAG_PLTRST_EVENT] = false;
    pthread_mutex_unlock(&triggered_mutex);

#ifdef FBY2_DEBUG
    if (*triggered)
      syslog(LOG_DEBUG, "%s fru=%d, triggered", __FUNCTION__, fru);
#endif

    return ST_OK;
}



// platform_reset pin = pltrst_n
int platform_reset_is_asserted(const int fru, bool* asserted)
{
    uint8_t gpio;
    int ret;
    if (fby2_get_slot_type(fru) != SLOT_TYPE_GPV2) {
      if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
        syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
        return ST_ERR;
      }

      ret = bic_get_gpio_status(fru, PLTRST_N, &gpio);
      if (!ret) {
        /* active low */
        *asserted = gpio == GPIO_VALUE_LOW ? true : false;
      }
    } else {
      ret = sph_prdy_get(fru, &gpio);
      if (!ret) {
        /* active low */
        *asserted = gpio == GPIO_VALUE_LOW ? true : false;
      }
    }

#ifdef FBY2_DEBUG
    if (*asserted)
      syslog(LOG_DEBUG, "%s fru=%d asserted", __FUNCTION__, fru);
#endif

    return ST_OK;
}


int prdy_is_event_triggered(const int fru, bool* triggered)
{
    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return ST_ERR;
    }

    pthread_mutex_lock(&triggered_mutex);
    *triggered = g_gpios_triggered[JTAG_PRDY_EVENT];
    g_gpios_triggered[JTAG_PRDY_EVENT] = false;
    pthread_mutex_unlock(&triggered_mutex);

#ifdef FBY2_DEBUG
    if (*triggered)
      syslog(LOG_DEBUG, "%s fru=%d triggered", __FUNCTION__, fru);
#endif

    return ST_OK;
}


// pready pin = xdp_bic_prdy_n
int prdy_is_asserted(const int fru, bool* asserted)
{
    uint8_t gpio;
    int ret;

    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return ST_ERR;
    }

    ret = bic_get_gpio_status(fru, XDP_BIC_PRDY_N, &gpio);
    if (!ret) {
      /* active low */
      *asserted = gpio == GPIO_VALUE_LOW ? true : false;
    } else {
      syslog(LOG_ERR, "%s: error getting GPIO. fru: %d", __FUNCTION__, fru);
    }

#ifdef FBY2_DEBUG
    if (*asserted)
      syslog(LOG_DEBUG, "%s fru=%d asserted", __FUNCTION__, fru);
#endif

    return ret;
}

int xdp_present_is_event_triggered(const int fru, bool* triggered)
{
    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return ST_ERR;
    }

    pthread_mutex_lock(&triggered_mutex);
    *triggered = g_gpios_triggered[JTAG_XDP_PRESENT_EVENT];
    g_gpios_triggered[JTAG_XDP_PRESENT_EVENT] = false;
    pthread_mutex_unlock(&triggered_mutex);

#ifdef FBY2_DEBUG
    if (*triggered)
      syslog(LOG_DEBUG, "%s fru=%d triggered", __FUNCTION__, fru);
#endif

    return ST_OK;
}

// xdp present pin = XDP_PRSNT_IN_N, xdp_prsnt_in_n
int xdp_present_is_asserted(const int fru, bool* asserted)
{
    uint8_t gpio;
    int ret;

    if ((fru < FRU_SLOT1) || (fru > FRU_SLOT4)) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return ST_ERR;
    }

    ret = bic_get_gpio_status(fru, XDP_PRSNT_IN_N, &gpio);
    if (!ret) {
      /* active low */
      *asserted = gpio == GPIO_VALUE_LOW ? true : false;
    } else {
      syslog(LOG_ERR, "%s: error getting GPIO. fru: %d", __FUNCTION__, fru);
    }

#ifdef FBY2_DEBUG
    if (*asserted)
      syslog(LOG_DEBUG, "%s fru=%d asserted", __FUNCTION__, fru);
#endif

    return ret;
}

static int
set_sph_mux_command(uint8_t slot_id, uint8_t select) {
  int i = 0, ret = 0, retry = 5, bus = 0;
  uint8_t tbuf[256] = {0x18, 0x52, 0x05, 0xD4, 0x05, 0x8D};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 6;
  uint8_t rlen = 0;

  bus = pal_dev_jtag_gpio_to_bus(slot_id);
  tbuf[2] = bus;

  // Read existing FPGA Output pins status.
  while (retry >= 0) {
    ret =  bic_ipmb_wrapper(slot_id, tbuf[0]>>2, tbuf[1], &tbuf[2], tlen-2, rbuf, &rlen);
    if (ret == 0) 
     break;
    retry--;
  }

  for (i = 0; i < rlen; i++) {
    tbuf[6+i] = rbuf[i];
  }
  tlen += (rlen - 1);

  {
    tbuf[8] =  tbuf[8] & ~3;
    if (select == 0) {
      tbuf[8] |= 2;
    }
    else {
      tbuf[8] |= 1;
    }

    retry = 5;
    while (retry >= 0) {
      ret = bic_ipmb_wrapper(slot_id, tbuf[0]>>2, tbuf[1], &tbuf[2], tlen-2, rbuf, &rlen);
      if (ret == 0) {
        break;
      }

      retry--;
    }
    if (ret) {
      syslog(LOG_ERR, "%s: BIC no response!\n", __FUNCTION__);
      return ret;
    }
  }

  return 0;
}

int tck_mux_select_assert(const int fru, bool assert)
{
  if (fby2_get_slot_type(fru) == SLOT_TYPE_GPV2) {
    int status = set_sph_mux_command(fru, assert);
    if (status != 0) {
     syslog(LOG_ERR, "%s fru=%d triggered : failed to pass command", __FUNCTION__, fru);
    }
  }
  return ST_OK;
}
