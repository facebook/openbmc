/*
 * Copyright 2014-present Facebook. All Rights Reserved.
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

#include <cstdio>
#include <iostream>
#include <regex>
#include <string>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <gio/gio.h>
#include <nlohmann/json.hpp>
#include "DBus.h"
using namespace openbmc::qin;

// validator for dbus_name and interface_name
static bool validateName(const char* flagname, const std::string &name) {
  if (std::regex_match(name, DBus::dbusNameRegex)) {
    return true;
  }
  printf("Invalid value for --%s: %s\n", flagname, name.c_str());
  return false;
}

// validator for object path
static bool validatePath(const char* flagname, const std::string &path) {
  if (std::regex_match(path, DBus::pathRegex)) {
    return true;
  }
  printf("Invalid value for --%s: %s\n", flagname, path.c_str());
  return false;
}

DEFINE_bool(system, true, "To connect to the system bus or not;"
                          "If false, the session bus will be connected");

DEFINE_bool(r, false, "Recursively dump the child objects");

DEFINE_bool(t, false, "Dump the whole tree recursively");

DEFINE_string(dbus_name, "org.openbmc.Sensord", "DBus name to be connected");

DEFINE_string(object_path, "/org/openbmc/Chassis",
              "DBus object to be queried");

DEFINE_string(interface_name, "org.openbmc.Object",
              "Interface name that contains the methods to be called");

static const bool dbusNameDummy =
  ::gflags::RegisterFlagValidator(&FLAGS_dbus_name, &validateName);

static const bool interfaceNameDummy =
  ::gflags::RegisterFlagValidator(&FLAGS_interface_name, &validateName);

static const bool pathDummy =
  ::gflags::RegisterFlagValidator(&FLAGS_object_path, &validatePath);

int main (int argc, char* argv[]) {
  ::google::InitGoogleLogging(argv[0]);
  ::gflags::ParseCommandLineFlags(&argc, &argv, true);

  GError* error = nullptr;
  GBusType busType = FLAGS_system? G_BUS_TYPE_SYSTEM: G_BUS_TYPE_SESSION;
  LOG(INFO) << "Registering the dbus proxy at \n" <<
               "dbus type: "      << (FLAGS_system? "system\n": "session\n") <<
               "dbus name: "      << FLAGS_dbus_name   << "\n" <<
               "object path: "    << FLAGS_object_path << "\n" <<
               "interface name: " << FLAGS_interface_name;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_sync(
                        busType,
                        G_DBUS_PROXY_FLAGS_NONE,
                        NULL,
                        FLAGS_dbus_name.c_str(),
                        FLAGS_object_path.c_str(),
                        FLAGS_interface_name.c_str(),
                        NULL,
                        &error);
  if (proxy == nullptr) {
    LOG(ERROR) << "Cannot register the dbus proxy: " << error->message;
    return 1;
  }

  std::string method_name = "org.openbmc.Object.dumpByObject";
  if (FLAGS_r) {
    method_name = "org.openbmc.Object.dumpRecursiveByObject";
  }
  if (FLAGS_t) {
    method_name = "org.openbmc.Object.dumpTree";
  }

  LOG(INFO) << "Invoking the method " << method_name;
  error = nullptr;
  GVariant* result = g_dbus_proxy_call_sync(proxy,
                                            method_name.c_str(),
                                            NULL,
                                            G_DBUS_CALL_FLAGS_NONE,
                                            -1,
                                            NULL,
                                            &error);
  if (result == nullptr) {
    LOG(ERROR) << "Error invoking the method " << method_name << ": "
      << error->message;
    return 1;
  }

  const char* cdump;
  g_variant_get(result, "(&s)", &cdump);
  nlohmann::json jsonDump = nlohmann::json::parse(std::string(cdump));
  LOG(INFO) << "Printing the dump result.";
  std::cout << jsonDump.dump(2) << std::endl;

  return 0;
}
