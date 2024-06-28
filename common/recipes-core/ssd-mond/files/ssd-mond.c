/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <openbmc/log.h>
#include <openbmc/ipmi.h>
#include <openbmc/misc-utils.h>

#define UNINITIALIZED_STATE     0xFF
#define SEL_SYS_EVENT_RECORD    0x02
#define POLL_INTERVAL           5
#define PRESENT                 1
#define ABSENT                  0

/*
 * @brief Function to generate IPMI raw commands to add SEL entries for SSD presence
 *
 * Depending on the presence of the SSD, it adds entries to the System Event Log (SEL)
 * that can be later checked using the `ipmitool sel list` command on x86 systems.
 *
 * The generated logs will indicate:
 *     - SSD Inserted: 5 | 09/21/2023 | 01:32:56 | Drive Slot / Bay | Device Present | Asserted
 *     - SSD Removed: 6 | 09/21/2023 | 01:33:02 | Drive Slot / Bay | Device Present | Deasserted
 *
 * @param assert Boolean indicating whether the event is an assertion (true) or deassertion (false).
 * @param state Boolean indicating the state of the device (true for present, false for absent).
 */
void ipmi_sel_add(bool assert, bool state) {

    uint8_t tbuf[19] = {
        [0] = 1,                        // Slot ID
        [1] = NETFN_STORAGE_REQ << 2,   // Net function for storage request
        [2] = CMD_STORAGE_ADD_SEL,      // Command for adding SEL entry
        [5] = SEL_SYS_EVENT_RECORD,     // Record type: System Event Record
        [13] = 0x0D,                    // Sensor Type , Drive Slot (Bay)
        [14] = 0x00,                    // Sensor number
        [15] = (assert ? 0 : 1) << 7 | 0x8, //Event type [6-0] 0x8 / 0 absent 1 present
        [16] = state ? 0x1 : 0x0,       // Presence state
        [17] = UNINITIALIZED_STATE,     //Event data 2
        [18] = UNINITIALIZED_STATE,     //Event data 3
    };
    uint8_t rbuf[5] = {0x00};
    uint16_t rlen = 0;
    uint8_t tlen = sizeof(tbuf);

    lib_ipmi_handle(tbuf, tlen, rbuf, &rlen);
}

/*
 * @brief Function to poll the SSD presence status file and generate SEL entries
 *
 * This function continuously monitors the SSD presence status file specified
 * by `ssd_presence_status_path`. When a change in the SSD presence state is
 * detected, it calls `ipmi_sel_add` to add an appropriate SEL entry.
 *
 * @param ssd_presence_status_path Path to the SSD presence status file.
 */
void ssd_poller(const char *ssd_presence_status_path) {

    int rc = 0;
    int last_state, current_state;
    
    rc = device_read(ssd_presence_status_path, &last_state);
    if (rc) {
        OBMC_ERROR(rc, "Error reading from SSD presence status file %s : %s\n",
            ssd_presence_status_path , strerror(rc));
            return;
    }

    while (1) {
        rc = device_read(ssd_presence_status_path, &current_state);
        if (rc) {
            OBMC_ERROR(rc, "Error reading from SSD presence status file %s : %s\n",
                ssd_presence_status_path , strerror(rc));
            return;
        }
        if (current_state != last_state) {
            OBMC_INFO("SSD state changed to %s", current_state == 1 ? "present" : "absent");
            if ( current_state == 1 ) {
                ipmi_sel_add(1, PRESENT);
            } else if ( current_state == 0 ) {
                ipmi_sel_add(1, ABSENT);
            }
            last_state = current_state;
        }
        sleep(POLL_INTERVAL);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        OBMC_ERROR(LOG_ERR, "Usage: %s <SSD presence status file>", argv[0]);
        return 1;
    }

    openlog(argv[0], LOG_PID, LOG_DAEMON);
    OBMC_INFO("Starting SSD-monitor");
    const char *ssd_status_path_file = argv[1];
    if (access(ssd_status_path_file, F_OK) == -1) {
        OBMC_ERROR(LOG_ERR, "SSD status path file does not exist");
        return 1;
    }
    ssd_poller(ssd_status_path_file);
    return 0;
}
