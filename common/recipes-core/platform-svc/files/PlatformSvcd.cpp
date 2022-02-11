/*
 * PlatformSvcd.cpp
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

#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <gio/gio.h>
#include <dbus-utils/DBus.h>
#include <dbus-utils/dbus-interface/DBusObjectInterface.h>
#include "PlatformObjectTree.h"
#include "PlatformJsonParser.h"
#include "SensorService.h"
#include "FruService.h"
using namespace openbmc::qin;

// validator for the json filename
static bool validateFilename(const char* flagname,
                             const std::string &filename) {
  std::ifstream ifs(filename);
  if (ifs.good()) { // file is good
    return true;
  }
  printf("Invalid value for --%s: %s\n", flagname, filename.c_str());
  return false;
}

// PlatformSvcd takes json file name to parse the platform tree
DEFINE_string(json, "",
              "JSON file name to be parsed under the object \"/org/openbmc\"");

static const bool regDummy =
  ::gflags::RegisterFlagValidator(&FLAGS_json, &validateFilename);

// implementation for handling DBus request messages
static DBusObjectInterface objectInterface;

// event handler for DBus request messages
static void eventLoop(GMainLoop* loop) {
  LOG(INFO) << "Event loop begins";
  g_main_loop_run(loop);
  LOG(INFO) << "Event loop ends";
}

//Callback for DBus Name Lost
void platformSvcdSvcdOnDbusNameLost() {
  LOG(ERROR) << "DBus Name lost, exiting";
  exit(-1);
}

// Callback for SensorService dbus name known to exist
static void sensorServiceOnNameAppeared (GDBusConnection *connection,
                                         const gchar     *name,
                                         const gchar     *name_owner,
                                         gpointer         user_data)
{
  LOG(INFO) << name <<" is Up";
  PlatformObjectTree* platformTree = (PlatformObjectTree*)user_data;

  //Mark SensorService available and reset SensorService
  platformTree->setSensorServiceAvailable(true);
}

// Callback for SensorService dbus name known to not exist
static void sensorServiceOnNameVanished (GDBusConnection *connection,
                                         const gchar     *name,
                                         gpointer         user_data)
{
  LOG(INFO) << name << " is down";
  PlatformObjectTree* platformTree = (PlatformObjectTree*)user_data;
  // Mark SensorService unavailable
  platformTree->setSensorServiceAvailable(false);
}

// Callback for FruService dbus name known to exist
static void fruServiceOnNameAppeared (GDBusConnection *connection,
                                      const gchar     *name,
                                      const gchar     *name_owner,
                                      gpointer         user_data)
{
  LOG(INFO) << name <<" is Up";
  PlatformObjectTree* platformTree = (PlatformObjectTree*)user_data;

  //Mark FruService available and reset FruService
  platformTree->setFruServiceAvailable(true);
}

// Callback for FruService dbus name known to not exist
static void fruServiceOnNameVanished (GDBusConnection *connection,
                                         const gchar     *name,
                                         gpointer         user_data)
{
  LOG(INFO) << name << " is down";
  PlatformObjectTree* platformTree = (PlatformObjectTree*)user_data;
  // Mark FruService unavailable
  platformTree->setFruServiceAvailable(false);
}

/*
 * Monitors hotplug supported frus every 5 seconds
 */
static void hotPlugMonitor(PlatformObjectTree* platformTree) {
  LOG(INFO) << "hotPlugMonitor started";

  if (platformTree->getNofHPIntDetectSupportedFrus() > 0) {
    while (true) {
      //Check for hot plug supported frus
      platformTree->checkHotPlugSupportedFrus();

      //Sleep for 5 seconds
      std::this_thread::sleep_for(std::chrono::seconds(5));
    }
  }
  else {
    LOG(INFO) << "No Fru Supports internal hotplug detection, "
                 "exiting hotPlugMonitor";
  }
}

int main (int argc, char* argv[]) {
  ::google::InitGoogleLogging(argv[0]);
  ::gflags::ParseCommandLineFlags(&argc, &argv, true);

  const std::string dbusName = "org.openbmc.PlatformService";
  LOG(INFO) << "Registering for DBus name \""<< dbusName <<"\" with interface "
            << objectInterface.getName();

  std::shared_ptr<DBus> sDbus =
    std::shared_ptr<DBus>(new DBus(dbusName, &objectInterface));
  DBus &dbus = *(sDbus.get());
  // Register callback for DBus Name Lost
  dbus.onConnLost = platformSvcdSvcdOnDbusNameLost;

  GMainLoop *loop;
  std::thread t;

  LOG(INFO) << "Connecting the platform-svcd to the system DBus daemon";
  dbus.registerConnection();

  LOG(INFO) << "Setting up the event loop and waiting for the connection";
  loop = g_main_loop_new(nullptr, FALSE);
  t = std::thread(eventLoop, loop);
  dbus.waitForConnection();

  LOG(INFO) << "Creating platform tree" << std::endl;
  PlatformObjectTree platformTree(sDbus, "org");

  platformTree.addObject("openbmc","/org");

  LOG(INFO) << "Parsing \"" << FLAGS_json << "\" into the sensor tree";
  PlatformJsonParser::parse(FLAGS_json, platformTree, "/org/openbmc");

  LOG(INFO) << "Main thread joining the event loop thread";

  //Watch SensorService dbus name
  guint watcher_sensor_svc_id =
      g_bus_watch_name (G_BUS_TYPE_SYSTEM,
                        platformTree.getSensorService()->getDBusName().c_str(),
                        G_BUS_NAME_WATCHER_FLAGS_NONE,
                        sensorServiceOnNameAppeared,
                        sensorServiceOnNameVanished,
                        &platformTree,
                        nullptr);

  //Watch FruService dbus name
  guint watcher_fru_svc_id =
      g_bus_watch_name (G_BUS_TYPE_SYSTEM,
                        platformTree.getFruService()->getDBusName().c_str(),
                        G_BUS_NAME_WATCHER_FLAGS_NONE,
                        fruServiceOnNameAppeared,
                        fruServiceOnNameVanished,
                        &platformTree,
                        nullptr);

  //Start a thread for monitoring hotplug supported frus
  std::thread hotPlugMonitorThread(hotPlugMonitor, &platformTree);

  t.join();

  LOG(INFO) << "Quitting the event loop";
  g_bus_unwatch_name(watcher_sensor_svc_id);
  g_bus_unwatch_name(watcher_fru_svc_id);
  g_main_loop_quit(loop);
  g_main_loop_unref(loop);

  LOG(INFO) << "Unregistering the connection to the system DBus";
  dbus.unregisterConnection(); // unregister all objects
  return 0;
}
