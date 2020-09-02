/*
 * mac-util
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
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <CLI/CLI.hpp>

#define MB_EEPROM "/sys/class/i2c-dev/i2c-6/device/6-0054/eeprom"
#define MAC_LEN 6
#define MAC_OFFSET 1024

using namespace std;

static int _set_mac(unsigned char *addr)
{
  int ret = -1;
  ofstream f(MB_EEPROM, ios::out | ios::binary);
  if (f) {
    f.seekp(MAC_OFFSET, ios::beg);
    if (!f.good())
      goto exit;
    f.write((char*)addr, MAC_LEN);
    if (!f.good())
      goto exit;
  } else {
    cerr << "Failed to open MB EEPROM" << endl;
    return -1;
  }

  ret = 0;
exit:
  f.close();
  return ret;
}

static int set_mac(string str)
{
  vector<string> sv;
  sv.clear();
  istringstream iss(str);
  string temp;
  unsigned char addr[MAC_LEN] = {0};

  while (getline(iss, temp, ':'))
    sv.emplace_back(move(temp));

  if (sv.size() != MAC_LEN)
    goto err;

  for (int i = 0; i < MAC_LEN; i++) {
    unsigned long byte = stoul(sv[i], NULL, 16);
    if (byte > 0xff)
      goto err;

    addr[i] = byte;
  }
  return _set_mac(addr);
err:
  cerr << "Invalid MAC address" << endl;
  return -1;
}

static int _show_mac()
{
  int ret = -1;
  unsigned char addr[MAC_LEN] = {0};
  ifstream f(MB_EEPROM, ios::in | ios::binary);
  if (f) {
    f.seekg(MAC_OFFSET, ios::beg);
    if (!f.good())
      goto exit;
    f.read((char*)addr, MAC_LEN);
    if (!f.good())
      goto exit;
    cout.setf(ios::right, std::ios::adjustfield);
    cout.setf(cout.hex , cout.basefield);
    cout << setfill('0') << setw(2) << (int)addr[0] << ":"
         << setfill('0') << setw(2) << (int)addr[1] << ":"
         << setfill('0') << setw(2) << (int)addr[2] << ":"
         << setfill('0') << setw(2) << (int)addr[3] << ":"
         << setfill('0') << setw(2) << (int)addr[4] << ":"
         << setfill('0') << setw(2) << (int)addr[5] << endl;
  } else {
    cerr << "Failed to open MB EEPROM" << endl;
    return -1;
  }

  ret = 0;
exit:
  f.close();
  return ret;
}

int main(int argc, char **argv)
{
  int rc = -1;
  CLI::App app("MAC Utility");
  app.failure_message(CLI::FailureMessage::help);

  string addr;
  auto set = app.add_option("--set", addr,
                            "Set MAC address\n"
			    "Example: 02:00:00:00:00:01");
  bool show_mac = false;
  app.add_flag("--get", show_mac, "Get MAC address");

  app.require_option();
  CLI11_PARSE(app, argc, argv);

  if (*set) {
    rc = set_mac(addr);
    if (rc < 0)
      cerr << "Failed to set MAC address" << endl;
    else
      cout << "Set MAC address to " << addr << endl;
  }

  if (show_mac) {
    rc = _show_mac();
    if (rc < 0)
      cerr << "Failed to get MAC address" << endl;
  }

  return rc;
}
