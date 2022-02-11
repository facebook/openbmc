/*
 * DBusSubscriber.cpp
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

static GMainLoop *loop;         // dbus Main Event Loop
static int signalCount;         // Exit when signalCount number of signals received
static guint subScriberId;      // dbus signal sucriber id

/*
 * Callback is called when signal is received
 */
void timerSignalCallBack (GDBusConnection *connection,
                          const gchar *sender_name,
                          const gchar *object_path,
                          const gchar *interface_name,
                          const gchar *signal_name,
                          GVariant *parameters,
                          gpointer user_data) {

    std::cout << "Received signal " << signal_name << std::endl;

    signalCount--;
    if (signalCount <= 0) {
      //unsubscribe signal
      g_dbus_connection_signal_unsubscribe(connection, subScriberId);
      //Stop GMainLoop from running
      g_main_loop_quit(loop);
      g_main_loop_unref(loop);
    }
}

/*
 * Entry point for dbus-subscriber
 * Takes two arguments, argv[1] - signal name, argv[2] - signalCount
 */
int main (int argc, char *argv[]) {
  GError *error = nullptr;
  GDBusConnection* connection;

  //Argument validation
  if (argc != 3) {
    std::cout << "Usage: dbus-subscriper [signal-name] [count]" << std::endl;
    exit(-1);
  }

  //Get signalCount from argv[2]
  signalCount = std::stoi(argv[2]);

  //Get connection to system bus
  connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);

  if (error != nullptr) {
    std::cout << "Error in connection : " << error->message << std::endl;
    exit(-1);
  }

  //Subscribe to signal argv[1]
  subScriberId = g_dbus_connection_signal_subscribe (connection,
                                      "org.openbmc.TimerService",
                                      "org.openbmc.TimerInterface",
                                      argv[1],
                                      "/org/openbmc/timer",
                                      nullptr,
                                      G_DBUS_SIGNAL_FLAGS_NONE,
                                      timerSignalCallBack,
                                      nullptr,
                                      nullptr);

  //start GMainLoop
  loop = g_main_loop_new(nullptr, FALSE);
  g_main_loop_run(loop);

  return 0;
}
