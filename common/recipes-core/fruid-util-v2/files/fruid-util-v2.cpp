/*
 * fruid-util-v2 (DBus based)
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

#include <cstring>
#include <fstream>
#include <gio/gio.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <unistd.h>
#include <vector>

#define FRU_SVC_DBUS_NAME "org.openbmc.FruService"
#define FRU_SVC_BASE_PATH "/org/openbmc/FruService"
#define FRU_SVC_FRU_OBJECT_INTERFACE "org.openbmc.FruObject"
#define FRU_SVC_METHOD_GET_FRUID_INFO "getFruIdInfo"
#define FRU_SVC_METHOD_FRUID_WRITE_BIN_DATA "fruIdWriteBinaryData"
#define FRU_SVC_METHOD_FRUID_DUMP_BIN_DATA "fruIdDumpBinaryData"

#define DBUS_INTROSPECT_INTERFACE "org.freedesktop.DBus.Introspectable"
#define DBUS_METHOD_INTROSPECT "Introspect"

using namespace std;

// helper function to print errors in excecution and exit
static void printErrorExit(string error, int errNo) {
  cout << "Error: " << error << std::endl;
  exit(errNo);
}

// helper function to print dbus errors in excecution and exit
static void checkDBusErrorAndExit(GError *error) {
  if (error != nullptr) {
    cout << "DBus error : " << error->message << "\n";
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
                      FRU_SVC_DBUS_NAME,
                      path,
                      interface,
                      nullptr,
                      &error);

  checkDBusErrorAndExit(error);
  return proxy;
}

/**
 * This function returns introspection xml data of fru-svc dbus object at path
 */
static string getIntrospectionXml(const string & path) {
  const gchar *xml_data;
  string xmlStr;
  GError *error = nullptr;
  GDBusProxy* proxy;
  GVariant *response;

  proxy = getDBusProxy(path.c_str(), DBUS_INTROSPECT_INTERFACE);
  response = g_dbus_proxy_call_sync(
      proxy,
      DBUS_METHOD_INTROSPECT,
      nullptr,
      G_DBUS_CALL_FLAGS_NONE,
      -1,
      nullptr,
      &error);

  if (error != nullptr) {
    printErrorExit("Fru Service not available", -1);
  }

  //Get xml data from response
  g_variant_get (response, "(&s)", &xml_data);
  xmlStr = string(xml_data);

  g_variant_unref(response);
  g_object_unref(proxy);

  return xmlStr;
}

/**
 * This function decodes xml data and returns vector of fru Names
 * This functions decodes <node name="XXXX"/> from xmlStr
 */
static vector<string> getFruNamesFromXml(const string & xmlStr) {
  vector<string> list;
  //Pattern to locate start of fru Name
  const string & startPattern = "<node name=\"";
  //Pattern to locate end of fru Name
  const string & endPattern = "\"/>";

  //Get position first node tag
  size_t pos = xmlStr.find(startPattern);

  while (pos != std::string::npos) {
    //Calculate start position of fruName
    pos += startPattern.length();
    //Get end position of fruName
    int endpos = xmlStr.find(endPattern, pos);

    //Add fru name to vector
    list.push_back(xmlStr.substr(pos, endpos - pos));

    //Position of next node tag
    pos = xmlStr.find("<node name=\"", endpos);
  }

  return list;
}


/**
 * This function recursively traverse frutree at basepath and returns fruNames
 */
static vector<string> getFruNames(string basepath) {

  //Get list of frus under basepath
  vector<string> list = getFruNamesFromXml(getIntrospectionXml(basepath));

  for (auto &it : list) {
    //Get list of frus under child fru
    vector<string> childfrulist = getFruNames(basepath + "/" + it);

    for (auto &itchild : childfrulist) {
      list.push_back(itchild);
    }
  }

  return list;
}

// helper function to print Usage
static void printUsageAndExit(int errNo) {

  //Get fru names from fru-svc
  vector<string> list = getFruNames(FRU_SVC_BASE_PATH);

  if (list.empty()){
    cout << "Fru Service not available" << endl;
  }
  else {
    //Build string of available fru names
    string fruNames = list.front();
    list.erase(list.begin());
    for (auto & it: list) {
      fruNames += " , " + it;
    }

    cout << "Usage: fruid-util-v2 [ all ," << fruNames << " ]" << endl;
    cout << "Usage: fruid-util-v2 [ " << fruNames << " ] [--dump | --write] <file>" << endl;
  }

  exit(errNo);
}

// helper function to output row in two columns
static void printRow(const char* str1 , const char* str2) {
  cout << std::left;
  cout << std::setw(30) << str1;
  cout << " : ";
  cout << str2 << std::endl;
}

/**
 * This gets fruId information of fru at fruPath from fru-svc over dbus
 * and prints fruid information on console
 */
static void printFruIdInfo(const std::string & fruPath) {
  GVariant *response;
  GVariantIter *iter = nullptr;
  GError *error = nullptr;

  // Get proxy to fru object
  GDBusProxy* proxy = getDBusProxy(fruPath.c_str(), FRU_SVC_FRU_OBJECT_INTERFACE);

  // Get fruid information from fru
  response = g_dbus_proxy_call_sync(
      proxy,
      FRU_SVC_METHOD_GET_FRUID_INFO,
      nullptr,
      G_DBUS_CALL_FLAGS_NONE,
      -1,
      nullptr,
      &error);

  checkDBusErrorAndExit(error);

  // extract fruid info from response
  g_variant_get(response, "(a{ss})", &iter);

  printRow("---------------------", "---------------------");
  // Get position of fruName in fruPath
  int pos = fruPath.find_last_of("/");
  printRow("FRU information", fruPath.substr(pos, fruPath.length() - pos).c_str());
  printRow("---------------------", "---------------------");

  //print FRUID information
  if (iter != nullptr) {
    const gchar* key = nullptr;
    const gchar* value = nullptr;
    if (g_variant_iter_n_children(iter) == 0)
    {
      cout << "NA" << endl;
    }
    else{
      while (g_variant_iter_loop (iter, "{ss}", &key, &value)) {
        printRow(key, value);
      }
    }
    g_variant_iter_free (iter);
  }
  else{
    cout << "NA" << endl;
  }

  g_variant_unref(response);
  g_object_unref(proxy);
}

/**
 * This is recursive function to locate fruName under fru-svc tree at parent
 * Returns path to fruName if located successfully else returns empty string
 */
static std::string getFruPath(const std::string & parent, const std::string & fruName) {
  //Empty string
  string path;
  //Get list of child fru names
  vector<string> list = getFruNamesFromXml(getIntrospectionXml(parent));

  //Return path if fru name matches with child fru name
  for (auto & it: list) {
    if (it.compare(fruName) == 0) {
      return parent + "/" + it;
    }
  }

  //Search fruName under child frus
  for (auto & it: list) {
    path = getFruPath(parent + "/" + it, fruName);
    if (!path.empty()) {
      break;
    }
  }

  return path;
}

/**
 * This is recursive function to traverse fruTree at basepath
 * and print fruId information on console for each fru
 */
static void printAllFruIdInfo(const string & basepath) {
  vector<string> list = getFruNamesFromXml(getIntrospectionXml(basepath));

  for (auto &it : list) {
    //Print fruid info
    printFruIdInfo(basepath + "/" + it);

    //Call recursively for child frus
    printAllFruIdInfo(basepath + "/" + it);
  }
}

/*
 * Helper function to call dbus methods fruIdWriteBinaryData and fruIdDumpBinaryData for fru object at fruPath
 * prints whether command is successful or not
 */
static void fruIdBinaryDataMethod(const string & methodName, const string & fruPath, const string & fileName) {
  GDBusProxy* proxy = getDBusProxy(fruPath.c_str(), FRU_SVC_FRU_OBJECT_INTERFACE);
  GError *error = nullptr;
  gboolean status;

  GVariant *response = g_dbus_proxy_call_sync(
      proxy,
      methodName.c_str(),
      g_variant_new("(&s)", fileName.c_str()),
      G_DBUS_CALL_FLAGS_NONE,
      -1,
      nullptr,
      &error);

  checkDBusErrorAndExit(error);

  //Get status from response
  g_variant_get (response, "(b)", &status);

  if (status == TRUE) {
    cout << "Command executed successfully " << endl;
  }
  else{
    cout << "Command failed " << endl;
  }

  g_variant_unref(response);
  g_object_unref(proxy);
}

/**
 * Returns absolute path for fileName
 * Absolute path is must as file path will be sent to fru-svc
 */
static string getAbsoluteFilePath(const string & fileName) {
  if (fileName.at(0) == '/') {
    //If fileName is already absolute path
    return fileName;
  }
  else {
    // Get current working directory
    char currentDir[FILENAME_MAX];
    getcwd(currentDir, sizeof(currentDir));
    return std::string(currentDir) + "/" + fileName;
  }
}

/**
 * Validates if file exists
 */
static void validateFilename(const string & fileName) {
  std::ifstream ifs(fileName);
  if (!ifs.good()) { // file is not good
    printErrorExit("Invalid file : " + fileName, -1);
  }
}

/**
 * Entry point for fruid-util-v2
 * Usage: fruid-util-v2 [ all/fru-name ]
 * Usage: fruid-util-v2 [ fru-name ] [--dump | --write] <file>
 */
int main(int argc, char const *argv[]) {

  //Argument validation
  if (argc != 2 && argc != 4)
  {
    printUsageAndExit(-1);
  }

  if (argc == 2){
    //Processing of command fruid-util-v2 [ all/fru-name ]
    if (strcmp(argv[1], "all") == 0) {
      //print fruid information of all frus
      printAllFruIdInfo(FRU_SVC_BASE_PATH);
    }
    else {
      //Get path of input fru
      string fruPath = getFruPath(FRU_SVC_BASE_PATH, argv[1]);
      if (fruPath.empty()) {
        printUsageAndExit(-1);
      }
      else {
        printFruIdInfo(fruPath);
      }
    }
  }
  else if (argc == 4) {
    //processing of command fruid-util-v2 [ fru-name ] [--dump | --write] <file>
    if (strcmp(argv[1], "all") == 0) {
      printUsageAndExit(-1);
    }
    else {
      if (strcmp(argv[2], "--dump") == 0) {
        fruIdBinaryDataMethod(FRU_SVC_METHOD_FRUID_DUMP_BIN_DATA,
                            getFruPath(FRU_SVC_BASE_PATH, argv[1]),
                            getAbsoluteFilePath(argv[3]));
      }
      else if (strcmp(argv[2], "--write") == 0) {
        validateFilename(argv[3]);
        fruIdBinaryDataMethod(FRU_SVC_METHOD_FRUID_WRITE_BIN_DATA,
                            getFruPath(FRU_SVC_BASE_PATH, argv[1]),
                            getAbsoluteFilePath(argv[3]));
      }
      else {
        printUsageAndExit(-1);
      }
    }
  }

  return 0;
}
