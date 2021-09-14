#include <unistd.h>

#include "ast-jtag-intf.h"
#include <openbmc/misc-utils.h>

extern struct jtag_ops jtag0_ops;
extern struct jtag_ops astjtag_ops;

static struct jtag_ops *jtag_ops = &jtag0_ops;
static int lock_fd = -1;


__attribute__((constructor))
void __attribute__((constructor)) ast_jtag_init(void)
{
  if (access("/dev/jtag0", F_OK) == 0) {
    jtag_ops = &jtag0_ops;
  } else {
    jtag_ops = &astjtag_ops;
  }
}

void ast_jtag_set_mode(unsigned int mode)
{
  jtag_ops->set_mode(mode);
}

int ast_jtag_open(void)
{
  if (lock_fd != -1) {
    return -1;
  }
  if ((lock_fd = single_instance_lock("ast-jtag")) < 0) {
    return -1;
  }
  int ret = jtag_ops->open();
  if (ret) {
    single_instance_unlock(lock_fd);
    lock_fd = -1;
    return ret;
  }
  return ret;
}

void ast_jtag_close(void)
{
  jtag_ops->close();
  single_instance_unlock(lock_fd);
  lock_fd = -1;
}

unsigned int ast_get_jtag_freq(void)
{
  return jtag_ops->get_freq();
}

int ast_set_jtag_freq(unsigned int freq)
{
  return jtag_ops->set_freq(freq);
}

int ast_jtag_run_test_idle(unsigned char reset, unsigned char end, unsigned char tck)
{
  return jtag_ops->run_test_idle(reset, end, tck);
}

int ast_jtag_sir_xfer(unsigned char endir, unsigned int len, unsigned int tdi)
{
  return jtag_ops->sir_xfer(endir, len, tdi);
}

int ast_jtag_tdi_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio)
{
  return jtag_ops->tdi_xfer(enddr, len, tdio);
}

int ast_jtag_tdo_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio)
{
  return jtag_ops->tdo_xfer(enddr, len, tdio);
}
