/*
 * DBusPrintObjectTree.cpp
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

#include <gio/gio.h>
#include <iostream>
#include <vector>

#define DBUS_INTROSPECT_INTERFACE "org.freedesktop.DBus.Introspectable"
#define DBUS_METHOD_INTROSPECT "Introspect"

using namespace std;

// helper function to print errors in excecution and exit
static void printErrorExit(const string & error, int errNo) {
  cout << "Error: " << error << endl;
  exit(errNo);
}

// helper function to print Usage
static void printUsageAndExit(int errNo) {
  cout << "Usage: dbus-print-objecttree [dbus-name] [dbus-object-path]"
       << endl;
  exit(errNo);
}

//Helper function to get DBus Proxy
static GDBusProxy* getDBusProxy(const char* dbusName,
                                const char* path,
                                const char* interface) {
  GError* error = nullptr;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_sync(
                      G_BUS_TYPE_SYSTEM,
                      G_DBUS_PROXY_FLAGS_NONE,
                      nullptr,
                      dbusName,
                      path,
                      interface,
                      nullptr,
                      &error);

  if (error != nullptr) {
    printErrorExit(error->message, -1);
  }
  return proxy;
}

/**
 * This function returns introspection xml data of dbus object
 */
static string getIntrospectionXml(const string & dbusName,
                                  const string & path) {
  const gchar *xml_data;
  string xmlStr;
  GError *error = nullptr;
  GDBusProxy* proxy;
  GVariant *response;

  proxy = getDBusProxy(dbusName.c_str(),
                       path.c_str(),
                       DBUS_INTROSPECT_INTERFACE);

  response = g_dbus_proxy_call_sync(
      proxy,
      DBUS_METHOD_INTROSPECT,
      nullptr,
      G_DBUS_CALL_FLAGS_NONE,
      -1,
      nullptr,
      &error);

  if (error != nullptr) {
    printErrorExit("Could not connect to Service " + dbusName + " " + path, -1);
  }

  //Get xml data from response
  g_variant_get (response, "(&s)", &xml_data);
  xmlStr = string(xml_data);

  g_variant_unref(response);
  g_object_unref(proxy);

  return xmlStr;
}


/**
 * This function decodes xml data and returns vector of object Names
 * This functions decodes <node name="XXXX"/> from xmlStr
 */
static vector<string> getObjectNamesFromXml(const string & xmlStr) {
  vector<string> list;
  //Pattern to locate start of object Name
  const string & startPattern = "<node name=\"";
  //Pattern to locate end of object Name
  const string & endPattern = "\"/>";

  //Get position first node tag
  int pos = xmlStr.find(startPattern);

  while (pos != string::npos) {
    //Calculate start position of object name
    pos += startPattern.length();
    //Get end position of object name
    int endpos = xmlStr.find(endPattern, pos);

    //Add object name to vector
    list.push_back(xmlStr.substr(pos, endpos - pos));

    //Position of next node tag
    pos = xmlStr.find("<node name=\"", endpos);
  }

  return list;
}

/**
 * This function recursively traverse object tree at dbusName and path basepath
 * Prints path to each object on console
 */
static void printObjectTree(const string & dbusName, const string & basepath) {

  //Get list of objects under basepath
  vector<string> list = getObjectNamesFromXml(getIntrospectionXml(dbusName,
                                                                  basepath));

  for (auto &it : list) {
    //Print child object
    cout << (basepath + "/" + it) << endl;

    //Print list of all objects under child object
    printObjectTree(dbusName, basepath + "/" + it);
  }
}

/**
 * Entry point for dbus-print-objectree
 * Usage: "Usage: dbus-print-objecttree [dbus-name] [dbus-object-path]"
 */
int main (int argc, char *argv[]) {
  //argument validation
  if (argc != 3) {
    printUsageAndExit(-1);
  }

  //print object tree
  //argv[1] - dbusName
  //argv[2] - basepath
  printObjectTree(argv[1], argv[2]);

  return 0;
}
