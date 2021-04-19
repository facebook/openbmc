#ifndef __MPSSE_H__
#define __MPSSE_H__

// Jtag Start
int tap_tms(struct ftdi_context *ftdi, int tms, uint8_t bit7);
int tap_readwrite(struct ftdi_context *ftdi, int write_bits, uint8_t *in, int read_bits, uint8_t *out, uint8_t last_trans);
int mpsse_jtag_write(struct ftdi_context *ftdi, int num_of_bits, uint8_t *in, uint8_t last_trans);
int mpsse_jtag_read(struct ftdi_context *ftdi, int num_of_bits, uint8_t *out, uint8_t last_trans);
int tap_tms_byte(struct ftdi_context *ftdi, int clk, int tms_seq, uint8_t bit7);
int tap_reset_to_rti(struct ftdi_context *ftdi);
// Jtag end

// mpsse start
int find_devlist(struct ftdi_context *ftdi);
int init_dev(struct ftdi_context *ftdi);
int mpsse_trigger_trst(struct ftdi_context *ftdi);
int mpsse_init_conf(struct ftdi_context *ftdi, uint16_t tck);
// mpsse end
#endif
