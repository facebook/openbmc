/*
 * cm-util
 *
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

#include <iostream>
#include  <iomanip>
#include <set>
#include <map>
#include <string>
#include <CLI/CLI.hpp>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>

struct ModeDesc {
  uint8_t mode;
  const std::string desc;
};

struct UsbChDesc {
  uint8_t channel;
  const std::string desc;
};

const std::map<std::string, ModeDesc> mode_map = {
    {"8S-0", {CM_MODE_8S_0, "8 Socket Mode, Tray 0 is primary"}},
    {"8S-1", {CM_MODE_8S_1, "8 Socket Mode, Tray 1 is primary"}},
    {"8S-2", {CM_MODE_8S_2, "8 Socket Mode, Tray 2 is primary"}},
    {"8S-3", {CM_MODE_8S_3, "8 Socket Mode, Tray 3 is primary"}},
    {"2S", {CM_MODE_2S, "2 Socket Mode"}},
    {"4S-4OU", {CM_MODE_4S_4OU, "4 Socket mode, in 4OU mode"}},
    {"4S-2OU-0", {CM_MODE_4S_2OU_0, "4 Socket mode with Tray 0 as primary"}},
    {"4S-2OU-1", {CM_MODE_4S_2OU_1, "4 Socket mode with Tray 1 as primary"}}
};


const std::map<std::string, UsbChDesc> cc_usb_ch_map = {
    {"CC-BMC0", {CC_USB_BMC_MB0, "Set CC USB Channel to Tray 0 BMC"}},
    {"CC-BMC1", {CC_USB_BMC_MB1, "Set CC USB Channel to Tray 1 BMC"}},
    {"CC-PCH0", {CC_USB_PCH_MB0, "Set CC USB Channel to Tray 0 PCH"}},
    {"CC-PCH1", {CC_USB_PCH_MB1, "Set CC USB Channel to Tray 1 PCH"}}
};

const std::map<std::string, UsbChDesc> ep_usb_ch_map = {
    {"EP-BMC0", {EP_USB_BMC_MB0, "Set EP USB Channel to Tray 0 BMC"}},
    {"EP-BMC1", {EP_USB_BMC_MB1, "Set EP USB Channel to Tray 1 BMC"}},
    {"EP-PCH0", {EP_USB_PCH_MB0, "Set EP USB Channel to Tray 0 PCH"}},
    {"EP-PCH1", {EP_USB_PCH_MB1, "Set EP USB Channel to Tray 1 PCH"}}
};

static int set_mode(uint8_t mode)
{
  uint8_t sys_mode;
  int rc = 0;

  if (mode == CM_MODE_8S_0 || mode == CM_MODE_8S_1 ||
      mode == CM_MODE_8S_2 || mode == CM_MODE_8S_3) {
    sys_mode = MB_8S_MODE;
  } else if (mode == CM_MODE_2S) {
    sys_mode = MB_2S_MODE;
  } else if (mode == CM_MODE_4S_4OU || mode == CM_MODE_4S_2OU_0 ||
             mode == CM_MODE_4S_2OU_1) {
    sys_mode = MB_4S_MODE;
  } else {
    std::cerr << "Configuration is invaild" << std::endl;
    return -1;
  }

  if (is_ep_present() && pal_ep_set_system_mode(sys_mode) < 0) {
    std::cerr << "Set JBOG system mode failed" << std::endl;
    rc = -1;
  }

  if (cmd_cmc_set_system_mode(mode, false) < 0) {
    std::cerr << "Set CM system mode failed" << std::endl;
    rc = -1;
  }
  return rc;
}

static int set_cc_usb_ch(uint8_t ch)
{
  uint8_t dev=0;
  uint8_t mb=0;

  if (ch >= CC_USB_CH_CNT)
    return -1;

  //Device is PCH
  if(ch >= CC_USB_PCH_MB0) {
    mb = ch - CC_USB_PCH_MB0;
    dev = 1;
  } else {
    mb = ch;
    dev = 0;
  }

  if(pal_cc_set_usb_ch(dev, mb)) {
    std::cerr << "Set CC usb channel failed" << std::endl;
  }
  return 0;
}

static int set_ep_usb_ch(uint8_t ch)
{
  uint8_t dev=0;
  uint8_t mb=0;

  if (ch >= EP_USB_CH_CNT)
    return -1;

  //Device is PCH
  if(ch >= EP_USB_PCH_MB0) {
    mb = ch - EP_USB_PCH_MB0;
    dev = 1;
  } else {
    mb = ch;
    dev = 0;
  }

  if(pal_ep_set_usb_ch(dev, mb)) {
    std::cerr << "Set EP usb channel failed" << std::endl;
  }
 
  return 0;
}

static std::set<std::string> get_bmcs()
{
  std::set<std::string> ret{};
  uint8_t mode;
  if (cmd_cmc_get_config_mode(&mode)) {
    return ret;
  }
  ret.insert("emeraldpools");
  if (mode > 4) {
    ret.insert("clearcreek");
    uint8_t pos;
    if (cmd_cmc_get_mb_position(&pos) || pos == 0) {
      ret.insert("tray1");
    } else {
      ret.insert("tray0");
    }
  }
  return ret;
}

static int get_mode()
{
  uint8_t def, cur, ch, jp;
  if (cmd_cmc_get_config_mode_ext(&def, &cur, &ch, &jp)) {
    return -1;
  }
  auto get_mode_desc = [](uint8_t md) {
    std::string desc = "Unknown mode: " + std::to_string(int(md));
    for (const auto &pair : mode_map) {
      if (md == pair.second.mode) {
        desc = pair.first + ": " + pair.second.desc;
        break;
      }
    }
    return desc;
  };

  std::cout << "Default: " << get_mode_desc(def) << std::endl;
  std::cout << "Current: " << get_mode_desc(cur) << std::endl;
  if (ch != 0xff) {
    std::cout << "Changed: " << get_mode_desc(ch) << std::endl;
  }
  const std::map<int, std::string> jumper_desc = {
    {0, "JP3"},
    {1, "JP4"},
    {2, "JP5"},
    {3, "JP6"},
    {4, "JP7"}
  };
  for (const auto& pair : jumper_desc) {
    bool st = jp & (1 << pair.first);
    std::cout << pair.second << ": " << st << std::endl;
  }
  return 0;
}

static int sled_cycle()
{
  int rc = 0;
  if (is_ep_present() && pal_ep_sled_cycle() < 0) {
    std::cerr << "Request JBOG sled-cycle failed\n";
    rc = -1;
  }
  if (cmd_cmc_sled_cycle()) {
    std::cerr << "Request chassis manager for a sled cycle failed\n";
    rc = -1;
  }
  return rc;
}

static int
get_pos() {
  uint8_t pos;
  if (cmd_cmc_get_mb_position(&pos)) {
    return -1;
  }
  std::cout << int(pos) << std::endl;
  return 0;
}

static int
reset_bmc(const std::string& bmc)
{
  if (bmc == "emeraldpools" && is_ep_present()) {
    int fd = i2c_cdev_slave_open(EP_I2C_BUS_NUMBER, 0x41, 0);
    if (fd < 0)
      return -1;
    int rc = i2c_smbus_write_byte_data(fd, 0x3, 0xfe);
    rc |= i2c_smbus_write_byte_data(fd, 0x3, 0xff);
    close(fd);
    return rc ? -1 : 0;
  }
  if (bmc == "clearcreek" && is_cc_present()) {
    // TODO Add correct CC IOExpander address.
    int fd = i2c_cdev_slave_open(CC_I2C_BUS_NUMBER, 0x77, 0);
    if (fd < 0)
      return -1;
    int rc = i2c_smbus_write_byte_data(fd, 0x3, 0xfe);
    rc |= i2c_smbus_write_byte_data(fd, 0x3, 0xff);
    close(fd);
    return rc ? -1 : 0;
  }
  const std::map<std::string, int> bmc_map = {
    {"tray0", 0},
    {"tray1", 1},
    {"tray2", 2},
    {"tray3", 3}
  };
  if (cmd_cmc_reset_bmc((uint8_t)bmc_map.at(bmc))) {
    return -1;
  }
  return 0;
}

static void
print_mac(uint8_t mac[6])
{
  std::cout << std::setfill('0') << std::setw(2) << std::right << std::hex << int(mac[0]);
  for (int i = 1; i < 6; i++) {
    std::cout << ":" << std::setfill('0') << std::setw(2) << std::right << std::hex << int(mac[i]);
  }
  std::cout << std::endl;
}

static void
print_linklocal(uint8_t mac[6])
{
  std::cout << std::setfill('0') << std::setw(2) << std::right << std::hex << 0xfe80 << "::";
  std::cout << std::setfill('0') << std::setw(2) << std::right << std::hex << int(mac[0] ^ 2) << int(mac[1]) << ":";
  std::cout << std::setfill('0') << std::setw(2) << std::right << std::hex << int(mac[2]) << 0xff << ":";
  std::cout << std::setfill('0') << std::setw(2) << std::right << std::hex << 0xfe << int(mac[3]) << ":";
  std::cout << std::setfill('0') << std::setw(2) << std::right << std::hex << int(mac[4]) << int(mac[5]) << std::endl;
}

static void
print_ip6_addr(uint8_t addr[SIZE_IP6_ADDR])
{
  uint16_t v = addr[1] | (addr[0] << 8);
  std::cout << std::hex << int(v);
  for (int i = 2; i < SIZE_IP6_ADDR; i+=2) {
    v = addr[i+1] | (addr[i] << 8);
    std::cout << ":" << std::hex << int(v);
  }
  std::cout << '\n';
}

static int
print_lan_info(const std::string& bmc)
{
  int (*get_lan_cfg)(uint8_t sel, uint8_t *buf, uint8_t *rlen);
  if (bmc == "emeraldpools") {
    get_lan_cfg = pal_ep_get_lan_config;
  } else if (bmc == "clearcreek") {
    get_lan_cfg = pal_cc_get_lan_config;
  } else if (bmc == "tray0" || bmc == "tray1") {
    get_lan_cfg = pal_peer_tray_get_lan_config;
  } else {
    std::cout << "Unsupported for " << bmc << std::endl;
    return -1;
  }

  uint8_t mac[6];
  uint8_t len;
  std::cout << "MAC: ";
  if (get_lan_cfg(LAN_PARAM_MAC_ADDR, mac, &len) || len != 6) {
    std::cout << "Unknown\n";
    std::cout << "Link-Local: Unknown\n";
  } else {
    print_mac(mac);
    std::cout << "Link-Local: ";
    print_linklocal(mac);
  }
  uint8_t addr[SIZE_IP6_ADDR];
  std::cout << "IP6 Address: ";
  if (get_lan_cfg(LAN_PARAM_IP6_ADDR, addr, &len) || len != SIZE_IP6_ADDR) {
    std::cout << "Unknown\n";
  } else {
    print_ip6_addr(addr);
  }
  return 0;
}

int
main(int argc, char **argv) {
  int rc = -1;
  bool do_cycle = false;
  CLI::App app("Chassis Management Utility");
  app.failure_message(CLI::FailureMessage::help);

//Set Mode Initial
  std::string op_mode;
  std::set<std::string> allowed_modes;
  std::string desc = "Set chassis operating mode\n";  
  for (const auto& pair : mode_map) {
    allowed_modes.insert(pair.first);
    desc += pair.first + ": " + pair.second.desc + "\n";
  }
  auto mode = app.add_set("--set-mode", op_mode,
                          allowed_modes,
                          desc
                         )->ignore_case();
//Set Usb Channel Initial
  std::string op_cc;
  std::set<std::string> allowed_cc;
  std::string desc_cc = "Set CC USB Channel\n";

  for (const auto& pair : cc_usb_ch_map) {
    allowed_cc.insert(pair.first);
    desc_cc += pair.first + ": " + pair.second.desc + "\n";
  }
  auto cc_usb_ch = app.add_set("--set-cc-usb", op_cc,
                          allowed_cc,
                          desc_cc
                         )->ignore_case();
                          
//Set Usb Channel Initial
  std::string op_ep;
  std::set<std::string> allowed_ep;
  std::string desc_ep = "Set EP USB Channel\n";

  for (const auto& pair : ep_usb_ch_map) {
    allowed_ep.insert(pair.first);
    desc_ep += pair.first + ": " + pair.second.desc + "\n";
  }
  auto ep_usb_ch = app.add_set("--set-ep-usb", op_ep,
                          allowed_ep,
                          desc_ep
                         )->ignore_case();

  app.add_flag("--sled-cycle", do_cycle,
               "Perform a SLED cycle after operations (If any)");
  app.require_option();
  bool get_mode_f = false;
  app.add_flag("--get-mode", get_mode_f, "Get Chassis Operating mode");

  bool get_pos_f = false;
  app.add_flag("--get-position", get_pos_f, "Get current tray position");

  std::string bmc_to_reset{};
  auto rstbmc = app.add_set("--reset-bmc", bmc_to_reset, get_bmcs(), "Reset BMC in the specified Tray");

  std::string bmc_to_get_lan{};
  auto laninfo = app.add_set("--get-lan", bmc_to_get_lan, get_bmcs(), "Get LAN/Network information of the given BMC");

  CLI11_PARSE(app, argc, argv);

  if (get_mode_f) {
    return get_mode();
  }

  if (get_pos_f) {
    return get_pos();
  }

  if (*rstbmc) {
    return reset_bmc(bmc_to_reset);
  }

  if (*laninfo) {
    return print_lan_info(bmc_to_get_lan);
  }

  if (*mode) {
    rc = set_mode(mode_map.at(op_mode).mode);
  }

  if (*cc_usb_ch) {
    rc = set_cc_usb_ch(cc_usb_ch_map.at(op_cc).channel);
  }

  if (*ep_usb_ch) {
    rc = set_ep_usb_ch(ep_usb_ch_map.at(op_ep).channel);
  }

  if (do_cycle) {
    rc = sled_cycle();
  }

  return rc;
}
