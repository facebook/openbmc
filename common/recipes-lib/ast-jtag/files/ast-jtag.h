#ifndef __AST_JTAG_H__
#define __AST_JTAG_H__

#include <stdint.h>

/*
 * JTAG_XFER_HW_MODE: JTAG hardware mode. Used to set HW drived or bitbang
 * mode. This is bitmask param of ioctl JTAG_SIOCMODE command
 */
#define  JTAG_XFER_HW_MODE 1

/**
 * enum jtag_endstate:
 *
 * @JTAG_STATE_IDLE: JTAG state machine IDLE state
 * @JTAG_STATE_PAUSEIR: JTAG state machine PAUSE_IR state
 * @JTAG_STATE_PAUSEDR: JTAG state machine PAUSE_DR state
 */
enum jtag_endstate {
    JTAG_STATE_IDLE,
    JTAG_STATE_PAUSEIR,
    JTAG_STATE_PAUSEDR,
};

/**
 * enum jtag_xfer_type:
 *
 * @JTAG_SIR_XFER: SIR transfer
 * @JTAG_SDR_XFER: SDR transfer
 */
enum jtag_xfer_type {
    JTAG_SIR_XFER,
    JTAG_SDR_XFER,
};

/**
 * enum jtag_xfer_direction:
 *
 * @JTAG_READ_XFER: read transfer
 * @JTAG_WRITE_XFER: write transfer
 */
enum jtag_xfer_direction {
    JTAG_READ_XFER,
    JTAG_WRITE_XFER,
};

/**
 * struct jtag_run_test_idle - forces JTAG state machine to
 * RUN_TEST/IDLE state
 *
 * @reset: 0 - run IDLE/PAUSE from current state
 *         1 - go through TEST_LOGIC/RESET state before  IDLE/PAUSE
 * @end: completion flag
 * @tck: clock counter
 *
 * Structure represents interface to JTAG device for jtag idle
 * execution.
 */
struct jtag_run_test_idle {
    uint8_t    reset;
    uint8_t    endstate;
    uint8_t    tck;
};

/**
 * struct jtag_xfer - jtag xfer:
 *
 * @type: transfer type
 * @direction: xfer direction
 * @length: xfer bits len
 * @tdio : xfer data array
 * @endir: xfer end state
 *
 * Structure represents interface to JTAG device for jtag sdr xfer
 * execution.
 */
struct jtag_xfer {
    uint8_t    type;
    uint8_t    direction;
    uint8_t    endstate;
    uint32_t   length;
    uint32_t   *tdio;
};

/* ioctl interface */
#define __JTAG_IOCTL_MAGIC  0xb2

#define JTAG_IOCRUNTEST _IOW(__JTAG_IOCTL_MAGIC, 0,\
                 struct jtag_run_test_idle)
#define JTAG_SIOCFREQ   _IOW(__JTAG_IOCTL_MAGIC, 1, unsigned int)
#define JTAG_GIOCFREQ   _IOR(__JTAG_IOCTL_MAGIC, 2, unsigned int)
#define JTAG_IOCXFER    _IOWR(__JTAG_IOCTL_MAGIC, 3, struct jtag_xfer)
#define JTAG_GIOCSTATUS _IOWR(__JTAG_IOCTL_MAGIC, 4, enum jtag_endstate)
#define JTAG_SIOCMODE   _IOW(__JTAG_IOCTL_MAGIC, 5, unsigned int)

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
