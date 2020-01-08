/*
 * fw-util
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

#include "bic_fw.h"
#include "bic_me.h"
#include "bic_cpld.h"
#include "bic_bios.h"
#include <facebook/bic.h>

using namespace std;

const int slot_id = 0;
// Register BIC FW and BL components
BicFwComponent bicfw("scm", "bic", slot_id);
BicFwBlComponent bicfwbl("scm", "bicbl", slot_id);
MeComponent me("scm", "me", slot_id);
CpldComponent cpld("scm", "cpld", slot_id);
BiosComponent bios("scm", "bios", slot_id);

