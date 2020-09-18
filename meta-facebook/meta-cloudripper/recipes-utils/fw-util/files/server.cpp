/*
 * fw-util
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

#include <iostream>
#include <cstdio>
#include <cstring>
#include <openbmc/pal.h>
#include "server.h"

using namespace std;

void Server::ready()
{
  int ret;
  uint8_t status;
  ret = pal_is_fru_prsnt(FRU_SCM, &status);
  if (ret < 0) {
  #ifdef DEBUG
    cerr << "pal_is_fru_prsnt failed for fru " << FRU_SCM << endl;
  #endif
    throw "get " + fru_name + " present failed";
  }
  if (status == 0) {
#ifdef DEBUG
    cerr << "slot" << _slot_id << " is empty!" << endl;
#endif
    throw fru_name + " is empty";
  }
  ret = pal_get_server_power(_slot_id, &status);
  if (ret < 0) {
#ifdef DEBUG
    cerr << "Failed to get server power status. Stopping update" << endl;
#endif
    throw "get " + fru_name + " power status failed";
  }
  if (status == SERVER_12V_OFF || status == SERVER_POWER_OFF) {
#ifdef DEBUG
    cerr << "Can't update FW version since the server is 12V off!" << endl;
#endif
    throw fru_name + " power 12V-off";
  }
  if (pal_is_slot_support_update(_slot_id)) {
#ifdef DEBUG
    cerr << "slot" << _slot_id << " is not a server" << endl;
#endif
    throw "fru " + fru_name + " is not a server";
  }
}
