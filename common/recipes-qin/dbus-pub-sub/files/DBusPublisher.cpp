/*
 * DBusPublisher.cpp: publishes signals one_sec_timer, three_sec_timer, five_sec_timer
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
#include <string>

static const gchar introspection_xml[] =
  "<!DOCTYPE node PUBLIC" \
  " \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" " \
  " \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">" \
  "<node>"
  "  <interface name='org.openbmc.TimerInterface'>"
  "    <signal name='one_sec_timer'/>"
  "    <signal name='three_sec_timer'/>"
  "    <signal name='five_sec_timer'/>"
  "  </interface>"
  "</node>";

//Class for Timer Signal
class TimerSignal {
  private:
    GDBusConnection * connection_;       //dbus connection
    const std::string objectPath_;       //dbus object path
    const std::string interfaceName_;    //dbus interface Name
    const std::string signalName_;       //dbus signal name

  public:
    /*
     * Constructor
     */
    TimerSignal(GDBusConnection * connection,
                const std::string & objectPath,
                const std::string & interfaceName,
                const std::string & signalName)
                : connection_(connection), objectPath_(objectPath),
                  interfaceName_(interfaceName), signalName_(signalName) {}

    /*
     * emits signal signalName_
     */
    void publish() {
      GError * error = nullptr;
      g_dbus_connection_emit_signal (connection_,
                                     nullptr,
                                     objectPath_.c_str(),
                                     interfaceName_.c_str(),
                                     signalName_.c_str(),
                                     nullptr,
                                     &error);

      if (error != nullptr) {
        std::cout << "Error in TimerSignal publish: "
                  << error->message << std::endl;
        exit(-1);
      }
    }
};

/*
 * Callback for g_timeout
 * Publishes timer signal
 */
gboolean timercallback (gpointer user_data) {
  TimerSignal *timerSignal = (TimerSignal*)user_data;
  timerSignal->publish();
  return true;
}

/*
 * Callback for dbus name acquired
 */
static void onTimerServiceNameAcquired (GDBusConnection *connection,
                                        const gchar     *name,
                                        gpointer         user_data) {
  guint registration_id;
  GDBusNodeInfo *introspection_data;
  GError * error = nullptr;

  // build introspection data structures from xml
  introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, nullptr);
  //Register object "/org/openbmc/timer"
  registration_id = g_dbus_connection_register_object (
                                        connection,
                                        "/org/openbmc/timer",
                                        introspection_data->interfaces[0],
                                        nullptr,
                                        nullptr,  /* user_data */
                                        nullptr,  /* user_data_free_func */
                                        &error); /* GError** */

  g_dbus_node_info_unref(introspection_data);

  if (error != nullptr) {
    std::cout << "Error in onTimerServiceNameAcquired: " << error->message << std::endl;
    exit(-1);
  }

  //Every one second publish signal one_sec_timer
  g_timeout_add_seconds(1, timercallback, new TimerSignal(connection,
                                                  "/org/openbmc/timer",
                                                  "org.openbmc.TimerInterface",
                                                  "one_sec_timer"));

  //Every 3 seconds publish signal three_sec_timer
  g_timeout_add_seconds(3, timercallback, new TimerSignal(connection,
                                                  "/org/openbmc/timer",
                                                  "org.openbmc.TimerInterface",
                                                  "three_sec_timer"));

  //Every 5 seconds publish signal five_sec_timer
  g_timeout_add_seconds(5, timercallback, new TimerSignal(connection,
                                                  "/org/openbmc/timer",
                                                  "org.openbmc.TimerInterface",
                                                  "five_sec_timer"));
}

/*
 * Callback for dbus name lost
 */
static void onTimerServiceNameLost (GDBusConnection *connection,
                                    const gchar     *name,
                                    gpointer         user_data) {
  std::cout << "Error: dbus Name lost, exiting " << std::endl;
  exit(1);
}

/*
 * Entry point for dbus-publisher
 * Publishes dbus signals:
 * 1. one_sec_timer- Every second
 * 2. three_sec_timer- Every 3 seconds
 * 3. five_sec_timer- Every 5 seconds
 */
int main (int argc, char *argv[]) {
  guint owner_id;
  GMainLoop *loop;

  // reserve bus name
  owner_id = g_bus_own_name(G_BUS_TYPE_SYSTEM,
                            "org.openbmc.TimerService",
                            G_BUS_NAME_OWNER_FLAGS_NONE,
                            nullptr,
                            onTimerServiceNameAcquired,
                            onTimerServiceNameLost,
                            nullptr,
                            nullptr);

  loop = g_main_loop_new(nullptr, FALSE);
  g_main_loop_run(loop);

  g_bus_unown_name(owner_id);

  return 0;
}
