/*
 * Copyright 2017-present Facebook. All Rights Reserved.
 *
 * This file contains code to provide addendum functionality over the I2C
 * device interfaces to utilize additional driver functionality.
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <linux/limits.h>

#include "i2c_core.h"
#include "i2c_sysfs.h"

char* i2c_sysfs_suid_gen(char *uid, size_t size,
			 int bus, uint16_t addr)
{
	if (uid != NULL && size > 0)
		snprintf(uid, size, "%d-%04x", bus, addr);

	return uid;
}

int i2c_sysfs_suid_parse(const char *slave_uid,
			 int *i2c_bus, uint16_t *i2c_addr)
{
	int bus;
	uint16_t addr;
	char *endptr;

	if (slave_uid == NULL || slave_uid[0] == '\0')
		goto error;

	/* parse bus field */
	bus = (int)strtol(slave_uid, &endptr, 10);
	if (bus < 0 || endptr == slave_uid || *endptr != '-')
		goto error;

	/* parse address field */
	endptr++;
	if (strlen(endptr) != 4)
		goto error;
	addr = (uint16_t)strtol(endptr, NULL, 16);
	if (!i2c_addr_is_valid(addr))
		goto error;

	if (i2c_bus != NULL)
		*i2c_bus = bus;
	if (i2c_addr != NULL)
		*i2c_addr = addr;
	return 0;

error:
	errno = EINVAL;
	return -1;
}

bool i2c_sysfs_is_valid_suid(const char* suid)
{
	return (i2c_sysfs_suid_parse(suid, NULL, NULL) == 0);
}

char* i2c_sysfs_slave_abspath(char *path, size_t size, int bus, uint16_t addr)
{
	if (path != NULL && size > 0) {
		char suid[NAME_MAX];
		snprintf(path, size, "%s/%s",
			 I2C_SYSFS_DEVICES,
			 i2c_sysfs_suid_gen(suid, sizeof(suid), bus, addr));
	}

	return path;
}

#ifdef _OBMC_I2C_UNITTEST_
int main(int argc, char **argv)
{
	uint16_t addr;
	int i, rc, bus, errors = 0;
	struct suid_parse_test {
		const char *suid;
		int exp_rc;
		int exp_bus;
		uint16_t exp_addr;
	} suid_parse_tests[] = {
		{"0-0031", 0, 0, 0x31},
		{"130-0073", 0, 130, 0x73},
		{"8-00ff", -1, 0, 0},
		{"1234", -1, 0, 0},
		{NULL, -1, 0, 0},
	};
	struct sysfs_path_test {
		int bus;
		uint16_t addr;
		const char *exp_suid;
		const char *exp_dir;
	} sysfs_path_tests[] = {
		{2, 0x71, "2-0071", I2C_SYSFS_DEVICES "/2-0071"},
		{213, 0x30, "213-0030", I2C_SYSFS_DEVICES "/213-0030"},
		{0, 0x1010, "0-1010", I2C_SYSFS_DEVICES "/0-1010"},
		{-1, 0, NULL, NULL},
	};

	/*
	 * Test if sysfs slave uid can be parsed properly.
	 */
	for (i = 0; suid_parse_tests[i].suid != NULL; i++) {
		struct suid_parse_test *t = &suid_parse_tests[i];

		printf("parsing suid %s..\n", t->suid);
		rc = i2c_sysfs_suid_parse(t->suid, &bus, &addr);
		if (rc != t->exp_rc) {
			printf("parsing %s failed: exp_rc %d, actual %d\n",
			       t->suid, t->exp_rc, rc);
			errors++;
			continue;
		}
		if (i2c_sysfs_is_valid_suid(t->suid) != (rc == 0)) {
			printf("validating %s failed: expect <%s>\n",
			       t->suid, (rc == 0) ? "valid" : "invalid");
			errors++;
			continue;
		}
		if ((rc == 0) &&
		    (bus != t->exp_bus || addr != t->exp_addr)) {
			printf("parsing %s failed: bus/addr mismatch\n",
			       t->suid);
			errors++;
			continue;
		}
	}

	/*
	 * Test if sysfs slave uid and abspath can be generated correctly.
	 */
	for (i = 0; sysfs_path_tests[i].bus >= 0; i++) {
		char buf[PATH_MAX];
		struct sysfs_path_test *t = &sysfs_path_tests[i];

		printf("generating suid for slave (bus=%d, addr=%#x)..\n",
		       t->bus, t->addr);
		i2c_sysfs_suid_gen(buf, sizeof(buf), t->bus, t->addr);
		if (strcmp(buf, t->exp_suid) != 0) {
			printf("got invalid suid: expect <%s>, actual <%s>\n",
			       t->exp_suid, buf);
			errors++;
			continue;
		}

		printf("generating abspath for slave (bus=%d, addr=%#x)..\n",
		       t->bus, t->addr);
		i2c_sysfs_slave_abspath(buf, sizeof(buf), t->bus, t->addr);
		if (strcmp(buf, t->exp_dir) != 0) {
			printf("got invalid abspath: expect <%s>, actual <%s>\n",
			       t->exp_dir, buf);
			errors++;
			continue;
		}
	}

	if (errors != 0) {
		printf("Test failed: total %d errors found!\n", errors);
		return -1;
	}

	printf("Test succeeded!\n");
	return 0;
}
#endif /* _OBMC_I2C_UNITTEST_ */
