/*
 * University of Illinois/NCSA Open Source License
 *
 * Copyright (c) 2022, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Developed by:
 *
 *                 AMD Research and AMD Software Development
 *
 *                 Advanced Micro Devices, Inc.
 *
 *                 www.amd.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimers.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimers in
 *    the documentation and/or other materials provided with the distribution.
 *  - Neither the names of <Name of Development Group, Name of Institution>,
 *    nor the names of its contributors may be used to endorse or promote
 *    products derived from this Software without specific prior written
 *    permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS WITH THE SOFTWARE.
 *
 */
#ifndef INCLUDE_APML_RECOVER_H_
#define INCLUDE_APML_RECOVER_H_

/** \file apml_recovery.h
 *  Header file for the APML recovery flow
 */

/**
 * @brief APML Client Devices
 */
typedef enum {
	DEV_SBRMI = 0x0,
	DEV_SBTSI,
} apml_client;


/**
 *  @brief Recover the APML client device for the given socket.
 *
 *  @details This function will recover the APML client device and
 *  returns successful on recovery or error if recovery is unsuccessful
 *
 *  NOTE: This fix is a software workaround for the erratum,
 *  Erratum:1444, "Advanced Platform Management Link (APML)
 *  May Cease to Function After Incomplete Read Transaction"
 *  Further details can be found in mentioned tech doc
 *  https://www.amd.com/system/files/TechDocs/57095-PUB_1.00.pdf
 *
 *  @param[in] soc_die_num [3:0]Socket index, [7:4]Die index.
 *
 *  @param[in] client DEV_SBRMI[0]/DEV_SBTSI[1] enum: apml_client
 *
 *  @retval ::OOB_SUCCESS is returned upon successful recovery
 *  @retval Non-zero is returned upon failure.
 *
 */
oob_status_t apml_recover_dev(uint8_t soc_die_num, uint8_t client);
#endif	//INCLUDE_APML_RECOVERY_H_
