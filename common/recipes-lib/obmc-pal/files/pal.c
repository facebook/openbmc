/*
 * Copyright 2018-present Facebook. All Rights Reserved.
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
#include "pal.h"

/* this file provides platform specific implementation
 * 
 * Anything added to the file which replaces this should be platform
 * specific. If it is not, please consider adding it to obmc-pal.c
 * or even better, having it as a library of its own.
 */

/* Have a dummy define to stop the compiler from complaining if
 * we decide to use the stubbed library */
void _pal_dummy(void)
{

}
