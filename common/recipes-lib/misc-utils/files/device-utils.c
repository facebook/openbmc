/*
 * Copyright 2020-present Facebook. All Rights Reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "misc-utils.h"

/*
 * Read integer value from the given device.
 *
 * Return:
 *   On success, zero is returned.
 *   On error, errno is returned.
 */
int device_read(const char *device, int *value) {
    FILE *fp;
    int rc;

    fp = fopen(device, "r");
    if (!fp)
        return errno;

    rc = fscanf(fp, "%i", value);
    fclose(fp);

    if (rc != 1)
        return errno;

    return 0;
}

/*
 * write buffer to the given device.
 *
 * Return:
 *   On success, zero is returned.
 *   On error, errno is returned.
 */
int device_write_buff(const char *device, const char *value) {
  FILE *fp;
  int rc;

    fp = fopen(device, "w");
    if (!fp)
        return errno;

    rc = fputs(value, fp);
    fclose(fp);

    if (rc < 0)
        return errno;

    return 0;
}
