/*
 * Copyright 2019-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __JTAG_INTERFACE_H__
#define __JTAG_INTERFACE_H__

#include <linux/types.h>
/*
 * JTAG_XFER_HW_MODE: JTAG hardware mode. Used to set HW drived or bitbang
 * mode. This is bitmask param of ioctl JTAG_SIOCMODE command
 */
#define JTAG_XFER_HW_MODE 1
#define JTAG_XFER_SW_MODE 0
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
typedef struct {
    __u8    reset;
    __u8    endstate;
    __u8    tck;
}jtag_run_test_idle_t;

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
    __u8    type;
    __u8    direction;
    __u8    endstate;
    __u32   length;
    __u64   tdio;
};


#define __JTAG_IOCTL_MAGIC  0xb2

#define JTAG_IOCRUNTEST _IOW(__JTAG_IOCTL_MAGIC, 0, jtag_run_test_idle_t)
#define JTAG_SIOCFREQ   _IOW(__JTAG_IOCTL_MAGIC, 1, unsigned int)
#define JTAG_GIOCFREQ   _IOR(__JTAG_IOCTL_MAGIC, 2, unsigned int)
#define JTAG_IOCXFER    _IOWR(__JTAG_IOCTL_MAGIC, 3, struct jtag_xfer)
#define JTAG_GIOCSTATUS _IOWR(__JTAG_IOCTL_MAGIC, 4, enum jtag_endstate)
#define JTAG_SIOCMODE   _IOW(__JTAG_IOCTL_MAGIC, 5, unsigned int)

#ifdef __cplusplus
extern "C" {
#endif

int jtag_interface_open(const char *dev_name);
void jtag_interface_close(int jtag_fd);
int jtag_interface_set_freq(int jtag_fd, unsigned int freq);
unsigned int jtag_interface_get_freq(int jtag_fd);

int jtag_interface_run_test_idle(int jtag_fd, 
                                 unsigned char reset, 
                                 unsigned char endstate, 
                                 unsigned char tck);

int jtag_interface_set_mode(int jtag_fd, unsigned int mode);
int jtag_interface_get_status(int jtag_fd, enum jtag_endstate *endstate);

int jtag_interface_xfer(int jtag_fd, 
                        unsigned char type,
                        unsigned char direction,
                        unsigned char endstate,
                        unsigned int length,
                        unsigned long long tdio);



#ifdef __cplusplus
}
#endif

#endif /* __JTAG_INTERFACE_H__ */
