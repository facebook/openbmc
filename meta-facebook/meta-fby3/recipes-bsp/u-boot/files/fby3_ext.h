/*
 * Copyright (C) 2012-2020  ASPEED Technology Inc.
 * Ryan Chen <ryan_chen@aspeedtech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __FBY3_CONFIG_EXT_H
#define __FBY3_CONFIG_EXT_H

/* u-boot reset command and reset() is implemented via sysreset
 * which will trigger the WDT to do full-chip reset by default
 * on FBY3 platform, u-boot reset is used to driven the external
 * WDTRST_N pin to TPM. But WDT Full-chip reset cannot successfully
 * driven the WDTRST_N pin, so define AST_SYSRESET_WITH_SOC will
 * make sysreset use SOC reset, and use AST_SYS_RESET_WITH_SOC as
 * SOC reset mask.
 * Notice: For system which loop back the WDTRST_N pin to BMC SRST pin.
 * the value of AST_SYSRESET_WITH_SOC does not matter, because
 * no matter how the BMC will get full reset by SRST pin.
 */
#define AST_SYSRESET_WITH_SOC 0x23FFFF3

#define CONFIG_ASPEED_WRITE_DEFAULT_ENV
#endif
