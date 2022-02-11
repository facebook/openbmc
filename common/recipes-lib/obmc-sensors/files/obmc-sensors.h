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

/* DESCRIPTION: This library provides a unified interface on top of HWMON
 * sensors and is designed to work in a unified way across kernel versions.
 *
 * DESIGN CONSIDERATIONS: This is designed to be an atomic library. Even
 * though it was designed as a wrapper on top of LM-sensor library, it may
 * in future have abstractions to read sensors not available on the sensors
 * lib (Like how PWM is supported at the moment) -- Only after exploration
 * on adding a HWMON sensor has been explored. The library may read run-time
 * configurations but may not have compile time platform-specific dependencies
 * (Read: it should never depend on features from PAL but PAL may depend on this
 * library).
 * */

#ifndef _OBMC_SENSORS_H_
#define _OBMC_SENSORS_H_

#ifdef __cplusplus
extern "C" {
#endif


// Read the given chip's sensor value
int sensors_read(const char *chip, const char *label, float *value);

// Write sensor value. Not supported on all chips/labels
int sensors_write(const char *chip, const char *label, float value);

// Read Fan RPM (fan1-fanN) or PWM (pwm1-pwmN)
int sensors_read_fan(const char *label, float *value);

// Write Fan PWM (pwm1-pwmN)
int sensors_write_fan(const char *label, float value);

// Read ADC value
int sensors_read_adc(const char *label, float *value);

// Re-initialize SensorList
void sensors_reinit();

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* _OBMC_SENSORS_H_ */
