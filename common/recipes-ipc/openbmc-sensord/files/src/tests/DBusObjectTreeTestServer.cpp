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

#include <memory>
#include <stdexcept>
#include <thread>
#include <glog/logging.h>
#include <gio/gio.h>
#include <ipc-interface/Ipc.h>
#include <dbus-utils/DBus.h>
#include <dbus-utils/DBusInterfaceBase.h>
#include <object-tree/ObjectTree.h>
#include <object-tree/Object.h>
#include <object-tree/Attribute.h>
#include "DBusObjectTreeInterface.h"
using namespace openbmc::qin;

static DBusObjectTreeInterface testInterface;

void eventLoop(GMainLoop* loop) {
  LOG(INFO) << "Event loop begins";
  g_main_loop_run(loop);
  LOG(INFO) << "Event loop ends";
}

int main (int argc, char* argv[]) {
  ::google::InitGoogleLogging(argv[0]);
  std::shared_ptr<DBus> sDbus(new DBus("org.openbmc.Chassis", &testInterface));
  DBus &dbus = *(sDbus.get());
  GMainLoop *loop;
  std::thread t;

  dbus.registerConnection();
  loop = g_main_loop_new(nullptr, FALSE);
  t = std::thread(eventLoop, loop);
  dbus.waitForConnection();

  // the testInterface is inserted as default interface
  ObjectTree obTree(sDbus, "org");
  obTree.addObject("openbmc", "/org");
  Object* chassis = obTree.addObject("Chassis", "/org/openbmc");
  obTree.addObject("Systems", "/org/openbmc");
  obTree.addObject("Managers", "/org/openbmc");
  // the above objects can be pinged through DBusObjectTreeInterface

  chassis->addAttribute("1_input");
  chassis->addAttribute("1_max");
  chassis->addAttribute("1_min");

  t.join(); // let the event loop run forever
  g_main_loop_quit(loop);
  g_main_loop_unref(loop);
  dbus.unregisterConnection();
  return 0;
}
