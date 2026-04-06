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

#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <openbmc/log.h>
#include <openbmc/misc-utils.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
/* Path define */
#define PATH_POSTCODE_LAST    "/mnt/data/postcode_last"
#define PATH_POSTCODE_CURRENT "/var/log/postcode_current"
#define POSTCODE_DEV_SNOOP "/dev/aspeed-lpc-snoop0"
#define POSTCODE_DEV_PCC "/dev/aspeed-lpc-pcc"
#define XP5P0_COME_PG         "/sys/bus/i2c/devices/1-0035/xp5p0_come_pg"
#define XP12P0_COME_PG        "/sys/bus/i2c/devices/1-0035/xp12p0_come_pg"
#define MAX_HISTORY 8192
/* 8192 codes * 3 bytes ("xx ") = 24,576 bytes. Assign 32768 bytes (32KB) to stay page-aligned (4KB * 8) and safe. */
#define STR_BUF_SIZE 32768
#define POST_END_TIMEOUT 20 // seconds to consider POST finished if code 0 lasts
//This is Snoop's native behavior; after the BIOS loads the operating system, the POST code poll from the snoop is always 0.

/* Global variables */
static char g_display_str[STR_BUF_SIZE]; 
static uint8_t history_buffer[MAX_HISTORY];
static int history_idx = 0;
static int history_count = 0;
static volatile int fd_post = -1, fd_curr_file = -1, fd_pg5 = -1, fd_pg12 = -1;

/* Init file handles */
int init_all_fds(const char *snoop_dev) {
    fd_pg5 = open(XP5P0_COME_PG, O_RDONLY);
    fd_pg12 = open(XP12P0_COME_PG, O_RDONLY);
    fd_post = open(snoop_dev, O_RDONLY | O_NONBLOCK);
    fd_curr_file = open(PATH_POSTCODE_CURRENT, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_post < 0 || fd_curr_file < 0 || fd_pg5 < 0 || fd_pg12 < 0) {
        OBMC_ERROR(LOG_ERR, "Initial FDs failed");
        return -1;
    }
    return 0;
}

/* Add a cleanup function */
void cleanup(int signum) {
    OBMC_INFO("Shutting down postcode-monitor.");
    if (fd_post >= 0) close(fd_post);
    if (fd_curr_file >= 0) close(fd_curr_file);
    if (fd_pg5 >= 0) close(fd_pg5);
    if (fd_pg12 >= 0) close(fd_pg12);
    closelog();
    exit(0);
}

/*Check power state*/
int is_power_on() {
    char buf5[16] = {0}, buf12[16] = {0};
    lseek(fd_pg5, 0, SEEK_SET);
    lseek(fd_pg12, 0, SEEK_SET);
    read(fd_pg5, buf5, 15);
    read(fd_pg12, buf12, 15);
    return (strtol(buf5, NULL, 0) == 1 && strtol(buf12, NULL, 0) == 1);
}

/* Archive the last power cycle postcodes */
void archive_last_history() {
    if (history_count == 0) {
        //fputs(&val, fp); 
        return;
    }
    FILE *fp = fopen(PATH_POSTCODE_LAST, "w");
    if (fp) {
        fputs(g_display_str, fp);
        fclose(fp);
        sync();
        OBMC_INFO("Last %d postcodes archived.", history_count);
    }
}

/* Store the real-time postcodes */
void update_current_history(uint8_t val) {
    history_buffer[history_idx] = val;
    history_idx = (history_idx + 1) % MAX_HISTORY;
    if (history_count < MAX_HISTORY) history_count++;

    int pos = 0;
    // Construct display string, newest code at the end
    for (int i = 0; i < history_count; i++) {
        int idx = (history_count < MAX_HISTORY) ? i : (history_idx + i) % MAX_HISTORY;
        pos += snprintf(g_display_str + pos, STR_BUF_SIZE - pos, "%02x ", history_buffer[idx]);
        if (pos >= STR_BUF_SIZE - 6) break;
    }
    g_display_str[pos] = '\n';
    g_display_str[pos+1] = '\0';

    // Write to current file
    pwrite(fd_curr_file, g_display_str, strlen(g_display_str), 0);
    ftruncate(fd_curr_file, strlen(g_display_str));
}

/* Poll aspeed_lpc_snoop events without blocking */
void postcode_poll_snoop_event(const char *postcode_dev) {
    if (init_all_fds(postcode_dev) < 0) return;

    struct pollfd pfd = { .fd = fd_post, .events = POLLIN };
    uint8_t last_received_code = 0xff;
    int prev_pwr = is_power_on();
    int post_finished = false;
    int is_pwron_triggered = false;
    time_t zero_start = 0;
    //uint8_t buf[64];
    uint8_t code;

    while (1) {
        // 1. Check power status (Off -> On)
        int curr_pwr = is_power_on();
        if (curr_pwr == 1 && prev_pwr == 0) {
            is_pwron_triggered = true;
            post_finished = false;
            // Physical Power-On detected, clear previous log
            if (fd_curr_file >= 0) {
                ftruncate(fd_curr_file, 0); 
                pwrite(fd_curr_file, "", 0, 0);
            }
            history_idx = 0; history_count = 0;
            memset(history_buffer, 0, sizeof(history_buffer));
            OBMC_INFO("Physical Power-On detected, ready for new POST cycle.");
        }
        prev_pwr = curr_pwr;

        // 2. Poll for postcode events
        int rc = poll(&pfd, 1, 1000);

        // --- postcode 0 lasting a certain time without new code change will be considered as finished ---
        if (zero_start != 0 && !post_finished) {
            if (time(NULL) - zero_start >= POST_END_TIMEOUT) {
                post_finished = true;
                OBMC_INFO("POST cycle finished by timeout, last code: 00");
                archive_last_history();
                zero_start = 0;
            }
        }

        if (rc > 0 && (pfd.revents & POLLIN)) {
            if (read(fd_post, &code, 1) <= 0) continue;

            // If code is 0, start or continue the zero timer; if non-zero, reset the zero timer
            if (code == 0) {
                if (zero_start == 0) zero_start = time(NULL);
            } else {
                zero_start = 0; 
            }

            // Reduce log noise by skipping duplicate codes.
            if (last_received_code == code) continue;
            last_received_code = code;

            if (!post_finished) {
                OBMC_INFO("%02x ", code);
                update_current_history(code);
            } else {
                // If finished already, none 0 consider a Warm Reset
                if (code != 0) {
                        OBMC_INFO("Warm Reset detected New cycle started");
                        post_finished = false;
                        // consider this as a new POST cycle start, clean current history
                        if (fd_curr_file >= 0) {
                        ftruncate(fd_curr_file, 0); 
                        pwrite(fd_curr_file, "", 0, 0);
                    }
                    history_idx = 0; history_count = 0;
                    memset(history_buffer, 0, sizeof(history_buffer));
                    update_current_history(code);
                }
            }
        }
    }
}

void usage(char const* app_name) {
    printf("Usage: %s [-h] -I <soop device path>\n", app_name);
    printf("   options: \n");
    printf("      -h: display usage\n");
    printf("      -I: postcode device path\n");
}

int main(int argc, char *argv[]) {
    int opt, rc = 0;
    char *input_path = NULL;
    while ((opt = getopt(argc, argv, "hI:")) != -1) {
        switch (opt) {
            case 'h':
                usage(argv[0]);
                return 0;
                break;
            case 'I':
                input_path = optarg;
                break;
            default:
                OBMC_ERROR(LOG_ERR, " invalid options [%c]", opt);
                usage(argv[0]);
                return 1;
        }
    }
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    openlog(argv[0], LOG_PID, LOG_DAEMON);
    OBMC_INFO("Starting postcode-monitor:%s\n",input_path);
    syslog(LOG_INFO, "Starting postcode-monitor##\n");
    if (access(input_path, F_OK) == -1 ) {
        OBMC_ERROR(LOG_ERR, "postcode device file does not exist");
        return 1;
    }
    if (strstr(input_path, "snoop")) {
        OBMC_INFO("Monitoring POST codes from aspeed LPC snoop device");
        postcode_poll_snoop_event(input_path);
    }
    else{
        OBMC_ERROR(LOG_ERR, "Unsupported postcode device");
        return 1;
    }
    return 0;
}