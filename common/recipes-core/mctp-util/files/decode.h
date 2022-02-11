/*
 * decode.h
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#ifndef _DECODE_H_
#define _DECODE_H_

#define MIN_MCTP_CTRL_RESP_LEN 3  //Byte1: IID, Byte2:Cmd, Byte3:Completion Code
#define MIN_PLDM_RESP_LEN 4  //Byte1: IID, Byte2: Type, Byte3:Cmd, Byte4:Completion Code

void print_raw_resp(uint8_t *rbuf, int rlen);
int  print_parsed_resp(uint8_t *rbuf, int rlen);
#endif /* _DECODE_H_ */
