#ifndef __AST_JTAG_H__
#define __AST_JTAG_H__

#include <stdint.h>

/*
 * JTAG_XFER_HW_MODE: JTAG hardware mode. Used to set HW drived or bitbang
 * mode. This is bitmask param of ioctl JTAG_SIOCMODE command
 */
#define  JTAG_XFER_SW_MODE 0
#define  JTAG_XFER_HW_MODE 1

/******************************************************************************************************************/
void ast_jtag_set_mode(unsigned int mode);
int ast_jtag_open(void);
void ast_jtag_close(void);
unsigned int ast_get_jtag_freq(void);
int ast_set_jtag_freq(unsigned int freq);
int ast_jtag_run_test_idle(unsigned char reset, unsigned char end, unsigned char tck);
int ast_jtag_sir_xfer(unsigned char endir, unsigned int len, unsigned int tdi);
int ast_jtag_tdo_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio);
int ast_jtag_tdi_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio);

#endif /* __AST_JTAG_H__ */
