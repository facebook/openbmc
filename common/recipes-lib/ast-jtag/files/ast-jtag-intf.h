#ifndef _AST_JTAG_INTF_H_
#define _AST_JTAG_INTF_H_

struct jtag_ops {
  int (*open)();
  void (*close)();
  void (*set_mode)(unsigned int);
  unsigned int (*get_freq)();
  int (*set_freq)(unsigned int);
  int (*run_test_idle)(unsigned char,unsigned char, unsigned char);
  int (*sir_xfer)(unsigned char,unsigned int, unsigned int);
  int (*tdo_xfer)(unsigned char, unsigned int, unsigned int*);
  int (*tdi_xfer)(unsigned char, unsigned int, unsigned int*);
};

#endif
