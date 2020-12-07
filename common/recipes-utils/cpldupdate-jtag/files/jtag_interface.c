/*
 * jtag_interface
 *
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

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include "jtag_interface.h"


int jtag_interface_open(const char * dev_name)
{
    int temp_fd;

    if(dev_name == NULL){
        printf("%s(%d) - %s: JTAG device name is null!\n", \
               __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    temp_fd = open(dev_name, O_RDWR);
    if (temp_fd < 0) {
        printf("%s(%d) - %s: Can't open %s, please install driver!!\n", \
               __FILE__, __LINE__, __FUNCTION__, dev_name);
        return -1;
    }

    return temp_fd;
}

void jtag_interface_close(int jtag_fd)
{
    if(jtag_fd > 0){
        close(jtag_fd);
    }
}

unsigned int jtag_interface_get_freq(int jtag_fd)
{
    int retval;
    unsigned int freq = 0;
    retval = ioctl(jtag_fd, JTAG_GIOCFREQ, &freq);
    if (retval < 0) {
        printf("%s(%d) - %s: ioctl JTAG_GIOCFREQ failure!\n", \
               __FILE__, __LINE__, __FUNCTION__);
        return 0;
    }

    return freq;
}

int jtag_interface_set_freq(int jtag_fd, unsigned int freq)
{
    int retval;
    retval = ioctl(jtag_fd, JTAG_SIOCFREQ, &freq);
    if (retval < 0) {
        printf("%s(%d) - %s: ioctl JTAG_SIOCFREQ failure!\n", \
                       __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    return 0;
}

int jtag_interface_run_test_idle(int jtag_fd, 
                                 unsigned char reset, 
                                 unsigned char endstate, 
                                 unsigned char tck)

{
    int retval;
    struct jtag_end_tap_state run_idle;

    run_idle.endstate = endstate;
    run_idle.reset = reset;
    run_idle.tck = tck;

    retval = ioctl(jtag_fd, JTAG_SIOCSTATE, &run_idle);
    if (retval < 0) {
        printf("%s(%d) - %s: ioctl JTAG_SIOCSTATE failure!\n", \
                       __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    return 0;
}


int jtag_interface_get_status(int jtag_fd, enum jtag_endstate *endstate)
{
    int retval;
    enum jtag_endstate tempstate;

    retval = ioctl(jtag_fd, JTAG_GIOCSTATUS, &tempstate);
    if (retval < 0) {
        printf("%s(%d) - %s: ioctl JTAG_GIOCSTATUS failure!\n", \
                       __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    *endstate = tempstate;
    return 0;
}

int jtag_interface_xfer(int jtag_fd, 
                        unsigned char type,
                        unsigned char direction,
                        unsigned char endstate,
                        unsigned int length,
                        unsigned int tdio)
{
    int retval;
    struct jtag_xfer xfer;

    xfer.type = type;
    xfer.direction = direction;
    xfer.length = length;
    xfer.endstate = endstate;
    xfer.tdio = tdio;

    retval = ioctl(jtag_fd, JTAG_IOCXFER, &xfer);
    if (retval < 0) {
        printf("%s(%d) - %s: ioctl JTAG_IOCXFER failure!\n", \
                       __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    return 0;
}

int jtag_interface_set_xfer_mode(int jtag_fd, unsigned int mode)
{
    int retval;
    struct jtag_mode j_mode = {
        .feature = JTAG_XFER_MODE,
    };

    /* Only support two modes */
    if (mode >= JTAG_XFER_HW_MODE) {
        j_mode.mode = JTAG_XFER_HW_MODE;
    } else {
        j_mode.mode = JTAG_XFER_SW_MODE;
    }

    retval = ioctl(jtag_fd, JTAG_SIOCMODE, &j_mode);
    if (retval < 0) {
        printf("%s(%d) - %s: ioctl JTAG_SIOCMODE failure!\n", \
                       __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    return 0;
}


