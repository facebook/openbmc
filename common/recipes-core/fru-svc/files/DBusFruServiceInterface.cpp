/*
 * DBusFruServiceInterface.cpp
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

#include <string>
#include <glog/logging.h>
#include <gio/gio.h>
#include <nlohmann/json.hpp>
#include <object-tree/Object.h>
#include "DBusFruServiceInterface.h"
#include "FruJsonParser.h"

namespace openbmc {
namespace qin {

static const char* xml =
  "<!DOCTYPE node PUBLIC"
  " \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
  " \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
  "<node>"
  "  <interface name='org.openbmc.FruService'>"
  "    <method name='addFRU'>"
  "      <arg type='s' name='fruParentPath' direction='in'/>"
  "      <arg type='s' name='fruJsonString' direction='in'/>"
  "      <arg type='b' name='status' direction='out'/>"
  "    </method>"
  "    <method name='resetTree'>"
  "    </method>"
  "    <method name='removeFRU'>"
  "      <arg type='s' name='fruPath' direction='in'/>"
  "      <arg type='b' name='status' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";

DBusFruServiceInterface::DBusFruServiceInterface() {
  info_ = g_dbus_node_info_new_for_xml(xml, nullptr);
  if (info_ == nullptr) {
    LOG(ERROR) << "xml parsing for dbus interface failed";
    throw std::invalid_argument("XML parsing failed");
  }
  no_ = 0;
  name_ = info_->interfaces[no_]->name;
  vtable_ = {methodCallBack, nullptr, nullptr, nullptr};
}

DBusFruServiceInterface::~DBusFruServiceInterface() {
  g_dbus_node_info_unref(info_);
}

void DBusFruServiceInterface::addFRU(GDBusMethodInvocation* invocation,
                                     GVariant*              parameters,
                                     FruObjectTree*         fruTree,
                                     const char*            objectPath) {

  gchar* fruJsonString = NULL;
  gchar* fruParentPath = NULL;
  gboolean status = TRUE;

  g_variant_get(parameters, "(ss)", &fruParentPath, &fruJsonString);

  LOG(INFO) << "addFRU at " << fruParentPath << " Fru :" << fruJsonString;

  if (fruTree->getObject(fruParentPath) == nullptr) {
    LOG(ERROR) << "Object " << fruParentPath << " does not exists";
    status = FALSE;
  }
  else {
    //Covert fruJsonString to nlohmann::json jObject and add fru to FruTree
    nlohmann::json jObject = nlohmann::json::parse(fruJsonString);
    FruJsonParser::parseFRU(jObject, *fruTree, fruParentPath);
  }

  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(b)", status));
}

/**
 * Helper function to remove subtree at Object obj from FruTree
 */
static void deleteSubtree(FruObjectTree* fruTree , Object* obj) {
  Object::ChildMap childMap = obj->getChildMap();
  for (auto it = childMap.cbegin(); it != childMap.cend();) {
    deleteSubtree(fruTree, (it++)->second);
  }

  fruTree->deleteObjectByPath(obj->getObjectPath());
}

void DBusFruServiceInterface::resetTree(GDBusMethodInvocation* invocation,
                                        FruObjectTree*         fruTree,
                                        const char*            objectPath) {
  LOG(INFO) << "resetTree";

  Object* obj = fruTree->getObject(objectPath);
  Object::ChildMap childMap = obj->getChildMap();
  for (auto it = childMap.cbegin(); it != childMap.cend();) {
    deleteSubtree(fruTree, (it++)->second);
  }

  g_dbus_method_invocation_return_value (invocation, NULL);
}

void DBusFruServiceInterface::removeFRU(GDBusMethodInvocation* invocation,
                                        GVariant*              parameters,
                                        FruObjectTree*         fruTree,
                                        const char*            objectPath) {
  gchar* fruPath = NULL;
  gboolean status = TRUE;

  g_variant_get(parameters, "(&s)", &fruPath);

  LOG(INFO) << "removeFRU " << fruPath;

  Object* obj = fruTree->getObject(fruPath);
  if ((dynamic_cast<FRU*>(obj)) != nullptr) {
    deleteSubtree(fruTree, obj);
  }
  else {
    LOG(ERROR) << "FRU " << fruPath << " does not exists";
    status = FALSE;
  }

  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(b)", status));
}

void DBusFruServiceInterface::methodCallBack(
                                     GDBusConnection*       connection,
                                     const char*            sender,
                                     const char*            objectPath,
                                     const char*            interfaceName,
                                     const char*            methodName,
                                     GVariant*              parameters,
                                     GDBusMethodInvocation* invocation,
                                     gpointer               arg) {
  // arg should be a pointer to Object in object-tree
  DCHECK(arg != nullptr) << "Empty object passed to callback";

  FruObjectTree* fruTree = static_cast<FruObjectTree*>(arg);

  if (g_strcmp0(methodName, "addFRU") == 0) {
    addFRU(invocation, parameters, fruTree, objectPath);
  }
  else if (g_strcmp0(methodName, "resetTree") == 0) {
    resetTree(invocation, fruTree, objectPath);
  }
  else if (g_strcmp0(methodName, "removeFRU") == 0) {
    removeFRU(invocation, parameters, fruTree, objectPath);
  }
}

} // namespace qin
} // namespace openbmc
