/*
 * brcm-ncsi-util.h
 *
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
#ifndef _BRCM_NCSI_UTIL_H_
#define _BRCM_NCSI_UTIL_H_

#include "ncsi-util.h"

int brcm_ncsi_util_handler(int argc, char **argv, ncsi_util_args_t *util_args);

#endif /* _BRCM_NCSI_UTIL_H_ */
