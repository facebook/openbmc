/*
 * FruSvcd.cpp
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
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

#include <fstream>
#include <string>
#include <thread>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <gio/gio.h>
#include <dbus-utils/DBus.h>
#include <dbus-utils/dbus-interface/DBusObjectInterface.h>
#include "FruObjectTree.h"
using namespace openbmc::qin;

// implementation for handling DBus request messages
static DBusObjectInterface objectInterface;

// event handler for DBus request messages
static void eventLoop(GMainLoop* loop) {
  LOG(INFO) << "Event loop begins";
  g_main_loop_run(loop);
  LOG(INFO) << "Event loop ends";
}

//Callback for DBus Name Lost
void fruSvcdOnDbusNameLost() {
  LOG(ERROR) << "DBus Name lost, exiting";
  exit(-1);
}

int main (int argc, char* argv[]) {
  ::google::InitGoogleLogging(argv[0]);
  ::gflags::ParseCommandLineFlags(&argc, &argv, true);

  const std::string dbusName = "org.openbmc.FruService";
  LOG(INFO) << "Registering for DBus name \""<< dbusName <<"\" with interface "
            << objectInterface.getName();

  std::shared_ptr<DBus> sDbus =
    std::shared_ptr<DBus>(new DBus(dbusName, &objectInterface));
  DBus &dbus = *(sDbus.get());
  // Register callback for DBus Name Lost
  dbus.onConnLost = fruSvcdOnDbusNameLost;

  GMainLoop *loop;
  std::thread t;

  LOG(INFO) << "Connecting the fru-svcd to the system DBus daemon" << std::endl;
  dbus.registerConnection();

  LOG(INFO) << "Setting up the event loop and waiting for the connection" << std::endl;
  loop = g_main_loop_new(nullptr, FALSE);
  t = std::thread(eventLoop, loop);
  dbus.waitForConnection();

  LOG(INFO) << "Creating fru tree" << std::endl;
  FruObjectTree fruTree(sDbus, "org");

  fruTree.addObject("openbmc","/org");
  fruTree.addFruService("FruService", "/org/openbmc");

  LOG(INFO) << "Main thread joining the event loop thread" << std::endl;
  t.join();

  LOG(INFO) << "Quitting the event loop" << std::endl;
  g_main_loop_quit(loop);
  g_main_loop_unref(loop);

  LOG(INFO) << "Unregistering the connection to the system DBus" << std::endl;
  dbus.unregisterConnection(); // unregister all objects
  return 0;
}
