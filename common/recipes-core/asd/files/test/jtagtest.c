/*
Copyright (c) 2017, Intel Corporation
 
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
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <openbmc/pal.h>
#include "asd/SoftwareJTAGHandler.h"

#define ASD_HW_JUMPER_REMOVE   1
#define ASD_HW_JUMPER_POP      0

#ifndef timersub
#define timersub(a, b, result)                           \
    do {                                                 \
        (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;    \
        (result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
        if ((result)->tv_usec < 0) {                     \
            --(result)->tv_sec;                          \
            (result)->tv_usec += 1000000;                \
        }                                                \
    } while (0)
#endif

// Global variables
static bool continueLoop = true;
unsigned int totalBits;

// interrupt handler for ctrl-c
void intHandler(int dummy) {
    continueLoop = false;
}

void printQ(unsigned int disablePrint, const char* format, ... ) {
    if (disablePrint) {
        return;
    }
    va_list args;
    va_start( args, format );
    vprintf( format, args );
    va_end( args );
}

void showUsage(char **argv, unsigned int qFlag) {
    printQ(qFlag,"Usage: %s [option]\n", argv[0]);
    printQ(qFlag,"  -q            Run quietly, no printing to the console\n");
    printQ(qFlag,"  -f            Run endlessly until ctrl-c is used\n");
    printQ(qFlag,"  -s <number>   Connect to fru <number> (default=1)\n");
    printQ(qFlag,"  -i <number>   Run [number] of iterations\n");
    printQ(qFlag,"  -r <number>   IR size (CPU=11, PCH=8, default 11)\n");
    printQ(qFlag,"  -j            Get asd jumper status of the fru\n");
    printQ(qFlag,"\n");
}

static int
check_dup_process(uint8_t fru) {
  int pid_file;
  char path[64];

  sprintf(path, "/var/run/asd_%d.lock", fru);
  pid_file = open(path, O_CREAT | O_RDWR, 0666);
  if (flock(pid_file, LOCK_EX | LOCK_NB) && (errno == EWOULDBLOCK)) {
    return -1;
  }
  return 0;
}

int
get_jumper_status(uint8_t fru) {
    uint8_t status = 0;
    int ret = pal_is_jumper_enable(fru, &status);
    if (ret < 0) {
        printf("Fail to get Jumper status\n");
        return -1;
    }

    if (status == ASD_HW_JUMPER_REMOVE) {
        printf("HW jumper is nonpop\n");
    } else if (status == ASD_HW_JUMPER_POP) {
        printf("HW jumper is popped\n");
    } else {
        printf("get jumper status fail, get value = %x", status);
    }
    return 0;
}

int main (int argc, char **argv) {
    unsigned char shiftDataOut[2048]; // 16384 bits is probably more than needed for shifts
    unsigned char compareData[24]; // 4*32bits (idcode) + 64 bits (shift in data)
    unsigned char idcode[16]; // We can save 4 idcodes at most...
    //unsigned char dead_beef[8] = {0x0d, 0xf0, 0xd4, 0xba, 0xef, 0xbe, 0xad, 0xde};  // Used for tap data comparison
    unsigned char dead_beef[8];  // Used for tap data comparison
    unsigned long long human_readable = 0xdeadbeefbad4f00d; // Used for tap data comparison
    uint64_t milSec;
    unsigned int irSize = 11; // 11 bits per uncore
    unsigned int numUncores = 0;
    unsigned int writeSize = 0;
    unsigned long long ir_command = 0; // 8 bytes (enough to support 4 sockets)
    unsigned int shiftSize = 0;
    unsigned int fFlag = 0;
    unsigned int qFlag = 0;
    unsigned int mFlag = 0;
    unsigned int jumperStatusFlag = 0;
    unsigned int numIterations = 0;
    unsigned int shift_size_in_bits = 0;
    unsigned int c = 0, i = 0, j = 0, loopCnt = 0, fru=1;
    struct timeval tval_before, tval_after, tval_result;
    JTAG_Handler *handle;

    memcpy(dead_beef, &human_readable, sizeof(human_readable));

    signal(SIGINT, intHandler);  // catch ctrl-c

    opterr = 0;
    while ((c = getopt (argc, argv, "qfmji:s:r:?")) != -1)
        switch (c) {
            case 'q':
                qFlag = 1;
                break;
            case 'f':
                fFlag = 1;
                numIterations = 1;
                break;
            case 'm':
                mFlag = 1;
                break;
            case 'i':
                if ((numIterations = atoi(optarg)) > 0)
                    break;
            case 's':
                if ((fru = (unsigned int)strtoul(optarg, NULL, 0)) > 0)
                    break;
            case 'r':
                if ((irSize = atoi(optarg)) > 0)
                    break;
            case 'j':
                jumperStatusFlag = 1;
                break;
            case '?':
                showUsage(argv, qFlag);
            return -1;
        }


    if (jumperStatusFlag) {
        get_jumper_status(fru);
    }

    // check that all the arguments have been processed and that the user didn't provide any we can't process
    if (optind < argc || argc == 1 || !numIterations) {
        showUsage(argv, qFlag);
        return -1;
    }
    if (check_dup_process(fru) != 0) {
        printf("ERROR: Another instance running for FRU: %d\n", fru);
        return -1;
    }

    printf("ASD: connect to fru %d, irSize=%d, %s mode\n", fru, irSize, (mFlag)?"HW":"SW");
    syslog(LOG_WARNING, "ASD: ASD test on fru %d, irSize=%d, %s mode",
           fru, irSize, (mFlag)?"HW":"SW");

    // load the driver
    handle = SoftwareJTAGHandler(fru); // TODO get from user
    if (!handle) {
      printf("Setup of JTAG driver failed!\n");
      return -1;
    }
    if (JTAG_initialize(handle, !mFlag) != ST_OK) {
      printf("Initialize of JTAG driver failed!\n");
      return -1;
    }

    //*************************************************************************
    // Start automatic uncore discovery
    if (JTAG_set_tap_state(handle, JtagTLR) != ST_OK) {
      printQ(qFlag,"Unable to set TLR tap state!\n");
      goto error;
    }

    if (JTAG_set_tap_state(handle, JtagRTI) != ST_OK) {
        printQ(qFlag,"Unable to set RTI tap state!\n");
        goto error;
    }

    // determine idcodes in the tap
    if (JTAG_set_tap_state(handle, JtagShfDR) != ST_OK) {
        printQ(qFlag,"Unable to set the tap state to 2!\n");
        goto error;
    }

    // Fill the buffer to be shifted out with 1s
    memset(shiftDataOut, 0xff, sizeof(shiftDataOut));

    shiftSize = 192; // bits
    // shift 192 bits including our known pattern so that this allows 4x32bit idcodes+known pattern
    if (JTAG_shift(handle, shiftSize, sizeof(dead_beef),(unsigned char *)&dead_beef, sizeof(shiftDataOut), (unsigned char *)&shiftDataOut, JtagRTI) != ST_OK) {
        printQ(qFlag,"Unable to read idcode!\n");
        goto error;
    }

    // this assumes that IDCODE is byte aligned since the JTAG spec says they're 32bits each.
    for (i = 0; i < (shiftSize/8); i++) {
        if (memcmp(&shiftDataOut[i], (unsigned char *)&dead_beef, sizeof(dead_beef)) == 0) {
            shift_size_in_bits = i*8;
            printQ(qFlag,"\nFound TDI data on TDO after %d bits.\n", shift_size_in_bits);
            break;
        } else {
            shift_size_in_bits = 0;
        }
    }

    // check to see the shift_size_in_bits is not 0 and that we're on a power of two
    if ((shift_size_in_bits == 0) || ((shift_size_in_bits & (shift_size_in_bits - 1)) != 0)){
        shiftSize = 64;
        printQ(qFlag,"TDI data was not seen on TDO.  Please ensure the target is on.\n");
        printQ(qFlag,"Here is the first %d bits of data seen on TDO that might help to debug the problem:\n", shiftSize);
        for (i = 0; i < (shiftSize/8); i += 8) {
            printQ(qFlag,"0x%x: ", i);
            for (j = 0; j < 8; ++j) {
                printQ(qFlag,"0x%02x ", shiftDataOut[i + j]);
            }
            printQ(qFlag,"\n");
        }
        printQ(qFlag,"\n");
        goto error;
    }

    // The number of uncores in the system will be the number of bits in the shiftDR / 32 bits
    numUncores = shift_size_in_bits/32;

    printQ(qFlag,"Found %d possible device%s\n",numUncores,numUncores == 1 ? "" : "s");

    for (i = 0; i < numUncores; i++) {
        printQ(qFlag,"Device %d: 0x", i);
        for (j = (4+(4*i)); j > i*4; j--) {
            printQ(qFlag,"%02x", shiftDataOut[j-1]);
        }
        printQ(qFlag,"\n");
    }

    printQ(qFlag,"Additional data shifted through the tap: 0x");
    for (i = (numUncores * 4)+8; i > (numUncores * 4); i--) {
        printQ(qFlag,"%02x", shiftDataOut[i-1]);
    }
    printQ(qFlag,"\n");

    // save the idcodes for later comparison
    memcpy(idcode, shiftDataOut, 4*numUncores);

    // End automatic uncore discovery
    //*************************************************************************


    //*************************************************************************
    // Run test pattern

    // reset tap
    if (JTAG_set_tap_state(handle, JtagTLR) != ST_OK) {
        printQ(qFlag,"Unable to set TLR tap state!\n");
        goto error;
    }

    if (JTAG_set_tap_state(handle, JtagRTI) != ST_OK) {
        printQ(qFlag,"Unable to set RTI tap state!\n");
        goto error;
    }

    totalBits = 0;

    // Start of timer
    gettimeofday(&tval_before, NULL);

    // the main body of the iterations the user requested
    for (loopCnt = 0;loopCnt < numIterations; loopCnt++) {
        if (fFlag && continueLoop)
            numIterations++;
        // set idcode IR for the numUncores found in the drShift
        ir_command = 0;
        for (i = 0; i < numUncores; i++) {
            ir_command = ((ir_command << irSize) | 0x2);
        }

        // Set the tap state to Select IR
        if (JTAG_set_tap_state(handle, JtagShfIR) != ST_OK) {
            printQ(qFlag,"Unable to set the tap state to JtagShfIR!\n");
            goto error;
        }
        memset(shiftDataOut, 0x00, sizeof(shiftDataOut));

        writeSize = irSize*numUncores;
        // ShiftIR and remember to set the end state to something OTHER than ShiftDR since we can't move from shiftIR to ShiftDR directly.
        if (JTAG_shift(handle, writeSize, sizeof(ir_command), (unsigned char *)&ir_command, sizeof(shiftDataOut), (unsigned char *)&shiftDataOut, JtagPauIR) != ST_OK) {
            printQ(qFlag,"Unable to write IR for idcode!\n");
            goto error;
        }

        if (JTAG_set_tap_state(handle, JtagShfDR) != ST_OK) {
            printQ(qFlag,"Unable to set the tap state to JtagShfDR!\n");
            goto error;
        }

        writeSize = (numUncores*32)+64; // 32 bits per uncore idcode plus 64 bits for deadbeefbad4f00d
        if (JTAG_shift(handle, writeSize, sizeof(dead_beef), (unsigned char *)&dead_beef, sizeof(shiftDataOut), (unsigned char *)&shiftDataOut, JtagRTI) != ST_OK) {
            printQ(qFlag,"Unable to read shift data!\n");
            goto error;
        }

        memset(compareData, 0x00, sizeof(compareData));     // fill compareData with zeros
        memcpy(compareData, idcode, 4*numUncores); // copy the idcode from tap reset and shiftdr into compareData
        memcpy(&compareData[(4*numUncores)], &dead_beef, sizeof(dead_beef)); // copy deadbeefbad4f00d into compareData after idcode

        if (memcmp(compareData,shiftDataOut,((writeSize+7)/8)) != 0) {
            printQ(qFlag,"TAP results comparison failed!\n");
            printQ(qFlag,"Shifted data: \n");
            for (i = 0; i < (writeSize+7)/8; i += 8) {
                printQ(qFlag,"0x%x: ", i);
                for (j = 0; j < 8; ++j) {
                    printQ(qFlag,"0x%02x ", shiftDataOut[i + j]);
                }
                printQ(qFlag,"\n");
            }
            printQ(qFlag,"\n");

            printQ(qFlag,"Expected data: \n");
            for (i = 0; i < (writeSize+7)/8; i += 8) {
                printQ(qFlag,"0x%x: ", i);
                for (j = 0; j < 8; ++j) {
                    printQ(qFlag,"0x%02x ", compareData[i + j]);
                }
                printQ(qFlag,"\n");
            }
            printQ(qFlag,"\n");
            goto error;
        }

    } // end iterations loop

    // end of timer
    gettimeofday(&tval_after, NULL);
    timersub(&tval_after, &tval_before, &tval_result);
    printQ(qFlag,"Total bits: %d\n", totalBits);
    milSec = (tval_result.tv_sec * (uint64_t)1000) + (tval_result.tv_usec / 1000);
    printQ(qFlag,"Time elapsed: %f\n", (float)milSec/1000);

    printQ(qFlag,"Successfully finished %d %s of idcode with 64 bits of overshifted data!\n", numIterations, numIterations >1?"iterations":"iteration");

    if (JTAG_deinitialize(handle) != ST_OK)
        printQ(qFlag, "Failed to set ctrl mode to Slave!\n");
    return 0;

error:
    if (JTAG_deinitialize(handle) != ST_OK)
        printQ(qFlag, "Failed to set ctrl mode to Slave!\n");
    return -1;
}
