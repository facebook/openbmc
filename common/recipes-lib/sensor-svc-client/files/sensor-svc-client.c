/*
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
#include <openbmc/pal.h>
#include <syslog.h>
#include "sensor-svc-client.h"
#include <stdio.h>

//proxy to DBus objects are stored to optimize performance
static GDBusProxy* _proxy_sensor_service = NULL;
static GDBusProxy* _proxy_fru[MAX_NUM_FRUS] = {NULL};
static GDBusProxy* _proxy_sensor[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {NULL};

static GDBusProxy*
get_dbus_proxy(const char* path, const char* interface) {
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

  if (error != NULL) {
    syslog(LOG_ERR, "DBus error in setting proxy for object %s at inteface %s", path, interface);
  }
  return proxy;
}

// function to get proxy to DBUS FRU Object based on fru
static GDBusProxy*
get_proxy_fruobject (uint8_t fru){
  GVariant *response;
  const gchar* fruPath;
  GError *error = NULL;

  if (_proxy_sensor_service == NULL) {
    _proxy_sensor_service = get_dbus_proxy(SENSOR_SVC_BASE_PATH, SENSOR_SVC_SENSOR_TREE_INTERFACE);
    if (_proxy_sensor_service == NULL) {
      return NULL;
    }
  }

  //create proxy to FRU object if not yet created
  if (_proxy_fru[fru] == NULL) {
    // Get fru path from sensor service
    response = g_dbus_proxy_call_sync(
        _proxy_sensor_service,
        "org.openbmc.SensorTree.getFruPathById",
        g_variant_new ("(y)", fru),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);

    if (error != NULL) {
      syslog (LOG_ERR, "DBUS error in get_proxy_fruobject fru %d, %s", fru, error->message);
      g_object_unref(_proxy_sensor_service);
      _proxy_sensor_service = NULL; // Proxy to sensor service not working, reset
      return NULL;
    }

    g_variant_get(response, "(&s)", &fruPath);

    if ((fruPath == NULL) || (fruPath[0] == '\0')) {
      syslog (LOG_ERR, "Could not locate fru %d", fru);
      return NULL;
    }

    _proxy_fru[fru] = get_dbus_proxy(fruPath, SENSOR_SVC_SENSOR_TREE_INTERFACE);
    g_variant_unref(response);

  }
  return _proxy_fru[fru];
}

// function to get proxy to DBUS Sensor Object based on fru and sensor_num
static GDBusProxy*
get_proxy_sensorobject (uint8_t fru, uint8_t sensor_num){
  GDBusProxy* fruProxy;
  GError* error = NULL;
  const gchar* sensorPath;
  GVariant *response;

  //create proxy to sensor object if not yet created
  if (_proxy_sensor[fru][sensor_num] == NULL) {

    if ((fruProxy = get_proxy_fruobject(fru)) != NULL) {
      // Get sensor path from fru
      response = g_dbus_proxy_call_sync(
          fruProxy,
          "org.openbmc.SensorTree.getSensorPathById",
          g_variant_new ("(y)", sensor_num),
          G_DBUS_CALL_FLAGS_NONE,
          -1,
          NULL,
          &error);

      if (error != NULL) {
        syslog (LOG_ERR, "DBUS error in get_proxy_sensorobject fru %d sensor %d, %s", fru, sensor_num, error->message);
        g_object_unref(_proxy_fru[fru]);
        _proxy_fru[fru] = NULL; // Proxy to FRU not working, reset
        return NULL;
      }

      g_variant_get(response, "(&s)", &sensorPath);

      if ((sensorPath == NULL) || (sensorPath[0] == '\0')) {
        syslog (LOG_ERR, "Could not locate sensor %d", sensor_num);
        return NULL;
      }

      _proxy_sensor[fru][sensor_num] = get_dbus_proxy(sensorPath, SENSOR_SVC_SENSOR_OBJECT_INTERFACE);
      g_variant_unref(response);
    }
  }
  return _proxy_sensor[fru][sensor_num];
}

static int
sensor_read(uint8_t fru, uint8_t sensor_num, float *value, const char* method) {
  GDBusProxy* proxy = NULL;
  int readStatus;
  GVariant *response;
  GError *error = NULL;
  gdouble val;

  //create proxy to sensor object if not yet is created
  if ((proxy = get_proxy_sensorobject(fru, sensor_num)) != NULL) {
    response = g_dbus_proxy_call_sync(
        proxy,
        method,
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);

    if (error != NULL) {
      syslog (LOG_ERR, "DBUS error in sensorRead senor_num %d, %s", sensor_num, error->message);
      g_object_unref(_proxy_sensor[fru][sensor_num]);
      _proxy_sensor[fru][sensor_num] = NULL; // Proxy to Sensor not working, reset
      return -1;
    }

    g_variant_get(response, "(id)", &readStatus, &val);
    g_variant_unref(response);
    if (readStatus == 0) {
      *value = val;
    }

    return readStatus;
  }

  return -1;
}

int
sensor_svc_raw_read(uint8_t fru, uint8_t sensor_num, float *value) {
  return sensor_read(fru, sensor_num, value, "org.openbmc.SensorObject.sensorRawRead");
}

int
sensor_svc_read(uint8_t fru, uint8_t sensor_num, float *value) {
  return sensor_read(fru, sensor_num, value, "org.openbmc.SensorObject.sensorRead");
}
