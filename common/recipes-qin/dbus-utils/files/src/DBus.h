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

#pragma once
#include <unordered_map>
#include <string>
#include <regex>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <gio/gio.h>
#include <ipc-interface/Ipc.h>
#include "DBusInterfaceBase.h"
#include "dbus-interface/DBusDefaultInterface.h"
#include "DBusObject.h"

namespace openbmc {
namespace qin {

typedef std::unordered_map<std::string,
                           std::unique_ptr<DBusObject>> DBusObjectMap;

/**
 * DBus class inherits IPC and provides a reusable interface
 * to set up connection with dbus through GDBUS library.
 * There is an 1-1 mapping between DBus instance and DBus name.
 */
class DBus : public Ipc {
  private:
    mutable std::mutex              m_;
    mutable std::condition_variable cvConn_; // notify connection
    // connection to dbus daemon
    GDBusConnection*                connection_{nullptr};
    // dbus id; never 0 after owning the dbus name
    std::string                     name_;
    unsigned int                    id_{0};
    DBusObjectMap                   objectMap_;
    // default interface will be used if no interface specified
    // in the constructor nor when registering an object
    DBusDefaultInterface            defaultInterface_;
    DBusInterfaceBase*              interface_{&defaultInterface_};

  public:
    // default regex for matching DBus name
    static std::regex dbusNameRegex;
    // default regex for matching object path
    static std::regex pathRegex;

    /**
     * Constructor that sets the name of the DBus. The name
     * should match the DBus name regex.
     *
     * @param name of DBus to be registered
     * @param interface can replace default interface or be nullptr
     * @throw invalid_argument if dbus name does not match regex
     */
    DBus(const std::string &name,
         DBusInterfaceBase* interface = nullptr);

    /**
     * If the DBus is connected (DBus name is owned), destructor will
     * unregister all the objects and the connection.
     */

    ~DBus() {
      if (id_ > 0) {
        for (auto it = objectMap_.begin(); it != objectMap_.end(); it++) {
          unregisterObject(it->first);
        }
        g_bus_unown_name(id_);
      }
    }

    const std::string& getName() const {
      return name_;
    }

    unsigned int getId() const {
      std::lock_guard<std::mutex> lock(m_);
      return id_;
    }

    bool isConnected() const {
      std::lock_guard<std::mutex> lock(m_);
      return connection_ != nullptr;
    }

    DBusInterfaceBase& getDefaultInterface() const {
      return *interface_;
    }

    /**
     * Get the DBusObject with specified path.
     *
     * @param path of DBusObject
     * @return nullptr if not found; DBusObject pointer otherwise
     */
    const DBusObject* getDBusObject(const std::string &path) const{
      std::lock_guard<std::mutex> lock(m_);
      return getMutableDBusObject(path);
    }

    const DBusObjectMap& getDBusObjectMap() const{
      std::lock_guard<std::mutex> lock(m_);
      return objectMap_;
    }

    /**
     * Tell dbus daemon to own the SYSTEM bus with name_.
     * Only owning one bus name is available for each DBus class realization.
     * Calls onNameLost if the dbus name cannot be acquired.
     *
     * @throw std::system_error ECONNREFUSED if DBus name cannot be acquired
     */
    void registerConnection() override;

    /**
     * Unregister the dbus and release the name_.
     */
    void unregisterConnection() override;

    /**
     * Conditionally wait for connection. Returns when connection is done.
     * Note that there is no need to wait for disconnection since
     * unregistering connection happens immediately.
     */
    void waitForConnection() const {
      std::unique_lock<std::mutex> lock(m_);
      while (connection_ == nullptr) {
        cvConn_.wait(lock);
      }
    }

    /**
     * Check if the object path matches the regex
     *
     * @param path of the object
     * @return true if matched; false otherwise
     */
    bool isPathAllowed(const std::string &path) const override {
      return std::regex_match(path, pathRegex);
    }

    const std::string getPath(const std::string &parentPath,
                              const std::string &name) const override {
      return parentPath + "/" + name;
    }

    bool isObjectRegistered(const std::string &path,
                            DBusInterfaceBase &interface) const;

    /**
     * Register the dbus object path with the interface.
     *
     * @param path of the object
     * @param *interface to be added
     * @param userData to be registered and used in method callback
     * @throw invalid_argument if object path does not match regex
     */
    void registerObject(const std::string &path,
                        DBusInterfaceBase &interface,
                        void*             userData);

    /**
     * Register the dbus object path with the interface_ specified
     * by the contructor or the defaultInterface_.
     *
     * @param path of the object
     * @param userData to be registered and used in method callback
     * @throw invalid_argument if object path does not match regex
     * @throw runtime_error if unregistration failed
     */
    void registerObject(const std::string &path,
                        void*             userData) override {
      registerObject(path, *interface_, userData);
    }

    /**
     * Unregister the dbus object path with the interface. Upon unregistration,
     * delete the interface from object's map. The object will be deleted if
     * there are no interfaces left in its interfaceMap.
     *
     * @param path of the object
     * @param interface to be added
     * @throw system_error ENOENT if object not found
     * @throw runtime_error if unregistration failed
     */
    void unregisterObject(const std::string &path,
                          DBusInterfaceBase &interface);

    /**
     * Unregister the dbus object path with all the associated interfaces.
     * Delete the object.
     *
     * @param path of the object
     * @throw system_error ENOENT if object not found
     * @throw runtime_error if unregistration failed
     */
    void unregisterObject(const std::string &path) override;

  private:
    DBusObject* getMutableDBusObject(const std::string &path) const {
      DBusObjectMap::const_iterator it;
      if ((it = objectMap_.find(path)) == objectMap_.end()) {
        return nullptr;
      }
      return it->second.get();
    }

    /**
     * Register the object with the interface. It is assumed that objectMap_
     * contains object, and that interface has not been registered on object.
     *
     * @param *object to be added
     * @param *interface to be added
     */
    void registerObjectInterface(DBusObject              &object,
                                 DBusInterfaceBase &interface,
                                 void*                   userData);

    /**
     * Unregister the object with the interface. It is assumed that neither
     * of object nor interface are nullptr and that objectMap_ contains object.
     *
     * @param *object to be added
     * @param *interface to be added
     */
    void unregisterObjectInterface(DBusObject &object,
                                   DBusInterfaceBase &interface);

    /**
     * Event handler on bus acquired. Objects are registered here.
     * Note that this needs to be static.
     * The object of dbus is passed in arg.
     */
    static void onBusAcquired (GDBusConnection *connection,
                               const gchar     *name,
                               gpointer         arg) {}

    /**
     * Event handler on name acquired
     * Can be hacked to do whatever you want
     */
    static void onNameAcquired (GDBusConnection *connection,
                                const gchar     *name,
                                gpointer         arg);

    /**
     * Event handler on name lost. Reset DBusObject id to indicate
     * that they are not registered. Close the DBus connection if
     * it is still alive. Unregister connection during the event
     * loop will cause this callback to be called.
     * TODO: set error flags; logging error could be enough.
     */
    static void onNameLost (GDBusConnection *connection,
                            const gchar     *name,
                            gpointer         arg);
};

} // namespace qin
} // namespace openbmc
