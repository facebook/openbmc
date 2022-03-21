#include "obmc-ras.h"
#include <assert.h>
#include <pthread.h>
#include <syslog.h>

struct delayed_log {
  useconds_t usec;
  char msg[1024];
};

// Thread for delay event
static void *
delay_log(void *arg)
{
  struct delayed_log* log = (struct delayed_log*)arg;

  pthread_detach(pthread_self());

  if (arg) {
    usleep(log->usec);
    syslog(LOG_CRIT, "%s", log->msg);

    free(arg);
  }

  pthread_exit(NULL);
}

void __attribute__((weak))
log_gpio_change(gpiopoll_pin_t *gp, gpio_value_t value, useconds_t log_delay, bool active_low)
{
  const struct gpiopoll_config *cfg = gpio_poll_get_config(gp);
  assert(cfg);
  if (log_delay == 0) {
    if (active_low)
      syslog(LOG_CRIT, "%s: %s - %s\n", value ? "DEASSERT": "ASSERT", cfg->description, cfg->shadow);
    else
      syslog(LOG_CRIT, "%s: %s - %s\n", value ? "ASSERT": "DEASSERT", cfg->description, cfg->shadow);
  } else {
    pthread_t tid_delay_log;
    struct delayed_log *log = (struct delayed_log *)malloc(sizeof(struct delayed_log));
    if (log) {
      log->usec = log_delay;
      snprintf(log->msg, 1024, "%s: %s - %s\n", value ? "DEASSERT" : "ASSERT", cfg->description, cfg->shadow);
      if (pthread_create(&tid_delay_log, NULL, delay_log, (void *)log)) {
        free(log);
        log = NULL;
      }
    }
    if (!log) {
      syslog(LOG_CRIT, "%s: %s - %s\n", value ? "DEASSERT": "ASSERT", cfg->description, cfg->shadow);
    }
  }

}

void __attribute__((weak))
fast_prochot_hndlr(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr) {
  log_gpio_change(gp, curr, 0, true);
  
}

void __attribute__((weak))
fast_prochot_init(gpiopoll_pin_t *gp, gpio_value_t value)
{
  if (value == GPIO_VALUE_LOW)
    log_gpio_change(gp, value, 0, true);
}
