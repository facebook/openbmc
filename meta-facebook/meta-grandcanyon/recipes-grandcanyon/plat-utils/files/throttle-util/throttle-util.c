/*
 * throttle-util
 *
 * Copyright 2021-present Facebook. All Rights Reserved.
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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <ctype.h>
#include <openbmc/pal.h>
#include <facebook/bic.h>
#include <facebook/fbgc_common.h>
#include <openbmc/obmc-i2c.h>

/* Throttle Source control (According to BS FPGA specification)
Bit 0:  IRQ_SML1_PMBUS_ALERT_N, FM_THROTTLE_N
Bit 1:  IRQ_PVCCIO_CPU0_VRHOT_LVC3_N
Bit 2:  IRQ_PVCCIN_CPU1_VRHOT_LVC3_N
Bit 3:  FAST_PROCHOT_N
Bit 4:  IRQ_UV_DETECT_N
Bit 5:  FM_HSC_TIMER
Bit 6:  IRQ_PVDDQ_DE_VRHOT_LVT3_N
Bit 7:  IRQ_PVDDQ_AB_VRHOT_LVT3_N */

#define FM_THROTTLE_BIT       0
#define CPU0_VRHOT_BIT        1
#define CPU1_VRHOT_BIT        2
#define FAST_PROCHOT_BIT      3
#define UV_DETECT_BIT         4
#define FM_HSC_TIMER_BIT      5
#define DIMM_DE_VRHOT_BIT     6
#define DIMM_AB_VRHOT_BIT     7

#define THROTTLE_CONFIG_ENABLE         1
#define THROTTLE_CONFIG_DISABLE        0

#define BS_FPGA_THROTTLE_CONFIG_OFFSET     0x03

static const char *option_list[] = {
  "fm_throttle",
  "cpu0_vr_hot",
  "cpu1_vr_hot",
  "fast_prochot",
  "uv_detect",
  "fm_hsc_timer",
  "dimm_de_vr_hot",
  "dimm_ab_vr_hot",
};

typedef struct {
  uint8_t register_offset;
  uint8_t request_data;
} i2c_rw_throttle_config_command;

static void
print_usage_help(void) {
  int i = 0;
  printf("Usage: throttle-util --get\n");
  printf("       throttle-util --set <option> <enable/disable>\n");
  printf("         <option>:\n");
  for (i = 0; i < (sizeof(option_list) / sizeof(option_list[0])); i++) {
    printf("           %s\n", option_list[i]);
  }
}

static int
get_throttle_config(uint8_t *response, uint8_t response_len) {
  int i2cfd = 0, ret = 0, retry = 0;
  i2c_rw_throttle_config_command command;
  
  if (response == NULL) {
    syslog(LOG_WARNING, "fail to get throttle config because NULL parameter: *response\n");
    return -1;
  }
  
  memset(&command, 0, sizeof(command));
  command.register_offset = BS_FPGA_THROTTLE_CONFIG_OFFSET;
  
  i2cfd = i2c_cdev_slave_open(I2C_BS_FPGA_BUS, BS_FPGA_SLAVE_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (i2cfd < 0) {
    syslog(LOG_ERR, "fail to open device: I2C BUS: %d", I2C_BS_FPGA_BUS);
    return i2cfd;
  }
  
  while (retry < MAX_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, BS_FPGA_SLAVE_ADDR, (uint8_t*)&command.register_offset, sizeof(command.register_offset), response, response_len);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      ret = 0;
      break;
    }
  }
  if (retry == MAX_RETRY) {
    syslog(LOG_ERR, "fail to read server FPGA offset: 0x%02X via i2c\n", command.register_offset);
    ret = -1;
  }

  close(i2cfd);
  return ret;
}

static int
set_throttle_config(uint8_t original_data, uint8_t setting_bit, uint8_t setting_value) {
  int i2cfd = 0, ret = 0, retry = 0;
  i2c_rw_throttle_config_command command;
  uint8_t setting_data = 0;
  
  if (setting_value == THROTTLE_CONFIG_ENABLE) {
    setting_data = SETBIT(original_data, setting_bit);
  } else {
    setting_data = CLEARBIT(original_data, setting_bit);
  } 
  
  memset(&command, 0, sizeof(command));
  command.register_offset = BS_FPGA_THROTTLE_CONFIG_OFFSET;
  command.request_data = setting_data;
  
  i2cfd = i2c_cdev_slave_open(I2C_BS_FPGA_BUS, BS_FPGA_SLAVE_ADDR >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (i2cfd < 0) {
    syslog(LOG_ERR, "fail to open device: I2C BUS: %d", I2C_BS_FPGA_BUS);
    return i2cfd;
  }
  
  while (retry < MAX_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, BS_FPGA_SLAVE_ADDR, (uint8_t *)&command, sizeof(command), NULL, 0);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      ret = 0;
      break;
    }
  }
  if (retry == MAX_RETRY) {
    syslog(LOG_ERR, "fail to write server FPGA offset: 0x%02X via i2c\n", command.register_offset);
    ret = -1;
  }

  close(i2cfd);
  return ret;
}

int
main(int argc, char **argv) {
  int ret = 0, i = 0, config_bit = 0;
  uint8_t setting_value = THROTTLE_CONFIG_ENABLE;
  uint8_t response = 0;

  // get throttle config
  if ((argc == 2) && (strcmp(argv[1], "--get") == 0)) {
    ret = get_throttle_config(&response, sizeof(response));
    if (ret < 0) {
      printf("throttle-util: fail to get throttle config\n");
      return -1;
    }

    printf("Throttle Source Control\n");
    for (i = 0; i < (sizeof(option_list) / sizeof(option_list[0])); i++) {
      if (strcmp(option_list[i], "fm_throttle") == 0) {
        config_bit = FM_THROTTLE_BIT;
        
      } else if (strcmp(option_list[i], "cpu0_vr_hot") == 0) {
        config_bit = CPU0_VRHOT_BIT;
        
      } else if (strcmp(option_list[i], "cpu1_vr_hot") == 0) {
        config_bit = CPU1_VRHOT_BIT;
        
      } else if (strcmp(option_list[i], "fast_prochot") == 0) {
        config_bit = FAST_PROCHOT_BIT;
        
      } else if (strcmp(option_list[i], "uv_detect") == 0) {
        config_bit = UV_DETECT_BIT;
        
      } else if (strcmp(option_list[i], "fm_hsc_timer") == 0) {
        config_bit = FM_HSC_TIMER_BIT;
        
      } else if (strcmp(option_list[i], "dimm_de_vr_hot") == 0) {
        config_bit = DIMM_DE_VRHOT_BIT;
        
      } else if (strcmp(option_list[i], "dimm_ab_vr_hot") == 0) {
        config_bit = DIMM_AB_VRHOT_BIT;
      }
      
      
      if (GETBIT(response, config_bit) == THROTTLE_CONFIG_ENABLE) {
        printf("%15s: Enable\n", option_list[i]);
      } else {
        printf("%15s: Disable\n", option_list[i]);
      }
    }
    
    return ret;
  }
  
  // set throttle config
  if ((argc == 4) && (strcmp(argv[1], "--set") == 0)) {
    if (strcmp(argv[3], "enable") == 0) {
      setting_value = THROTTLE_CONFIG_ENABLE;
      
    } else if (strcmp(argv[3], "disable") == 0) {
      setting_value = THROTTLE_CONFIG_DISABLE;
      
    } else {
      printf("throttle-util: invalid option: %s\n\n", argv[3]);
      print_usage_help();
      return -1;
    }
    
    ret = get_throttle_config(&response, sizeof(response));
    if (ret < 0) {
      printf("throttle-util: fail to get throttle config\n");
      return -1;
    }
    
    if (strcmp(argv[2], "fm_throttle") == 0) {
      ret = set_throttle_config(response, FM_THROTTLE_BIT, setting_value);
      
    } else if (strcmp(argv[2], "cpu0_vr_hot") == 0) {
      ret = set_throttle_config(response, CPU0_VRHOT_BIT, setting_value);
      
    } else if (strcmp(argv[2], "cpu1_vr_hot") == 0) {
      ret = set_throttle_config(response, CPU1_VRHOT_BIT, setting_value);
      
    } else if (strcmp(argv[2], "fast_prochot") == 0) {
      ret = set_throttle_config(response, FAST_PROCHOT_BIT, setting_value);
    
    } else if (strcmp(argv[2], "uv_detect") == 0) {
      ret = set_throttle_config(response, UV_DETECT_BIT, setting_value);
    
    } else if (strcmp(argv[2], "fm_hsc_timer") == 0) {
      ret = set_throttle_config(response, FM_HSC_TIMER_BIT, setting_value);
      
    } else if (strcmp(argv[2], "dimm_de_vr_hot") == 0) {
      ret = set_throttle_config(response, DIMM_DE_VRHOT_BIT, setting_value);
      
    } else if (strcmp(argv[2], "dimm_ab_vr_hot") == 0) {
      ret = set_throttle_config(response, DIMM_AB_VRHOT_BIT, setting_value);
      
    } else {
      printf("throttle-util: invalid option: %s\n\n", argv[2]);
      print_usage_help();
      return -1;
    }
    
    if (ret < 0) {
      printf("throttle-util: fail to set throttle config\n");
    }
    return ret;
  }
  
  print_usage_help();
  return -1;
}
