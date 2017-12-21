/*
 * Microsemi Switchtec(tm) PCIe Management Command Line Interface
 * Copyright (c) 2017, Microsemi Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include "switchtec_priv.h"

#include "switchtec/switchtec.h"
#include "switchtec/mrpc.h"
#include "switchtec/errors.h"
#include "switchtec/log.h"
#include "switchtec/gas.h"

#include <linux/switchtec_ioctl.h>

#include <fcntl.h>
#include <unistd.h>
#include <endian.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <glob.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

int switchtec_inband_gas_read(struct switchtec_dev *dev, uint32_t offset,
			 void *data, size_t data_len) 
{
	void *map;

    map = switchtec_gas_map(dev, 0, NULL);
    if (map == MAP_FAILED) {
        switchtec_perror("gas_map");
        return -1;
    }

    memcpy(data, map + offset, data_len);

    switchtec_gas_unmap(dev, map);

    return 0;
}

int switchtec_inband_gas_write(struct switchtec_dev *dev, uint32_t offset,
			 const void *data, size_t data_len) 
{
	void *map;

    map = switchtec_gas_map(dev, 1, NULL);
    if (map == MAP_FAILED) {
        switchtec_perror("gas_map");
        return -1;
    }

    memcpy(map + offset, (void*)data, data_len);

    switchtec_gas_unmap(dev, map);

    return 0;
}

int switchtec_twi_gas_get_result(struct switchtec_dev *dev, uint32_t *result)
{
    uint32_t offset = 0xFFFFFFFF;
    struct i2c_rdwr_ioctl_data ioctl_data = {0};
    struct i2c_msg msg[3] = {{0}};
    int ret;

    msg[0].addr = dev->twi_slave;
    msg[0].flags = 0; /* write */
    msg[0].len = 4;
    msg[0].buf = (uint8_t*)&offset;
    msg[1].addr = dev->twi_slave;
    msg[1].flags = I2C_M_RD;
    msg[1].len = 4;
    msg[1].buf = (uint8_t *)result;
    ioctl_data.nmsgs = 2;
    ioctl_data.msgs = &msg[0];

    ret = ioctl(dev->fd, I2C_RDWR, &ioctl_data); 
    if (ret < 0) {
        perror("I2C_RDWR");
        return ret;
    }

    return 0;
}

int switchtec_twi_gas_read_1024(struct switchtec_dev *dev, uint32_t offset,
			 void *data, size_t data_len) 
{
    struct i2c_rdwr_ioctl_data ioctl_data = {0};
    struct i2c_msg msg[3] = {{0}};
    uint32_t be_offset;
    int ret;
    uint32_t result;
    int i;
    uint32_t p;

    if (offset % 4) 
        fprintf(stderr, "invalid access width\n");

    if (!data_len || (data_len % 4) || (data_len > 1024))
        fprintf(stderr, "invalid data length\n");

    be_offset = htobe32(offset);

    msg[0].addr = dev->twi_slave;
    msg[0].flags = 0; /* write */
    msg[0].len = 4;
    msg[0].buf = (uint8_t*)&be_offset;
    msg[1].addr = dev->twi_slave;
    msg[1].flags = I2C_M_RD;
    msg[1].len = data_len;
    msg[1].buf = data;
    ioctl_data.nmsgs = 2;
    ioctl_data.msgs = &msg[0];

    ret = ioctl(dev->fd, I2C_RDWR, &ioctl_data); 
    if (ret < 0) {
        perror("I2C_RDWR");
        return ret;
    }

    ret = switchtec_twi_gas_get_result(dev, &result);
    if (ret) 
        return ret;

    for (i = 0; i < data_len / 4; i++) {
        memcpy((void *)&p, data, 4);
        p = htobe32(p); 
        memcpy(data, (void *)&p, 4);
        data += 4;
    }

    return result;
}

int switchtec_twi_gas_write_28(struct switchtec_dev *dev, uint32_t offset,
			 const void *data, size_t data_len) 
{
    struct i2c_rdwr_ioctl_data ioctl_data = {0};
    struct i2c_msg msg[2] = {{0}};;
    uint8_t buf[data_len + sizeof(offset)];
    int ret;
    uint32_t be_offset;
    uint32_t result;
    int i;
    uint32_t p;

    if (offset % 4) 
        fprintf(stderr, "invalid access width\n");

    if (!data_len || (data_len % 4) || (data_len > 28))
        fprintf(stderr, "invalid data length\n");

    be_offset = htobe32(offset);

    memset(buf, 0, sizeof(buf));
    memcpy((void *)(buf), (void *)&be_offset, sizeof(be_offset));

    for (i = 0; i < data_len / 4; i++) {
        memcpy((void *)&p, data, 4);
        p = htobe32(p); 
        memcpy((void *)&(buf[4 + i*4]), (void *)&p, 4);
        data += 4;
    }

    msg[0].addr = dev->twi_slave;
    msg[0].flags = 0; /* write */
    msg[0].len = sizeof(buf);
    msg[0].buf = buf;
    ioctl_data.nmsgs = 1;
    ioctl_data.msgs = &msg[0];

    ret = ioctl(dev->fd, I2C_RDWR, &ioctl_data);
    if (ret < 0) {
        perror("I2C_RDWR");
        return ret;
    }

    ret = switchtec_twi_gas_get_result(dev, &result);
    if (ret) 
        return ret;

    return result;
}

int switchtec_twi_gas_read(struct switchtec_dev *dev, uint32_t offset,
			 void *data, size_t data_len) 
{
    uint32_t start, end;
    size_t new_size;
    void *buf;
    int ret;

    if (!data_len) 
        return 0; 

    start = offset & ~(4 - 1);
    end = (offset + data_len + 4 - 1) & ~(4 - 1); 
    new_size = end - start;

    if (new_size > 1024) {
		fprintf(stderr, "Too much data to read: %lu.\n", new_size);
        return -1;
    }

    if (new_size != data_len) {
        buf = malloc(new_size);
        if (buf == NULL) 
            return -1;
    } else {
        buf = (void *)data;
    }

    ret = switchtec_twi_gas_read_1024(dev, start, buf, new_size);
    if (ret) 
        goto free_ret;

    memcpy(data, (buf + offset % 4), data_len);

free_ret:
    if (buf != data) 
        free(buf);

    return ret;
}

int switchtec_twi_gas_write(struct switchtec_dev *dev, uint32_t offset,
			 const void *data, size_t data_len) 
{
    int ret;
    uint32_t start, end;
    size_t new_size;
    void *buf;
    int pos;

    if (data_len > 1024) {
		fprintf(stderr, "Too much data to write: %lu.\n", data_len);
        return -1;
    }

    start = offset & ~(4 - 1);
    end = (offset + data_len + 4 - 1) & ~(4 - 1); 
    new_size = end - start;

    if (new_size != data_len) {
        buf = malloc(new_size);
        if (buf == NULL) 
            return -1;
        ret = switchtec_twi_gas_read(dev, start, buf, new_size); 
        if (ret) 
            goto free_ret;
        
        memcpy((buf + offset % 4), data, data_len);
    } else {
        buf = (void *)data;
    }

    offset = start;
    pos = 0;
    while (new_size) {
        if (new_size <= 28) {
            ret = switchtec_twi_gas_write_28(dev, offset, buf + pos, new_size); 
            goto free_ret;
        } else {
            ret = switchtec_twi_gas_write_28(dev, offset, buf + pos, 28); 
            if (ret) 
                goto free_ret;

            offset += 28;
            new_size -= 28;
            pos += 28;
        }
    }

free_ret:
    if (buf != data) 
        free(buf);
    
    return ret;
}

