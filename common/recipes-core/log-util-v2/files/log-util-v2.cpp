/*
 * Copyright 2015-present Facebook. All Rights Reserved.
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

#define __USE_XOPEN
#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <openbmc/pal.h>

#define MAX_LEN 64
#define MAX_PID_LEN 32   // A 64-bit int can be represented in 20 characters.
#define MAX_LINE 1024
#define MAX_TOKEN_LEN 256
#define FRU_SYS 0x20
#define sem_path "/logsem"
#define JSON_FILE "/tmp/log_json_format.tmp"

enum {
  CMD_PRINT = 0,
  CMD_CLEAR = 1,
};

const char logfile_list[][MAX_LEN] = {"/mnt/data/logfile.0", "/mnt/data/logfile"};

static int
rsyslog_hup (void) {
  char pid[MAX_PID_LEN] = "";
  FILE *fp;
  char cmd[MAX_LEN] = "";
  int ret = -1;

  sprintf(cmd, "pidof rsyslogd");

  fp = popen(cmd, "r");
  if (NULL == fp) {
    // Ignore restart rsyslogd
    return -1;
  }
  if (NULL == fgets(pid, sizeof(pid), fp)) {
    return -1;
  }
  ret = pclose(fp);
  if(-1 == ret) {
     printf("%s: pclose() fail ", __func__);
  }

  if (strspn(pid, "0123456789") == strlen(pid)-1) {
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "kill -HUP %s", pid);
    if (0 != system(cmd)) {
      return -1;
    }
  }

  return 0;
}

static int
print_log (uint8_t fru_id, bool opt_json) {
  FILE *fd = NULL;
  char strline[MAX_LINE] = "";
  int ret = -1;
  size_t i,j,k;
  size_t len;
  char *pch;
  bool print_pair_flag = false;
  uint8_t pair_slot;
  char pair_fruname[MAX_LEN] = "";
  int fru_num;
  char fruname[MAX_LEN] = "";
  char tmp_str_token[64][MAX_TOKEN_LEN];
  char curtime[MAX_TOKEN_LEN*4] = "";
  struct tm ts;
  int ts_end;
  char hostname[MAX_LEN] = "";
  char version[MAX_LEN] = "";
  char app[MAX_LEN] = "";
  char message[MAX_LINE] = "";
  bool first_json = true;

  print_pair_flag = pal_get_pair_fru(fru_id, &pair_slot);
  if (opt_json) {
    printf("{\n    \"Logs\": [\n");
  } else {
    printf("%-4s %-8s %-22s %-16s %s\n",
           "FRU#",
           "FRU_NAME",
           "TIME_STAMP",
           "APP_NAME",
           "MESSAGE");
  }

  for(i=0; i<sizeof(logfile_list)/MAX_LEN; i++) {
    if ((fd = fopen(logfile_list[i],"a+")) == NULL) {
      printf("Open %s falied!", logfile_list[i]);
      return -1;
    }

    while (fgets(strline,MAX_LINE,fd) != NULL) {
//      printf("Original 666 %s", strline);

      // Print only critical logs
      if (!(strstr(strline, ".crit") || strstr(strline, "log-util"))) {
        continue;
      }

      // Find the FRU number
      pch = strstr(strline, "FRU: ");
      if (pch) {
        fru_num = atoi(pch+5);
      } else {
        fru_num = 0;
      }

      // FRU # is always aligned with indexing of fru list
      memset(fruname, 0, sizeof(fruname));
      if (fru_id == FRU_SYS && fru_num == 0) {
        strcpy(fruname, "sys");
      } else if (fru_num == 0) {
        strcpy(fruname, "all");
      } else if (fru_num != 0) {
        ret = pal_get_fru_name(fru_num, fruname);
        if (0 != ret) {
          if (fru_id != FRU_ALL) {
            continue;
          }
        }
      }

      // Print only if the argument fru matches the log fru
      if (strstr(strline, "log-util")) {
        if (strstr(strline, "all logs")) {
          if (!opt_json) {
            printf("%s\n", strline);
          }
          continue;
        }
      }

      if (print_pair_flag && pair_slot != 0) {
        ret = pal_get_fru_name(pair_slot, pair_fruname);
        if (0 != ret)
          continue;
        if (fru_id != FRU_ALL && fru_id != fru_num && fru_num != pair_slot)
          continue;
      } else {
        if (fru_id != FRU_ALL && fru_id != fru_num) {
          if (fru_id != FRU_SYS || fru_num != 0)
            continue;
        }
      }

      if (strstr(strline, "log-util")) {
        if (!opt_json) {
          printf("%s\n", strline);
        }
        continue;
      }

      // Split strline by space
      j=0;
      pch = strtok(strline, " ");
      memset(tmp_str_token, 0, sizeof(tmp_str_token));
      while (pch != NULL)
      {
        strcpy(tmp_str_token[j], pch);
        pch = strtok (NULL, " ");
        j++;
      }

      memset(curtime, 0, sizeof(curtime));
      if (strlen(tmp_str_token[0]) == 4 && strspn(tmp_str_token[0], "0123456789") == strlen(tmp_str_token[0])) {
        // Time format 2017 Sep 28 22:10:50
        ts_end = 4;
        sprintf(curtime, "%s %s %s %s", tmp_str_token[0], tmp_str_token[1], tmp_str_token[2], tmp_str_token[3]);
        strptime(curtime, "%Y %b %d %H:%M:%S", &ts);
        strftime(curtime, 256, "%Y-%m-%d %H:%M:%S", &ts);
      } else {
        // Time format Sep 28 22:10:50
        ts_end = 3;
        sprintf(curtime, "%s %s %s", tmp_str_token[0], tmp_str_token[1], tmp_str_token[2]);
        strptime(curtime, "%b %d %H:%M:%S", &ts);
        strftime(curtime, 256, "%m-%d %H:%M:%S", &ts);
      }

      // Hostname
      memset(hostname, 0, sizeof(hostname));
      strcpy(hostname, tmp_str_token[ts_end]);

      // OpenBMC Version Information
      memset(version, 0, sizeof(version));
      strcpy(version, tmp_str_token[ts_end+2]);

      // Application Name
      memset(app, 0, sizeof(app));
      strncpy(app, tmp_str_token[ts_end+3], strlen(tmp_str_token[ts_end+3]) > sizeof(app) ? sizeof(app)-1 : strlen(tmp_str_token[ts_end+3])-1);

      // Log Message
      memset(message, 0, sizeof(message));
      for (k=ts_end+4; k<j; k++) {
        if (k == j-1) {
          strncat(message, tmp_str_token[k], sizeof(message) - 1);
        }
        else {
          strncat(message, tmp_str_token[k], sizeof(message) - 1);
          strncat(message, " ", sizeof(message) - 1);
        }
      }
      len = strlen(message) - 1;
      if (*message && message[len] == '\n') 
        message[len] = '\0';

      if (opt_json) {
        if (first_json) {
          printf("        {\n");
          first_json = false;
        } else {
          printf(",\n");
          printf("        {\n");
        }
        printf("            \"FRU_NAME\": \"%s\",\n", fruname);
        printf("            \"FRU#\": \"%d\",\n", fru_num);
        printf("            \"TIME_STAMP\": \"%s\",\n", curtime);
        printf("            \"APP_NAME\": \"%s\",\n", app);
        printf("            \"MESSAGE\": \"%s\"\n", message);
        printf("        }");
      } else {
        printf("%-4d %-8s %-22s %-16s %s\n",
               fru_num,
               fruname,
               curtime,
               app,
               message);
      }
    }

    first_json = true;
    fclose(fd);
  }

  if (opt_json) {
    printf("\n    ]\n}\n");
  }

  return 0;
}

static int
clear_log (uint8_t fru_id) {
  FILE *fd_src, *fd_dst;
  size_t i;
  char strline[MAX_LINE] = "";
  int ret = -1;
  char *pch;
  int fru_num;
  char fruname[MAX_LEN] = "";
  char curtime[80] = "";
  time_t rawtime;
  struct tm *ts;
  char clear_str[MAX_LINE] = "";
  int pid;
  char tmp_path[MAX_LEN] = "";
  char fru_str[MAX_LEN] = "";

  pid = getpid();
  for(i=0; i<sizeof(logfile_list)/MAX_LEN; i++) {
    if ((fd_src = fopen(logfile_list[i],"a+")) == NULL) {
      printf("Open %s falied!", logfile_list[i]);
      return -1;
    }

    sprintf(tmp_path, "%s.tmp%d", logfile_list[i], pid);
    if ((fd_dst = fopen(tmp_path,"w")) == NULL) {
      printf("Create %s falied!", tmp_path);
      fclose(fd_src);
      return -1;
    }

    while (fgets(strline,MAX_LINE,fd_src) != NULL) {
//      printf("Original 666 %s", strline);

      // Print only critical logs
      if (!(strstr(strline, ".crit") || strstr(strline, "log-util"))) {
        continue;
      }

      // Find the FRU number
      pch = strstr(strline, "FRU: ");
      if (pch) {
        fru_num = atoi(pch+5);
      } else {
        fru_num = 0;
      }

      // Clear the log is the argument fru matches the log fru
      if (fru_id == FRU_ALL || fru_id == fru_num || (fru_id == FRU_SYS && fru_num == 0)) {
        // Drop this log line
        continue;
      } else {
        fputs(strline, fd_dst);
      }
    }

    // Dump the new log in a tmp file
    strcpy(fruname, "");
    if (!strcmp(logfile_list[i], "/mnt/data/logfile")) {
      if (fru_id == FRU_ALL) {
        sprintf(fruname, "all");
      } else {
        if (fru_id == FRU_SYS) {
          sprintf(fruname, "sys");
        } else {
          sprintf(fruname, "FRU: %d", fru_id);
        }
      }

      time(&rawtime);
      ts = localtime(&rawtime);
      strftime(curtime, 80, "%Y %b %d %H:%M:%S", ts);
      sprintf(clear_str, "%s log-util: User cleared %s logs\n", curtime, fruname);
      fputs(clear_str, fd_dst);
    }

    fclose(fd_dst);
    fclose(fd_src);

    ret = rename(tmp_path, logfile_list[i]);
    if (0 != ret) {
      printf("Clear %s Fail !!\n", logfile_list[i]);
    }
  } // End for loop

  // Clear FRU Health Status
  if (fru_id != FRU_SYS ) {
    if (fru_id == FRU_ALL) {
      sprintf(fru_str, "all");
    } else {
      ret = pal_get_fru_name(fru_id, fru_str);
      if (0 != ret)
        printf("pal_get_fru_name fail\n");
    }
    pal_log_clear(fru_str);
  }

  // Restart rsyslogd
  ret = rsyslog_hup();
  if (0 != ret)
    printf("rsyslog restart fail\n");

  return 0;
}

static void
print_usage_help(void) {
  printf("Usage: log-util [ %s, sys ] --print [ --json]\n", pal_fru_list);
  printf("       log-util [ %s, sys ] --clear\n", pal_fru_list);
}

/* Utility */
int main(int argc, char * argv[]) {
  uint8_t fru_id;
  uint8_t cmd;
  int ret = 0;
  bool opt_json = false;
  sem_t *sem = NULL;

  // Check for border conditions
  if ((argc != 3) && (argc != 4)) {
    goto err_exit;
  }

  // Check if the fru passed in as argument exists in the fru list
  if (!strcmp(argv[1], "sys")) {
    fru_id = FRU_SYS;
  } else {
    ret = pal_get_fru_id(argv[1], &fru_id);
    if (ret < 0) {
      goto err_exit;
    }
  }

  // Check if the cmd passed in as argument exists in the cmd list
  if (!strcmp(argv[2], "--print")) {
    cmd = CMD_PRINT;
  } else if (!strcmp(argv[2] , "--clear")) {
    cmd = CMD_CLEAR;
  } else {
    printf("Unknown command: %s \n", argv[2]);
    goto err_exit;
  }

  // Get the optional command
  if (4 == argc) {
    if (!strcmp(argv[3] , "--json")) {
      if (cmd != CMD_PRINT) {
        printf("--json option is only valid for --print\n");
        goto err_exit;
      } else {
        opt_json = true;
      }
    } else {
      printf("Unknown command: %s \n", argv[3]);
      goto err_exit;
    }
  }

  switch (cmd) {
    case CMD_PRINT:
      ret = print_log(fru_id, opt_json);
      break;
    case CMD_CLEAR:
      sem = sem_open(sem_path, O_CREAT | O_EXCL, 0644, 1);
      if (sem == SEM_FAILED) {
        if (errno == EEXIST) {
          sem = sem_open(sem_path, 0);
        } else {
          return -1;
        }
      }

      sem_wait(sem);
      ret = clear_log(fru_id);
      sem_post(sem);
      sem_close(sem);
      break;
    default:
      printf("Unknown command: %s \n", argv[2]);
      goto err_exit;
      break;
  }

  if (0 != ret) {
    printf("Unexpected error\n");
  }

  return 0;

err_exit:
  print_usage_help();
  return -1;
}
