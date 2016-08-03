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

#include <glog/logging.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <stdio.h>
#include "../dbus-interface/DBusDefaultInterface.h"
using namespace openbmc::qin;

static DBusDefaultInterface testInterface;

static void on_bus_acquired(GDBusConnection *connection,
                            const gchar     *name,
                            gpointer         user_data) {
  guint registration_id;
  registration_id =
    g_dbus_connection_register_object(connection,
    "/org/openbmc/test",
    testInterface.getInfo()->interfaces[testInterface.getNo()],
    testInterface.getVtable(),
    nullptr,  /* user_data */
    nullptr,  /* user_data_free_func */
    nullptr); /* GError** */
  if (registration_id == 0)
    LOG(FATAL) << "Object registration failed.";
}

static void on_name_acquired(GDBusConnection *connection,
                             const gchar     *name,
                             gpointer         user_data) {
}

static void on_name_lost(GDBusConnection *connection,
                         const gchar     *name,
                         gpointer         user_data) {
  LOG(FATAL) << "DBus name losted";
}

int main (int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  guint owner_id;
  GMainLoop *loop;

  // reserve bus name
  owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
                            "org.openbmc.TestServer",
                            G_BUS_NAME_OWNER_FLAGS_NONE,
                            on_bus_acquired,
                            on_name_acquired,
                            on_name_lost,
                            nullptr,
                            nullptr);

  loop = g_main_loop_new(nullptr, FALSE);
  g_main_loop_run(loop);

  g_bus_unown_name(owner_id);

  return 0;
}
