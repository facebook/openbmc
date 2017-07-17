/*
 * DBusFruInterface.cpp
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

#include <ctime>
#include <string>
#include <stdexcept>
#include <system_error>
#include <glog/logging.h>
#include <gio/gio.h>
#include <object-tree/Object.h>
#include "DBusFruInterface.h"
#include "FRU.h"

const char* DBusFruInterface::xml =
"<!DOCTYPE node PUBLIC"
" \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
" \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
"<node>"
"  <interface name='org.openbmc.FruObject'>"
"    <method name='getFruIdInfo'>"
"      <arg type='a{ss}' name='fruId' direction='out'/>"
"    </method>"
"  </interface>"
"</node>";

DBusFruInterface::DBusFruInterface() {
  info_ = g_dbus_node_info_new_for_xml(xml, nullptr);
  if (info_ == nullptr) {
    LOG(ERROR) << "xml parsing for dbus interface failed";
    throw std::invalid_argument("XML parsing failed");
  }
  no_ = 0;
  name_ = info_->interfaces[no_]->name;
  vtable_ = {methodCallBack, nullptr, nullptr, nullptr};
}

DBusFruInterface::~DBusFruInterface() {
  g_dbus_node_info_unref(info_);
}

void DBusFruInterface::getFruIdInfo(GDBusMethodInvocation* invocation,
                                    gpointer               arg){
  FRU* fru = static_cast<FRU*>(arg);

  GVariantBuilder* builder;
  builder = g_variant_builder_new (G_VARIANT_TYPE("a{ss}"));


  //Get getFruIdInfo list from fru
  std::vector<std::pair<std::string, std::string>> getFruIdInfoList =  fru->getFruIdInfoList();

  //Add getFruIdInfo list to dictionary
  for (auto& it: getFruIdInfoList){
    g_variant_builder_add (builder, "{ss}", it.first.c_str(), it.second.c_str());
  }

  g_dbus_method_invocation_return_value (invocation, g_variant_new ("(a{ss})", builder));
  g_variant_builder_unref(builder);
}

void DBusFruInterface::methodCallBack(GDBusConnection*       connection,
                                      const char*            sender,
                                      const char*            objectPath,
                                      const char*            interfaceName,
                                      const char*            methodName,
                                      GVariant*              parameters,
                                      GDBusMethodInvocation* invocation,
                                      gpointer               arg) {
  // arg should be a pointer to FRU
  DCHECK(arg != nullptr) << "Empty object passed to callback";

  if (g_strcmp0(methodName, "getFruIdInfo") == 0) {
    getFruIdInfo(invocation, arg);
  }
}
