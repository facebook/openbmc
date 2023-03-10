/*
 * Copyright 2016-present Facebook. All Rights Reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openbmc/pal.h>
#include <string.h>
#include <dirent.h>
#include "CLI/CLI.hpp"
#include <iostream>
#include <pthread.h>

using std::string;
using std::cout;
using std::endl;

#define MAX_ZONE_NUM 16 //According to Aspeed chip PWM capability

const int ALL_FAN_NUM = -1;
static constexpr auto SENSOR_FAIL_RECORD_DIR = "/tmp/sensorfail_record";
static constexpr auto FAN_FAIL_RECORD_DIR = "/tmp/fanfail_record";
static constexpr auto SENSOR_FAIL_FILE = "/tmp/cache_store/sensor_fail_boost";
static constexpr auto FAN_FAIL_FILE = "/tmp/cache_store/fan_fail_boost";
static constexpr auto FAN_MODE_FILE = "/tmp/cache_store/fan_mode";
static constexpr auto FSCD_DRIVER_FILE = "/tmp/cache_store/fscd_driver";

enum {
  NORMAL = 0,
  TRANSIT = 1,
  BOOST = 2,
  PROGRESSION = 3,
  SB_BOOST = 4,
};

static void
sensor_fail_check(bool status) {
  struct dirent *directoryEntry;
  int cnt = 0;

  if (status) {
    cout << "Sensor Fail: Not support in manual mode(No fscd running)" << endl;
    return;
  }

  if (access(SENSOR_FAIL_FILE, F_OK) == 0) {
    auto dir = opendir(SENSOR_FAIL_RECORD_DIR);
    if (dir != NULL) {
      cout << "Sensor Fail: ";
      while((directoryEntry = readdir(dir)) != NULL) {
        if (strcmp(directoryEntry->d_name, ".") == 0 || strcmp(directoryEntry->d_name, "..") == 0)
          continue;
        cnt++;
        if (cnt == 1) {
          cout << "\n" << directoryEntry->d_name << endl;
          continue;
        }
        cout << directoryEntry->d_name << endl;
      }
      closedir(dir);
      if (cnt == 0)
        cout << "None" << endl;
    } else {
      cout << "Sensor Fail: None" << endl;
    }
  } else {
    cout << "Sensor Fail: None" << endl;
  }
  return;
}

static void
fan_fail_check(bool status) {
  struct dirent *directoryEntry;
  int cnt = 0;

  if (status) {
    cout << "Fan Fail: Not support in manual mode(No fscd running)" << endl;
    return;
  }

  if (access(FAN_FAIL_FILE, F_OK) == 0) {
    auto dir = opendir(FAN_FAIL_RECORD_DIR);
    if (dir != NULL) {
      cout << "Fan Fail: ";
      while((directoryEntry = readdir(dir)) != NULL) {
        if (strcmp(directoryEntry->d_name, ".") == 0 || strcmp(directoryEntry->d_name, "..") == 0)
          continue;
        cnt++;
        if (cnt == 1) {
          cout << "\n" << directoryEntry->d_name << endl;
          continue;
        }
        cout << directoryEntry->d_name << endl;
      }
      closedir(dir);
      if (cnt == 0)
        cout << "None" << endl;
    } else {
      cout << "Fan Fail: None" << endl;
    }
  } else {
    cout << "Fan Fail: None" << endl;
  }
  return;
}

static bool
fan_mode_check(bool printMode) {
  char buf[32];
  int res;
  int fd;
  uint8_t mode;
  int cnt;
  static constexpr auto cmd = "ps -w | grep [/]usr/bin/fscd.py | wc -l";

  auto filePointer = popen(cmd, "r");
  if(filePointer == NULL) {
    if (printMode)
      cout << "Fan Mode: Unknown" << endl;
    return false;
  }

  if(fgets(buf, sizeof(buf), filePointer) != NULL) {
    res = atoi(buf);
    if(res < 1) {
      if (printMode)
        cout << "Fan Mode: Manual(No fscd running)" << endl;
      pclose(filePointer);
      return true;
    }
  }
  pclose(filePointer);

  if (printMode) {
    fd = open(FAN_MODE_FILE, O_RDONLY);
    if (fd < 0) {
      cout << "Fan Mode: Unknown" << endl;
      return false;
    }

    cnt = read(fd, &mode, sizeof(uint8_t));

    if (cnt <= 0) {
      cout << "Fan Mode: Unknown" << endl;
      close(fd);
      return false;
    }

    mode = mode - '0';
    switch(mode) {
      case NORMAL:
        cout << "Fan Mode: Normal" << endl;
        break;
      case TRANSIT:
        cout << "Fan Mode: Transitional" << endl;
        break;
      case BOOST:
        cout << "Fan Mode: Boost" << endl;
        break;
      case PROGRESSION:
        cout << "Fan Mode: Progression" << endl;
        break;
      case SB_BOOST:
        cout << "Fan Mode: Standby Boost" << endl;
        break;
      default:
        cout << "Fan Mode: Unknown" << endl;
        break;
    }

    close(fd);
  }
  return false;
}

static int
fscd_driver_check(bool status) {
#define MAX_BUF_SIZE 256
  int rc;
  char buf[MAX_BUF_SIZE] = {0};

  if (status) {
    cout << "FSCD Driver: Not support in manual mode(No fscd running)" << endl;
    return -1;
  }

  auto filePointer = fopen(FSCD_DRIVER_FILE, "r");
  if (!filePointer) {
    cout << "FSCD Driver: N/A" << endl;
    return -1;
  }

  if (fgets(buf, MAX_BUF_SIZE, filePointer) == NULL) {
    cout << "FSCD Driver returned nothing" << endl;
    rc = -1;
  } else {
    cout << "FSCD Driver: " << buf << endl;
    rc = 0;
  }

  fclose(filePointer);
  return rc;
}

int
main(int argc, char **argv) {
  uint8_t pwm = 0xFF;
  int fan = ALL_FAN_NUM;
  uint8_t i = 0;
  int index = 0;
  int ret = 0;
  int rpm = 0;
  char fan_name[32];
  bool manu_flag = false;
  int pwm_cnt = 0;
  int tach_cnt = 0;
  string fanOpt = "";
  pwm_cnt = pal_get_pwm_cnt();
  tach_cnt = pal_get_tach_cnt();
  pthread_t tid[MAX_ZONE_NUM];
  PWM_INFO *pwm_info;
  pwm_info = (PWM_INFO *)malloc(MAX_ZONE_NUM * sizeof(PWM_INFO));
  bool fans_equal = pwm_cnt == tach_cnt;
  const string ident_set = fans_equal ? "Fan" : "Zone";
  const string ident_get = fans_equal ? "Fan" : "Tach";

  /* This is a temporary workaround that removes the dashes from the
   * command line input. In the future we need to change outside dependencies
   * so that they call the correct, non-dashed subcommands.
   */
  if(argc > 1) {
    if (strcmp(argv[1], "--set") == 0)
      strcpy(argv[1], "set");
    else if (strcmp(argv[1], "--get") == 0)
      strcpy(argv[1], "get");
    else if (strcmp(argv[1], "--get-drivers") == 0)
      strcpy(argv[1], "get-drivers");
    else if (strcmp(argv[1], "--auto-mode") == 0)
      strcpy(argv[1], "auto-mode");
  }

  CLI::App app{"Fan Util"};
  app.set_help_flag();
  app.set_help_all_flag("-h,--help");

  auto setCommand = app.add_subcommand("set", "set the pwm of a " + ident_set);
  setCommand->add_option("pwm", pwm, "%% pwm to set")->check(CLI::Range(0,100))->required();
  setCommand->add_option("fan", fan, ident_set + " to set pwm for")->check(CLI::Range(0,pwm_cnt-1));

  auto getCommand = app.add_subcommand("get", "get the pwm of a " + ident_get);
  getCommand->add_option("fan", fan, ident_get)->check(CLI::Range(0,tach_cnt-1));

  auto driverCommand = app.add_subcommand("get-drivers", "");

  string fanOptList = pal_get_fan_opt_list();
  if (fanOptList.length() > 0) {
    auto autoCommand = app.add_subcommand("auto-mode", "set auto mode");
    autoCommand->add_option("fan", fanOpt, "set auto mode [" + fanOptList + "]")
      ->required();
  }

  CLI11_PARSE(app, argc, argv);

  if (setCommand->parsed()) {
    // Set the Fan Speed.
    // If the fan num is not mentioned, set all fans to given pwm.
    for (i = 0; i < pwm_cnt; i++) {
      // Check if the fan number is mentioned.
      if (fan != ALL_FAN_NUM && fan != i)
          continue;
      (pwm_info + index)->fan_id = i;
      (pwm_info + index)->pwm = pwm;
      ret = pthread_create(&tid[i], NULL, pal_set_fan_speed_thread, (pwm_info + index));
      if (ret < 0) {
        syslog(LOG_WARNING, "%s(): Create pal_set_fan_speed_thread thread failed!", __func__);
        goto error_exit;
      }
      index++;
    }
    for (i = 0; i < pwm_cnt; i++) {
      if (fan != ALL_FAN_NUM && fan != i)
          continue;
      pthread_join(tid[i], NULL);
    }
  } else if (getCommand->parsed()) {
    for (i = 0; i < tach_cnt; i++) {

      // Check if the fan number is mentioned.
      if (fan != ALL_FAN_NUM && fan != i)
          continue;
      // Check fan present.
      if(pal_is_fan_prsnt(i)) {
        // Get the Fan Speed
        ret = pal_get_fan_speed(i, &rpm);
        pwm = 0;
        if (!ret) {
          memset(fan_name, 0, 32);
          pal_get_fan_name(i, fan_name);
          if (pal_get_pwm_value(i, &pwm) == 0)
            cout << fan_name << " Speed: " << rpm << " RPM (" << +pwm << "%)" << endl;
          else {
            cout << "Error while getting fan PWM for Fan " << +i << endl;
            cout << fan_name << " Speed: " << rpm << " RPM" << endl;
          }
        } else {
          cout << "Error while getting fan speed for Fan " << +i << endl;
        }
      }
    }
  }

  if ((getCommand->parsed()) && (argc == 2)) {
    manu_flag = fan_mode_check(true);
    fscd_driver_check(manu_flag);
    sensor_fail_check(manu_flag);
    fan_fail_check(manu_flag);
    pal_specific_plat_fan_check(manu_flag);
  } else if (driverCommand->parsed()) {
    manu_flag = fan_mode_check(false);
    fscd_driver_check(manu_flag);
  } else if ((fanOptList.length() > 0) && (app.got_subcommand("auto-mode"))) {
    ret = pal_set_fan_ctrl(argv[2]);
    if (ret < 0) {
        cout << "Error while setting fan auto mode : " << argv[2] << endl;
        goto error_exit;
    }
  } else if ( argc == 1 ) {
    cout << app.help() << endl;
  }
error_exit:
  if (pwm_info != NULL) {
    free(pwm_info);
  }
  return ret;
}
