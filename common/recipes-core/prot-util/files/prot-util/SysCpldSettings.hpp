/*
 *
 * Copyright 2022-present Facebook. All Rights Reserved.
 *
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
#pragma once

#include <cstdint>

/*
 * This file is template of all cpld related settings. Each platform must to
 * override the file based on platform specific design.
 */
namespace sysCpld {
constexpr uint8_t sys_cpld_addr = 0x0f;

/* Authentication complete signal
 * Platform must override it by cpld design
 */
constexpr uint8_t prot_seq_signals = 0x00;
constexpr uint8_t auth_complete_bit = 0;

/* Boot fail signal
 * Platform must override it by cpld design
 */
constexpr uint8_t prot_ctrl_signals = 0x00;
constexpr uint8_t boot_fail_bit = 0;

constexpr uint8_t retry_delay = 50;
constexpr uint8_t max_retry = 3;
} // end of namespace sysCpld