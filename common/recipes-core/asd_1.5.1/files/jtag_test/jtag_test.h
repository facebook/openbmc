/*
Copyright (c) 2019, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _JTAG_TEST_H_
#define _JTAG_TEST_H_

#include <stdbool.h>
#include <stddef.h>
#include "../target/jtag_handler.h"
#include "../include/logging.h"

#define MAX_TAPS_SUPPORTED 16
#define MAX_TDO_SIZE 2048
#define DEFAULT_TAP_DATA_PATTERN 0xdeadbeefbad4f00d // for tap comparison
#define SIZEOF_TAP_DATA_PATTERN 8
#define IR08_SHIFT_SIZE 8        // 8  bits per uncore
#define DEFAULT_IR_SHIFT_SIZE 11 // 11 bits per uncore
#define IR12_SHIFT_SIZE 12       // 12 bits per uncore
#define IR14_SHIFT_SIZE 14       // 14 bits per uncore
#define IR16_SHIFT_SIZE 16       // 16 bits per uncore
#define MAX_IR_SHIFT_SIZE 0x400
#define DEFAULT_NUMBER_TEST_ITERATIONS 1
#define DEFAULT_IR_VALUE 2
#define DEFAULT_DR_SHIFT_SIZE 32
#define MAX_DR_SHIFT_SIZE 0x20000
#define DEFAULT_TO_MANUAL_MODE false
#define DEFAULT_JTAG_CONTROLLER_MODE SW_MODE
#define DEFAULT_JTAG_TCK 1
#define SIZEOF_ID_CODE 4
#define UNCORE_DISCOVERY_SHIFT_SIZE_IN_BITS                                    \
    (((MAX_TAPS_SUPPORTED * SIZEOF_ID_CODE) + SIZEOF_TAP_DATA_PATTERN) * 8)
#define DEFAULT_LOG_LEVEL ASD_LogLevel_Info
#define DEFAULT_LOG_STREAMS ASD_LogStream_Test

#define IR_SIG_MASK 0x0FFFFFFF

typedef struct jtag_test_args
{
    unsigned int fru;
    unsigned long long int human_readable;
    unsigned int ir_shift_size;
    bool loop_forever;
    int numIterations;
    unsigned int ir_value;
    unsigned int dr_shift_size;
    bool manual_mode;
    bool count_mode;
    bool random_mode;
    int mode;
    unsigned int tck;
    unsigned char tap_data_pattern[8]; // Used for tap data comparison
    ASD_LogLevel log_level;
    ASD_LogStream log_streams;
} jtag_test_args;

typedef struct uncore_info
{
    unsigned int idcode[MAX_TAPS_SUPPORTED];
    unsigned int numUncores;
} uncore_info;

typedef struct ir_shift_size_map
{
    unsigned int signature;
    unsigned int ir_shift_size;
} ir_shift_size_map;

extern bool continue_loop;

#ifndef UNIT_TEST_MAIN
int main(int argc, char** argv);
#endif

int jtag_test_main(int argc, char** argv);

void interrupt_handler(int dummy);

void shift_right(unsigned char* buffer, size_t buffer_size);

void shift_left(unsigned char* buffer, size_t buffer_size);

bool parse_arguments(int argc, char** argv, jtag_test_args* args);

void showUsage(char** argv);

JTAG_Handler* init_jtag(jtag_test_args* args);

unsigned int find_pattern(const unsigned char* haystack,
                          unsigned int haystack_size,
                          const unsigned char* needle);

bool reset_jtag_to_RTI(JTAG_Handler* jtag);

bool uncore_discovery(JTAG_Handler* jtag, uncore_info* uncore,
                      jtag_test_args* args);

bool jtag_test(JTAG_Handler* jtag, uncore_info* uncore, jtag_test_args* args);

void print_test_results(uint64_t iterations, uint64_t micro_seconds,
                        uint64_t total_bits, uint64_t failures);

#endif // _JTAG_TEST_H_
