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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "phymem.h"


void * phymem_open(off_t base, size_t length, size_t *mapped_size)
{
	int fd = 0;
	void *map_base = MAP_FAILED;
	size_t page_size = 0;
	off_t page_offset = 0;

	page_size = getpagesize();

	page_offset = base & (page_size - 1);
	if (page_offset > 0) {
		length += page_offset;
	}
	*mapped_size = length;

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
		syslog(LOG_ERR, "fail to open \"/dev/mem\"");
		goto end;
	}

	map_base = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, base & ~(off_t)(page_size - 1));

	if (map_base == MAP_FAILED) {
		syslog(LOG_ERR, "mmap failed");
	}
	close(fd);
end:
	return map_base;
}

int phymem_close(void *map_base, size_t length)
{
	int ret = 0;
	ret = munmap(map_base, length);
	if (ret == -1) {
		syslog(LOG_ERR, "munmap failed");
	}
	return ret;
}

int phymem_read(off_t addr, size_t offset, memLength length, void *value)
{
	void *vir_addr = NULL, *map_base = NULL;
	size_t mapped_size = 0, page_size = 0;
	int ret = 0;

	page_size = getpagesize();
	map_base = phymem_open(addr, page_size, &mapped_size);
	if (map_base == MAP_FAILED) {
		return -1;
	}
	vir_addr = (char*)map_base + offset;

	switch (length) {
	case M_BYTE:
		*(uint8_t *)value = *(volatile uint8_t*)vir_addr;
		break;
	case M_WORD:
		*(uint16_t *)value = *(volatile uint16_t*)vir_addr;
		break;
	case M_DWORD:
		*(uint32_t *)value = *(volatile uint32_t*)vir_addr;
		break;
	default:
		*(uint32_t *)value = *(volatile uint32_t*)vir_addr;
		break;
	}
	ret = phymem_close(map_base, mapped_size);
	return ret;
}

int phymem_write(off_t addr, size_t offset, memLength length, uint32_t value)
{
	void *vir_addr = NULL, *map_base = NULL;
	size_t mapped_size = 0, page_size = 0;
	int ret = 0;

	page_size = getpagesize();
	map_base = phymem_open(addr, page_size, &mapped_size);
	if (map_base == MAP_FAILED) {
                return -1;
        }
	vir_addr = (char*)map_base + offset;

	switch (length) {
	case M_BYTE:
		*(volatile uint8_t*)vir_addr = (uint8_t)value;
		break;
	case M_WORD:
		*(volatile uint16_t*)vir_addr = (uint16_t)value;
		break;
	case M_DWORD:
		*(volatile uint32_t*)vir_addr = (uint32_t)value;
		break;
	default:
		*(volatile uint32_t*)vir_addr = (uint32_t)value;
		break;
	}
	ret = phymem_close(map_base, mapped_size);
	return ret;
}

int phymem_get_byte(off_t addr, size_t offset, uint8_t *value)
{
	return phymem_read(addr, offset, M_BYTE, value);
}

int phymem_get_word(off_t addr, size_t offset, uint16_t *value)
{
	return phymem_read(addr, offset, M_WORD, value);
}

int phymem_get_dword(off_t addr, size_t offset, uint32_t *value)
{
	return phymem_read(addr, offset, M_DWORD, value);
}
/*
 * ASPEED BMC only support 4 bytes align writing
 * Reserve for future use
 */
#if 0
int phymem_set_byte(off_t addr, size_t offset, uint8_t value)
{
	return phymem_write(addr, offset, M_BYTE, value);
}

int phymem_set_word(off_t addr, size_t offset, uint16_t value)
{
	return phymem_write(addr, offset, M_WORD, value);
}
#endif
int phymem_set_dword(off_t addr, size_t offset, uint32_t value)
{
	return phymem_write(addr, offset, M_DWORD, value);
}

int phymem_dword_set_bit(off_t base, size_t offset, uint8_t bit) {
	int ret = 0;
	uint32_t value = 0;

	ret = phymem_get_dword(base, offset, &value);
	if (ret < 0) {
		return -1;
	}
	value |= (0x1 << bit);
	ret = phymem_set_dword(base, offset, value);

	return ret;
}

int phymem_dword_clear_bit(off_t base, size_t offset, uint8_t bit) {
	int ret = 0;
	uint32_t value = 0;

	ret = phymem_get_dword(base, offset, &value);
	if (ret < 0) {
		return -1;
	}
	value &= ~(0x1 << bit);
	ret = phymem_set_dword(base, offset, value);

	return ret;
}
