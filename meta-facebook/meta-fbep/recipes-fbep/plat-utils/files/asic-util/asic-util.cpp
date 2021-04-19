/*
 * asic-util
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#include <iostream>
#include <string>
#include <syslog.h>
#include <CLI/CLI.hpp>
#include <openbmc/kv.h>
#include <openbmc/pal.h>
#include <openbmc/libgpio.h>
#include <facebook/asic.h>

enum {
  SLOT_ALL = -1,
  SLOT_0 = 0,
  SLOT_1,
  SLOT_2,
  SLOT_3,
  SLOT_4,
  SLOT_5,
  SLOT_6,
  SLOT_7,
};

static gpio_value_t gpio_get(std::string shadow)
{
  gpio_value_t value = GPIO_VALUE_INVALID;
  gpio_desc_t *desc = gpio_open_by_shadow(shadow.c_str());
  if (!desc) {
    std::cerr << "Open GPIO " << shadow << " failed" << std::endl;
    return GPIO_VALUE_INVALID;
  }
  if (gpio_get_value(desc, &value)) {
    std::cerr << "Get GPIO " << shadow << " failed" << std::endl;
    value = GPIO_VALUE_INVALID;
  }
  gpio_close(desc);
  return value;
}

static int gpio_set(std::string shadow, gpio_value_t value, bool to_input)
{
  int ret = 0;
  gpio_desc_t *desc = gpio_open_by_shadow(shadow.c_str());
  if (!desc) {
    std::cerr << "Open GPIO " << shadow << " failed" << std::endl;
    return -1;
  }
  if (gpio_set_direction(desc, GPIO_DIRECTION_OUT) || gpio_set_value(desc, value)) {
    std::cerr << "Set GPIO " << shadow << " failed" << std::endl;
    ret = -1;
  }
  if (to_input) {
    if (gpio_set_direction(desc, GPIO_DIRECTION_IN)) {
      std::cerr << "Set direction failed for GPIO: " << shadow << std::endl;
      ret = -1;
    }
  }
  gpio_close(desc);
  return ret;
}

static int show_linkconfig(int slot)
{
  std::string shadow = "OAM" + std::to_string(slot) + "_LINK_CONFIG";
  std::string str = "LINK_CONFIG[4:0]: ";
  gpio_desc_t *desc;
  gpio_value_t value;

  for (int i = 4; i >= 0; i--) {
    std::string name = shadow + std::to_string(i);
    desc = gpio_open_by_shadow(name.c_str());
    if (!desc) {
      std::cerr << "Open GPIO " << name << "failed" << std::endl;
      return -1;
    }
    if (gpio_get_value(desc, &value) < 0) {
      std::cerr << "Get GPIO " << name << "failed" << std::endl;
      gpio_close(desc);
      return -1;
    }
    gpio_close(desc);
    str += std::to_string((int)value);
  }

  std::cout << "\t" << str << std::endl;
  return 0;
}

static int show_pe_bif(int slot)
{
  std::string shadow = "OAM" + std::to_string(slot) + "_PE_BIF";
  std::string str;
  gpio_desc_t *desc;
  gpio_value_t value;
  int pe_bif = 0x0;

  for (int i = 1; i >= 0; i--) {
    std::string name = shadow + std::to_string(i);
    desc = gpio_open_by_shadow(name.c_str());
    if (!desc) {
      std::cerr << "Open GPIO " << name << "failed" << std::endl;
      return -1;
    }
    if (gpio_get_value(desc, &value) < 0) {
      std::cerr << "Get GPIO " << name << "failed" << std::endl;
      gpio_close(desc);
      return -1;
    }
    gpio_close(desc);
    pe_bif += (int)value << i;
  }
  switch (pe_bif)
  {
    case 0:
      str = "PE_BIF: One x16 PCIe host interface";
      break;
    case 1:
      str = "PE_BIF: Two x8 PCIe host interface";
      break;
    case 2:
      str = "PE_BIF: Four x4 PCIe host interface";
      break;
    default:
      str = "PE_BIF: Unknown PCIe host interface";
      break;
  }

  std::cout << "\t" << str << std::endl;
  return 0;
}

static int show_manf_mode(int slot)
{
  std::string shadow = "OAM" + std::to_string(slot) + "_MANF_MODE";
  std::string str;
  gpio_desc_t *desc;
  gpio_value_t value;

  desc = gpio_open_by_shadow(shadow.c_str());
  if (!desc) {
    std::cerr << "Open GPIO " << shadow << "failed" << std::endl;
    return -1;
  }
  if (gpio_get_value(desc, &value) < 0) {
    std::cerr << "Get GPIO " << shadow << "failed" << std::endl;
    gpio_close(desc);
    return -1;
  }
  gpio_close(desc);

  if (value == GPIO_VALUE_HIGH)
    str = "MANF_MODE: Normal mode";
  else
    str = "MANF_MODE: Manufacturing mode";

  std::cout << "\t" << str << std::endl;
  return 0;
}

static int show_fw_recover(int slot)
{
  std::string shadow = "OAM" + std::to_string(slot) + "_FW_RECOVERY";
  std::string str;
  gpio_desc_t *desc;
  gpio_value_t value;

  desc = gpio_open_by_shadow(shadow.c_str());
  if (!desc) {
    std::cerr << "Open GPIO " << shadow << "failed" << std::endl;
    return -1;
  }
  if (gpio_get_value(desc, &value) < 0) {
    std::cerr << "Get GPIO " << shadow << "failed" << std::endl;
    gpio_close(desc);
    return -1;
  }
  gpio_close(desc);

  if (value == GPIO_VALUE_HIGH)
    str = "FW_RECOVERY: Normal mode";
  else
    str = "FW_RECOVERY: Firmware Recovery mode";

  std::cout << "\t" << str << std::endl;
  return 0;
}

static int show_test_mode(int slot)
{
  std::string shadow = "OAM" + std::to_string(slot) + "_TEST_MODE";
  std::string str;
  gpio_desc_t *desc;
  gpio_value_t value;

  desc = gpio_open_by_shadow(shadow.c_str());
  if (!desc) {
    std::cerr << "Open GPIO " << shadow << "failed" << std::endl;
    return -1;
  }
  if (gpio_get_value(desc, &value) < 0) {
    std::cerr << "Get GPIO " << shadow << "failed" << std::endl;
    gpio_close(desc);
    return -1;
  }
  gpio_close(desc);

  if (value == GPIO_VALUE_HIGH)
    str = "TEST_MODE: Normal mode";
  else
    str = "TEST_MODE: Electrical mode";

  std::cout << "\t" << str << std::endl;
  return 0;
}

static int show_plink_cap(int slot)
{
  std::string shadow = "OAM" + std::to_string(slot) + "_PLINK_CAP";
  std::string str;
  gpio_desc_t *desc;
  gpio_value_t value;

  desc = gpio_open_by_shadow(shadow.c_str());
  if (!desc) {
    std::cerr << "Open GPIO " << shadow << "failed" << std::endl;
    return -1;
  }
  if (gpio_get_value(desc, &value) < 0) {
    std::cerr << "Get GPIO " << shadow << "failed" << std::endl;
    gpio_close(desc);
    return -1;
  }
  gpio_close(desc);

  if (value == GPIO_VALUE_LOW)
    str = "PLINK_CAP: PCIe only supported";
  else
    str = "PLINK_CAP: Alternate protocol supported";

  std::cout << "\t" << str << std::endl;
  return 0;
}

static int show_pwr_rdt(int slot)
{
  std::string shadow = "OAM" + std::to_string(slot) + "_PWRRDT";
  std::string str;
  gpio_desc_t *desc;
  gpio_value_t value;
  int pwr_rdt = 0x0;

  for (int i = 1; i >= 0; i--) {
    std::string name = shadow + std::to_string(i);
    desc = gpio_open_by_shadow(name.c_str());
    if (!desc) {
      std::cerr << "Open GPIO " << name << "failed" << std::endl;
      return -1;
    }
    if (gpio_get_value(desc, &value) < 0) {
      std::cerr << "Get GPIO " << name << "failed" << std::endl;
      gpio_close(desc);
      return -1;
    }
    gpio_close(desc);
    pwr_rdt += (int)value << i;
  }
  switch (pwr_rdt)
  {
    case 0:
      str = "PWRRDT: L3 Max power reduction";
      break;
    case 1:
      str = "PWRRDT: L2 2nd power reduction";
      break;
    case 2:
      str = "PWRRDT: L1 1st power reduction";
      break;
    case 3:
      str = "PWRRDT: L0 Normal power reduction";
      break;
    default:
      str = "PWRRDT: Unknown power reduction";
      break;
  }

  std::cout << "\t" << str << std::endl;
  return 0;
}

static int show_module_pwrgd(int slot)
{
  std::string shadow = "OAM" + std::to_string(slot) + "_MODULE_PWRGD";
  std::string str;
  gpio_desc_t *desc;
  gpio_value_t value;

  desc = gpio_open_by_shadow(shadow.c_str());
  if (!desc) {
    std::cerr << "Open GPIO " << shadow << "failed" << std::endl;
    return -1;
  }
  if (gpio_get_value(desc, &value) < 0) {
    std::cerr << "Get GPIO " << shadow << "failed" << std::endl;
    gpio_close(desc);
    return -1;
  }
  gpio_close(desc);

  if (value == GPIO_VALUE_HIGH)
    str = "MODULE_PWRGD: Normal";
  else
    str = "MODULE_PWRGD: Abnormal";

  std::cout << "\t" << str << std::endl;
  return 0;
}

static int show_asic_status(int slot)
{
  std::cout << "ASIC " << slot << " :" << std::endl;
  show_linkconfig(slot);
  show_pe_bif(slot);
  show_manf_mode(slot);
  show_fw_recover(slot);
  show_test_mode(slot);
  show_plink_cap(slot);
  show_pwr_rdt(slot);
  show_module_pwrgd(slot);
  return 0;
}

static int _show_status(int slot)
{
  if (slot == SLOT_ALL) {
    for (int i = 0; i < 8; i++) {
      if (is_asic_prsnt((uint8_t)i))
        show_asic_status(i);
      else
        std::cout << "ASIC on slot " << i << " is not present" << std::endl;
    }
  } else {
    if (is_asic_prsnt((uint8_t)slot))
      show_asic_status(slot);
    else
      std::cout << "ASIC on slot " << slot << " is not present" << std::endl;
  }
  return 0;
}

static int _set_power_limit(int slot, unsigned int watts)
{
  int ret, lock;
  unsigned int val;
  char asic_lock[32] = {0};

  snprintf(asic_lock, sizeof(asic_lock), "/tmp/asic_lock%d", slot);
  lock = open(asic_lock, O_CREAT | O_RDWR, 0666);
  if (lock < 0) {
    syslog(LOG_WARNING, "Failed to open ASIC lock %d", slot);
    return -1;
  }
  flock(lock, LOCK_EX);

  ret = asic_set_power_limit((uint8_t)slot, watts);
  if (ret < 0) {
    if (ret == ASIC_NOTSUP)
      std::cerr << "Power capping is not supported" << std::endl;
    else
      std::cerr << "Failed to set power limit of GPU on slot " << slot << std::endl;
    goto bail;
  }

  ret = asic_get_power_limit((uint8_t)slot, &val);
  if (ret < 0) {
    if (ret == ASIC_NOTSUP)
      std::cerr << "Get power limit is not supported" << std::endl;
    else
      std::cerr << "Failed to get power limit of GPU on slot " << slot << std::endl;
    goto bail;
  }

  if (watts != val) {
    std::cerr << "Failed to set power limit of GPU on slot " << slot << std::endl;
    ret = -1;
  } else {
    std::cout << "Set power limit of GPU on slot " << slot
              << " to " << watts << " Watts" << std::endl;
    syslog(LOG_CRIT, "Set power limit of GPU on slot %d to %d Watts", (int)slot, watts);
  }

bail:
  flock(lock, LOCK_UN);
  close(lock);
  return ret;
}

static int set_power_limit(int slot, unsigned int watts)
{
  int ret = -1;

  if (slot == SLOT_ALL) {
    for (int i = 0; i < 8; i++) {
      if (is_asic_prsnt((uint8_t)i)) {
        ret = _set_power_limit(i, watts);
        if (ret < 0)
          break;
      } else {
        std::cout << "ASIC on slot " << i << " is not present" << std::endl;
      }
    }
  } else {
    if (is_asic_prsnt((uint8_t)slot)) {
      ret = _set_power_limit(slot, watts);
    } else {
      std::cout << "ASIC on slot " << slot << " is not present" << std::endl;
    }
  }

  return ret;
}

/*
 * BMC can't get the limit before setting it
 * Refer table 3.9-Async Request in DG-06034-002
 *
static int _get_power_limit(int slot)
{
  int ret, lock;
  unsigned int watts;
  char asic_lock[32] = {0};

  snprintf(asic_lock, sizeof(asic_lock), "/tmp/asic_lock%d", slot);
  lock = open(asic_lock, O_CREAT | O_RDWR, 0666);
  if (lock < 0) {
    syslog(LOG_WARNING, "Failed to open ASIC lock %d", slot);
    return -1;
  }
  flock(lock, LOCK_EX);

  ret = asic_get_power_limit((uint8_t)slot, &watts);
  if (ret < 0) {
    if (ret == ASIC_NOTSUP)
      std::cerr << "Get power limit is not supported" << std::endl;
    else
      std::cerr << "Failed to get power limit of GPU on slot " << slot << std::endl;
    goto bail;
  }
  std::cout << "Current power limit of GPU on slot " << slot
            << " is " << watts << " Watts" << std::endl;
bail:
  flock(lock, LOCK_UN);
  close(lock);
  return ret;
}

static int get_power_limit(int slot)
{
  int ret = -1;

  if (slot == SLOT_ALL) {
    for (int i = 0; i < 8; i++) {
      if (is_asic_prsnt((uint8_t)i)) {
        ret = _get_power_limit(i);
        if (ret < 0)
          break;
      } else {
        std::cout << "ASIC on slot " << i << " is not present" << std::endl;
      }
    }
  } else {
    if (is_asic_prsnt((uint8_t)slot)) {
      ret = _get_power_limit(slot);
    } else {
      std::cout << "ASIC on slot " << slot << " is not present" << std::endl;
    }
  }

  return ret;
}
*/

static int _show_info(int slot)
{
  char vendor[MAX_VALUE_LEN] = {0};

  if (kv_get("asic_mfr", vendor, NULL, KV_FPERSIST) < 0) {
    std::cerr << "Failed to get manufacturer info" << std::endl;
    return -1;
  }
  std::cout << "ASIC Manufacturer: " << vendor << std::endl;

  gpio_value_t brk_on = gpio_get("OAM_FAST_BRK_ON_N");
  if (brk_on == GPIO_VALUE_INVALID) {
    std::cerr << "Failed to get GPU power brake status" << std::endl;
    return -1;
  }
  std::cout << "GPU power brake: " << (brk_on == GPIO_VALUE_LOW? "Enabled":"Disabled") << std::endl;

//  get_power_limit(slot);
  return 0;
}

static int _enable_power_brake(gpio_value_t value)
{
  int ret = gpio_set("OAM_FAST_BRK_ON_N", value, false);
  if (ret == 0)
    std::cout << (value == GPIO_VALUE_LOW? "Enable":"Disable") << " GPU power brake" << std::endl;
  return ret;
}

static int _set_power_brake(gpio_value_t value)
{
  int ret;

  if (gpio_get("OAM_FAST_BRK_ON_N") != GPIO_VALUE_LOW) {
    std::cerr << "GPU power brake function is not enabled, try \"--enable-brake 1\"" << std::endl;
    return -1;
  }
  ret = gpio_set("OAM_FAST_BRK_N", value, false);
  if (ret == 0)
    std::cout << (value == GPIO_VALUE_LOW? "Set":"Unset") << " GPU power brake" << std::endl;

  return ret;
}

int main(int argc, char **argv)
{
  CLI::App app("Application-specific IC Utility");
  app.failure_message(CLI::FailureMessage::help);


  unsigned int watts;
  auto power = app.add_option("--set-power", watts,
                              "The value of power limit in Watts [100..400]");
  int slot;
  auto num = app.add_option("--slot", slot,
                            "Action to specified slot [0..7]\n"
                            "If not specified, ALL slots by default");

  unsigned int enable_power_brake;
  auto en_brk = app.add_option("--enable-brake", enable_power_brake,
                              "Enable GPU Power brake function, 1 = Enable, 0 = Disable");

  unsigned int set_power_brake;
  auto set_brk = app.add_option("--set-brake", set_power_brake,
                              "Manually trigger GPU Power brake, 1 = Enable, 0 = Disable");
  bool show_status = false;
  app.add_flag("--status", show_status, "Get ASIC status");

  bool show_info = false;
  app.add_flag("--info", show_info, "Get GPU information");

  app.require_option();
  CLI11_PARSE(app, argc, argv);

  if (!(*num)) {
    slot = SLOT_ALL;
  } else if (slot < SLOT_0 || slot > SLOT_7) {
    std::cerr << "Error: Slot number is out-of-range [0..7]" << std::endl;
  }

  if (*power) {
    if (watts < 100 || watts > 400) {
      std::cerr << "Error: Power limit is out-of-range [100..400]" << std::endl;
      return -1;
    } else {
      return set_power_limit(slot, watts);
    }
  }

  if (show_status)
    return _show_status(slot);

  if (show_info) {
    return _show_info(slot);
  }

  if (*en_brk) {
    return _enable_power_brake(enable_power_brake? GPIO_VALUE_LOW: GPIO_VALUE_HIGH);
  }

  if (*set_brk) {
    return _set_power_brake(set_power_brake? GPIO_VALUE_LOW: GPIO_VALUE_HIGH);
  }

  return 0;
}
