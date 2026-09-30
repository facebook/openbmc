/*
 * University of Illinois/NCSA Open Source License
 *
 * Copyright (c) 2023, Advanced Micro Devices, Inc.
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

#include <stdint.h>

/**
 *  @brief Displays the list of MI300 commands available for the module.
 *
 *  @details This function will print the list of MI300 commands for the module.
 *
 *  @param[in] exe_name input.
 *
 */
void get_mi300_tsi_commands(char *exec_name);

/**
 *  @brief Get the mi300 arguments.
 *
 *  @details This function will get the mi300 arguments.
 *
 *  @param[in] argc argument count.
 *
 *  @param[in] argv argument values.
 *
 *  @param[in] soc_die_num Socket Die index.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 *
 */
oob_status_t parseesb_mi300_args(int argc, char **argv, uint8_t soc_die_num);

/**
 *  @brief Get the mi300 mailbox commands.
 *
 *  @details This function will get the mi300 mailbox commands.
 *
 *  @param[in] exe_name tool name.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 *
 */
void get_mi300_mailbox_commands(char *exe_name);

/**
 *  @brief Get the mi300 specific tsi register descriptions.
 *
 *  @details This function will print mi300 specific tsi register
 *  descriptions.
 *
 *  @param[in] soc_die_num Socket Die number.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 *
 */
oob_status_t get_apml_mi300_tsi_register_descriptions(uint8_t soc_die_num);

/**
 *  @brief Get the mi300 mailbox commands summary.
 *
 *  @details This function will print mi300 specific mailbox
 *  commands summary.
 *
 *  @param[in] soc_die_num socket number.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 *
 */
void get_mi_300_mailbox_cmds_summary(uint8_t soc_die_num);
/**
 *  @brief Get the HBM temperature status.
 *
 *  @details This function will HBM temperature status.
 *
 *  @param[in] soc_die_num socket number.
 *
 *  @retval ::OOB_SUCCESS is returned upon successful call.
 *  @retval None-zero is returned upon failure.
 *
 */
oob_status_t get_hbm_temp_status(uint8_t soc_die_num);
