/*
 * jtag_common
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
#include <stdlib.h>
#include "jtag_interface.h"
#include "jtag_common.h"

/* 
 * Function Description:
 * Create and initialize the JTAG interface
 *
 * Parameter Description:
 * dev_name : JTAG device name
 * mode : the JTAG working mode, hardware mode or software mode
 * freq : the JTAG working frequent, If freq == 0, then don't set the frequent.
 *
 * Return:
 * the JTAG object handle
 */
jtag_object_t *jtag_init(const char *dev_name, unsigned char mode, unsigned int freq)
{
    jtag_object_t *pobj;
    int retcode;
    if(dev_name == NULL){
        printf("%s(%d) - %s: JTAG interface name is null!\n", \
                       __FILE__, __LINE__, __FUNCTION__);
        return NULL;
    }

    pobj = (jtag_object_t *)malloc(sizeof(jtag_object_t));
    if(pobj == NULL){
        printf("%s(%d) - %s: faild to allocate memory!\n", \
                       __FILE__, __LINE__, __FUNCTION__);
        return NULL;
    }

    retcode = jtag_interface_open(dev_name);
    if(retcode < 0){
        printf("%s(%d) - %s: faild to open jtag interface %s!\n", \
                       __FILE__, __LINE__, __FUNCTION__, dev_name);
        goto end_of_func;
    }
 
    pobj->fd = retcode;

    retcode = jtag_interface_set_xfer_mode(pobj->fd, mode);
    if(retcode < 0){
        printf("%s(%d) - %s: faild to set mode %d!\n", \
                       __FILE__, __LINE__, __FUNCTION__, mode);
        goto end_of_func;

    }

    pobj->mode = mode;

    if(freq > 0){
        retcode = jtag_interface_set_freq(pobj->fd, freq);
        if(retcode < 0){
            printf("%s(%d) - %s: faild to set frequent %d!\n", \
                           __FILE__, __LINE__, __FUNCTION__, freq);
            goto end_of_func;
        }
        pobj->freq = freq;
    }else{
        retcode = jtag_interface_get_freq(pobj->fd);
        if(retcode == 0){
            printf("%s(%d) - %s: faild to get frequent, fd = %d!\n", \
                           __FILE__, __LINE__, __FUNCTION__, pobj->fd);
            goto end_of_func;

        }
        pobj->freq = retcode;  
    }

    printf("JTAG object fd = %d, mode = %d, freq = %d\n", \
        pobj->fd, pobj->mode, pobj->freq);

    return pobj;

end_of_func:
    if(pobj){
        free(pobj);
    }
    return  NULL;
}

void jtag_uninit(jtag_object_t *pobj)
{
    if(pobj){
        jtag_interface_close(pobj->fd);
        free(pobj);
    }
}

/* 
 * Function Description:
 * SIR Write operation
 *
 * Parameter Description:
 * pobj: JTAG object handle
 * endir: xfer end state after transaction finish
 * buf : data buffer to be written
 * length : data len in bits
 *
 * Return:
 * 0 - success, < 0 - failure
 */
int jtag_write_instruction_register(jtag_object_t *pobj, 
                                    unsigned int endstate,
                                    unsigned int *buf,
                                    int length)
{
    int retcode;
    unsigned int tdio;
    
    if(pobj == NULL){
        printf("%s(%d) - %s: JTAG object is null!\n", \
               __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    if(buf == NULL){
        printf("%s(%d) - %s: JTAG instruction buffer is null!\n", \
                       __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    tdio = (unsigned int)buf;

    retcode = jtag_interface_xfer(pobj->fd,
              JTAG_SIR_XFER,
              JTAG_WRITE_XFER,
              endstate,
              length,
              tdio);
    if(retcode < 0){
        printf("%s(%d) - %s: jtag_interface_xfer failure!\n", \
               __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }
    
    return 0;
}

/* 
 * Function Description:
 * SDR Write operation
 *
 * Parameter Description:
 * pobj: JTAG object handle
 * endir: xfer end state after transaction finish
 * buf : data buffer to be written
 * length : data len in bits
 *
 * Return:
 * 0 - success, < 0 - failure
 */
int jtag_write_data_register(jtag_object_t *pobj, 
                             unsigned int endstate,
                             unsigned int *buf,
                             int length) 
{
    int retcode;
    unsigned int tdio;

    if(pobj == NULL){
        printf("%s(%d) - %s: JTAG object is null!\n", \
               __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    if(buf == NULL){
        printf("%s(%d) - %s: JTAG data buffer is null!\n", \
                       __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }
    
    tdio = (unsigned int)buf;

    retcode = jtag_interface_xfer(pobj->fd,
              JTAG_SDR_XFER,
              JTAG_WRITE_XFER,
              endstate,
              length,
              tdio);
    if(retcode < 0){
        printf("%s(%d) - %s: jtag_interface_xfer failure!\n", \
               __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    return 0;
}

/* 
 * Function Description:
 * SDR Read operation
 *
 * Parameter Description:
 * pobj: JTAG object handle
 * endir: xfer end state after transaction finish
 * buf : data buffer used for saving read out data
 * length : data len in bits
 *
 * Return:
 * 0 - success, < 0 - failure
 */

int jtag_read_data_register(jtag_object_t *pobj, 
                            unsigned int endstate,
                            unsigned int *buf,
                            int length) 
{
    int retcode;
    unsigned int tdio;
    
    if(pobj == NULL){
        printf("%s(%d) - %s: JTAG object is null!\n", \
               __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    if(buf == NULL){
        printf("%s(%d) - %s: JTAG data buffer is null!\n", \
                       __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    tdio = (unsigned int)buf;
    retcode = jtag_interface_xfer(pobj->fd,
              JTAG_SDR_XFER,
              JTAG_READ_XFER,
              endstate,
              length,
              tdio);
    if(retcode < 0){
        printf("%s(%d) - %s: jtag_interface_xfer failure!\n", \
               __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    return 0;
}

int jtag_get_status(jtag_object_t *pobj,
                    enum jtag_endstate *endstate)
{
    if(pobj == NULL){
        printf("%s(%d) - %s: JTAG object is null!\n", \
               __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    if(endstate == NULL){
        printf("%s(%d) - %s: JTAG status is null!\n", \
               __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    return jtag_interface_get_status(pobj->fd, endstate);
}


int jtag_run_test_idle(jtag_object_t *pobj, 
                       unsigned char reset,
                       unsigned char endstate,
                       unsigned char tck)
{
    if(pobj == NULL){
        printf("%s(%d) - %s: JTAG object is null!\n", \
               __FILE__, __LINE__, __FUNCTION__);
        return -1;
    }

    return jtag_interface_run_test_idle(pobj->fd, reset, endstate, tck);
}
