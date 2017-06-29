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

// helper function to print errors in user command
static void printUsageAndExit(string error, int errNo) {
  cout << "Usage: sensor-util2 [ all/fru-name ] <sensor-num>\n";
  if (!error.empty()) {
    cout << "Error: " << error << "\n";
  }
  exit(errNo);
}

// helper function to print errors in excecution
static void printErrorExit(string error, int errNo) {
  cout << "Error: " << error << "\n";
  exit(errNo);
}

void checkDBusErrorAndExit(GError *error) {
  if (error != NULL) {
    cout << "DBus error : " << error->message << "\n";
    exit(error->code);
  }
}

//Helper function to get DBus Proxy
GDBusProxy* getDBusProxy(const char* path, const char* interface) {
  GError* error = NULL;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_sync(
                      G_BUS_TYPE_SYSTEM,
                      G_DBUS_PROXY_FLAGS_NONE,
                      NULL,
                      SENSOR_SVC_DBUS_NAME,
                      path,
                      interface,
                      NULL,
                      &error);

  checkDBusErrorAndExit(error);

  return proxy;
}

int parseSensorNumber(const char* strValue) {
  int sensorNum;
  try {
    if ((strlen(strValue) > 2) && (strValue[0] == '0' and strValue[1] == 'x')){
      //hex String
      sensorNum = std::stoi (strValue, nullptr, 16);
    }
    else {
      sensorNum = std::stoi (strValue);
    }
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

void printSensorValue(const gchar* sensorPath) {
  GVariant *response;
  const gchar* name;
  gchar id;
  gint ret;
  gdouble sensorValue;
  const gchar* unit;

  GError *error = NULL;

  // Get proxy to sensor object
  GDBusProxy* proxy = getDBusProxy(sensorPath, SENSOR_SVC_SENSOR_OBJECT_INTERFACE);

  // Get sensor value from Sensor
  response = g_dbus_proxy_call_sync(
      proxy,
      "org.openbmc.SensorObject.getSensorObject",
      NULL,
      G_DBUS_CALL_FLAGS_NONE,
      -1,
      NULL,
      &error);

  checkDBusErrorAndExit(error);

  // extract sensorpath from response
  g_variant_get(response, "(syids)", &name, &id, &ret ,&sensorValue, &unit);

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

  g_variant_unref(response);
}

char* getFruObjectPath(const char* fruName) {
  GVariant *response;
  char* fruPath;
  GError *error = NULL;

  // Get proxy to sensor object
  GDBusProxy* proxy = getDBusProxy(SENSOR_SVC_BASE_PATH, SENSOR_SVC_SENSOR_TREE_INTERFACE);

  // Get fru path
  response = g_dbus_proxy_call_sync(
      proxy,
      "org.openbmc.SensorTree.getFruPathByName",
      g_variant_new ("(&s)", fruName),
      G_DBUS_CALL_FLAGS_NONE,
      -1,
      NULL,
      &error);

  checkDBusErrorAndExit(error);

  // extract sensorpath from response
  g_variant_get(response, "(s)", &fruPath);
  g_variant_unref(response);

  return fruPath;
}

string getAvailableFruList() {
  GVariant *response;
  const char* fruPath;
  GError *error = NULL;
  GVariantIter *iter = NULL;
  gchar *fruName;
  string fruList;

  // Get proxy to sensor service
  GDBusProxy* proxy = getDBusProxy(SENSOR_SVC_BASE_PATH, SENSOR_SVC_SENSOR_TREE_INTERFACE);

  checkDBusErrorAndExit(error);

  // Get fru path
  response = g_dbus_proxy_call_sync(
      proxy,
      "org.openbmc.SensorTree.getFRUList",
      NULL,
      G_DBUS_CALL_FLAGS_NONE,
      -1,
      NULL,
      &error);

  checkDBusErrorAndExit(error);

  // print available fru list
  fruList = "[ all ";
  g_variant_get(response, "(as)", &iter);
  if (iter != NULL) {
    while (g_variant_iter_loop (iter, "&s", &fruName))
      fruList = fruList + fruName + " ";

    g_variant_iter_free (iter);
  }

  g_variant_unref(response);

  fruList += "]";
  return fruList;
}

int main(int argc, char const *argv[]) {
  GError *error = NULL;

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
      printErrorExit("Fru " + string(argv[1]) + "does not exists, Available FRU :" + getAvailableFruList(), -1);
    }
  }

  // Get proxy to fru object
  GDBusProxy* proxy = getDBusProxy(fruPath, SENSOR_SVC_SENSOR_TREE_INTERFACE);

  checkDBusErrorAndExit(error);

  if (sensorNum == -1){
    // Sensor number not given by user, print all sensors under fru
    //Get sensorList under given fru
    // Get sensor path based on sensorNum
    GVariant *response;
    const gchar *sensorPath;

    // Get sensorpath from FRU object
    response = g_dbus_proxy_call_sync(
        proxy,
        "org.openbmc.SensorTree.getSensorObjectPaths",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);
    checkDBusErrorAndExit(error);

    GVariantIter *iter = NULL;
    g_variant_get(response, "(as)", &iter);
    if (iter != NULL) {
      while (g_variant_iter_loop (iter, "&s", &sensorPath)) {
        printSensorValue(sensorPath);
      }

      g_variant_iter_free (iter);
    }
    g_variant_unref(response);
  }
  else {
    // Get sensor path based on sensorNum
    GVariant *response;
    GVariant* param = g_variant_new("(y)", sensorNum);
    const gchar *sensorPath;

    // Get sensorpath from FRU object
    response = g_dbus_proxy_call_sync(
        proxy,
        "org.openbmc.SensorTree.getSensorPathById",
        param,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);
    checkDBusErrorAndExit(error);

    // extract sensorpath from response
    g_variant_get(response, "(&s)", &sensorPath);

    //Check if sensor is located under FRU object
    if (sensorPath[0] == '\0') {
      printErrorExit("Sensor " + string(argv[2]) + " not found under FRU " + string(argv[1]), -1);
    }

    printSensorValue(sensorPath);
    g_variant_unref(response);
  }

  return 0;
}
