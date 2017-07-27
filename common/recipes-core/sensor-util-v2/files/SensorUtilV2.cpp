/*
 * Sensor Util V2 (DBus based)
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

#include <iostream>
#include <cstring>
#include <string>
#include <gio/gio.h>
#include <iomanip>

using namespace std;

#define SENSOR_SVC_DBUS_NAME "org.openbmc.SensorService"
#define SENSOR_SVC_BASE_PATH "/org/openbmc/SensorService"
#define SENSOR_SVC_SENSOR_TREE_INTERFACE "org.openbmc.SensorTree"
#define SENSOR_SVC_SENSOR_OBJECT_INTERFACE "org.openbmc.SensorObject"

#define MIN_SENSOR_NUM 1
#define MAX_SENSOR_NUM 255

//Helper function to check dbus erros
static void checkDBusErrorAndExit(GError* error) {
  if (error != nullptr) {
    if (error->code == G_DBUS_ERROR_SERVICE_UNKNOWN ) {
      cout << "Error: Sensor Service not available" << endl;
    }
    else {
      cout << "DBus error : " << error->message << endl;
    }
    exit(error->code);
  }
}

//Helper function to get DBus Proxy
static GDBusProxy* getDBusProxy(const char* path, const char* interface) {
  GError* error = nullptr;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_sync(
                      G_BUS_TYPE_SYSTEM,
                      G_DBUS_PROXY_FLAGS_NONE,
                      nullptr,
                      SENSOR_SVC_DBUS_NAME,
                      path,
                      interface,
                      nullptr,
                      &error);

  checkDBusErrorAndExit(error);

  return proxy;
}

/*
 * Returns available FRU names at sensor-svc
 */
static string getAvailableFruNames() {
  GVariant* response;
  const char* fruPath;
  GError* error = nullptr;
  GVariantIter* iter = nullptr;
  gchar* fruName;
  string fruNames;

  // Get proxy to sensor service
  GDBusProxy* proxy = getDBusProxy(SENSOR_SVC_BASE_PATH,
                                   SENSOR_SVC_SENSOR_TREE_INTERFACE);

  checkDBusErrorAndExit(error);

  // Get fru list
  response = g_dbus_proxy_call_sync(
      proxy,
      "getFRUList",
      nullptr,
      G_DBUS_CALL_FLAGS_NONE,
      -1,
      nullptr,
      &error);

  checkDBusErrorAndExit(error);

  // decode response
  fruNames = "all";
  g_variant_get(response, "(as)", &iter);
  if (iter != nullptr) {
    while (g_variant_iter_loop (iter, "&s", &fruName)) {
      fruNames = fruNames + ", " + fruName;
    }

    g_variant_iter_free (iter);
  }

  g_variant_unref(response);

  return fruNames;
}

// helper function to print errors in user command
static void printUsageAndExit(string error, int errNo) {
  string frulist = getAvailableFruNames();

  cout << "Usage: sensor-util-v2 [fru] <sensor-num>" << endl;
  cout << "       [fru]: " << frulist << endl;
  cout << "       <sensor num>: 0xXX (Omit [sensor num] means all sensors." << endl;

  if (!error.empty()) {
    cout << "Error: " << error << endl;
  }
  exit(errNo);
}

// helper function to print errors in excecution
static void printErrorExit(string error, int errNo) {
  cout << "Error: " << error << endl;
  exit(errNo);
}

/*
 * Parse sensor number from string strValue
 */
static int parseSensorNumber(const char* strValue) {
  int sensorNum;
  try {
    sensorNum = std::stoi (strValue, nullptr, 0);
  }
  catch(std::exception const & e)
  {
    printUsageAndExit("error : " + string(e.what()), -1);
  }

  if (sensorNum < MIN_SENSOR_NUM || sensorNum > MAX_SENSOR_NUM){
    printUsageAndExit("Invalid sensor number (1-255)", -1);
  }

  return sensorNum;
}

/*
 * Print Sensor Object
 */
static void printSensorObject(GVariant* sensorObject) {
  const gchar* name;
  gchar id;
  gint ret;
  gdouble sensorValue;
  const gchar* unit;

  //decode sensor object
  g_variant_get(sensorObject, "(syids)", &name, &id, &ret ,&sensorValue, &unit);

  //print SensorObject
  cout << std::left;
  cout << std::setw(45) << name;
  cout << "0x" << std::hex << std::setw(5) << (int)id << std::dec;
  if (ret != 0){
    cout << "NA" << std::endl;
  }
  else {
    cout << std::fixed << std::setprecision(2) << std::setw(10) << sensorValue;
    cout << unit << std::endl;
  }
}

/*
 * Print Sensor Object at sensorPath
 */
static void printSensor(const gchar* sensorPath) {
  GVariant* response;
  const gchar* name;
  gchar id;
  gint ret;
  gdouble sensorValue;
  const gchar* unit;

  GError* error = nullptr;

  // Get proxy to sensor
  GDBusProxy* proxy = getDBusProxy(sensorPath,
                                   SENSOR_SVC_SENSOR_OBJECT_INTERFACE);

  // Get sensor object
  response = g_dbus_proxy_call_sync(
      proxy,
      "getSensorObject",
      nullptr,
      G_DBUS_CALL_FLAGS_NONE,
      -1,
      nullptr,
      &error);

  checkDBusErrorAndExit(error);

  printSensorObject(response);
  g_variant_unref(response);
}

/*
 * Locate fru object with name 'fruName' under sensor service
 * and return path to fru object
 */
static char* getFruObjectPath(const char* fruName) {
  GVariant* response;
  char* fruPath;
  GError* error = nullptr;

  // Get proxy to Sensor Service
  GDBusProxy* proxy = getDBusProxy(SENSOR_SVC_BASE_PATH,
                                   SENSOR_SVC_SENSOR_TREE_INTERFACE);

  // Get fru path
  response = g_dbus_proxy_call_sync(
      proxy,
      "getFruPathByName",
      g_variant_new ("(&s)", fruName),
      G_DBUS_CALL_FLAGS_NONE,
      -1,
      nullptr,
      &error);

  checkDBusErrorAndExit(error);

  // extract sensorpath from response
  g_variant_get(response, "(s)", &fruPath);
  g_variant_unref(response);

  return fruPath;
}

int main(int argc, char const* argv[]) {
  GError* error = nullptr;

  int sensorNum = -1;

  // Argument validation
  if (argc < 2 || argc > 3)
  {
    printUsageAndExit("", -1);
    return 1;
  }

  // Parse sensorNumber
  if (argc == 3){
    sensorNum = parseSensorNumber(argv[2]);
  }

  //calculate dbuspath for fru object
  char* fruPath = SENSOR_SVC_BASE_PATH;
  if (strcmp(argv[1], "all") != 0) {
    // if user specified fru name, get fru object path
    fruPath = getFruObjectPath(argv[1]);

    if (fruPath[0] == '\0') {
      //fru argv[1] not found
      printUsageAndExit("Invalid FRU", -1);
    }
  }

  // Get proxy to fru object
  GDBusProxy* proxy = getDBusProxy(fruPath, SENSOR_SVC_SENSOR_TREE_INTERFACE);

  checkDBusErrorAndExit(error);

  if (sensorNum == -1){
    // Sensor number not given by user, print all sensors under fru
    // Get all sensor objects under given fru
    GVariant* response;
    const gchar* sensorPath;
    GVariantIter* iter = nullptr;

    // Get all sensor objects from FRU
    response = g_dbus_proxy_call_sync(
        proxy,
        "getSensorObjects",
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error);
    checkDBusErrorAndExit(error);

    g_variant_get(response, "(a(syids))", &iter);
    if (iter != nullptr) {
      GVariant* sensorObject;
      while ((sensorObject = g_variant_iter_next_value (iter))) {
        printSensorObject(sensorObject);
      }

      g_variant_iter_free (iter);
    }
    g_variant_unref(response);
  }
  else {
    // Get sensor path based on sensorNum
    GVariant* response;
    GVariant* param = g_variant_new("(y)", sensorNum);
    const gchar* sensorPath;

    // Get sensorpath from FRU object
    response = g_dbus_proxy_call_sync(
        proxy,
        "getSensorPathById",
        param,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error);
    checkDBusErrorAndExit(error);

    // extract sensorpath from response
    g_variant_get(response, "(&s)", &sensorPath);

    //Check if sensor is located under FRU object
    if (sensorPath[0] == '\0') {
      printErrorExit("Sensor " + string(argv[2]) +
                     " not found under FRU " + string(argv[1]), -1);
    }

    printSensor(sensorPath);
    g_variant_unref(response);
  }

  return 0;
}
