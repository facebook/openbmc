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
#include <string>
#include <unordered_map>
#include <memory>
#include <ipc-interface/Ipc.h>
#include "Object.h"

namespace openbmc {
namespace ipc {

/**
 * Tree structured object superclass that manages the objects and
 * bridges them with the interprocess communicaton (IPC).
 * Through this superclass, the objects are hidden behind the interface.
 * This prevents messing up with the tree data structure and
 * memory management. Object management functions are virtual in
 * order to provide reimplementation and polymorphism on Object
 * class.
 */
class ObjectTree {
  public:
    typedef std::unordered_map<std::string, std::unique_ptr<Object>> ObjectMap;

  protected:
    std::shared_ptr<Ipc>  ipc_;        // pointer to the ipc interface
    Object*               root_;       // pointer to the root object
    ObjectMap             objectMap_;  // path to *object map of all objects

  public:
    /**
     * Constructor. Root object will be added.
     *
     * @param ipc interface
     * @param rootName name of the root object
     * @throw invalid_argument if rootName does not meet the ipc regex
     */
    ObjectTree(const std::shared_ptr<Ipc> &ipc, const std::string &rootName);

    /**
     * Delete remove object registration
     */
    virtual ~ObjectTree() {
      for (auto it = objectMap_.begin(); it != objectMap_.end(); it++) {
        ipc_.get()->unregisterObject(it->first);
      }
    }

    Object* getRoot() const {
      return root_;
    }

    int getObjectCount() const {
      return objectMap_.size();
    }

    /**
     * Get the object. Allow modifying the Object at the outside caller
     * especially for managing the Attributes. This saves the wrapper
     * functions here for Attribute and also saves reimplementatiing
     * functions for derived Attribute class when needed. But one
     * should be careful about messing up the tree structure of the
     * objects.
     *
     * @param path of the object
     * @return nullptr if not found; Object* otherwise
     */
    Object* getObject(const std::string &path) const {
      if (objectMap_.find(path) == objectMap_.end()) {
        return nullptr;
      }
      return objectMap_.find(path)->second.get();
    }

    bool containObject(const std::string &path) const {
      return getObject(path) != nullptr;
    }

    /**
     * Add an object to the objectMap_ with parent path specified.
     * Use the default ObjectArg specified in the constructor.
     *
     * @param name of the object to be added
     * @param parentPath is the parent object path for the object
     *        to be added at
     * @return nullptr if parentPath not found or the object with name
     *         has already existed under the parent; otherwise,
     *         object is created and added
     */
    virtual Object* addObject(const std::string &name,
                              const std::string &parentPath);

    /**
     * Delete the object with name under the parentPath. Won't be able to
     * delete root. Cannot delete objects having children.
     *
     * @param name of the object
     * @param path of the object's parent
     */
    virtual void deleteObjectByName(const std::string &name,
                              const std::string &parentPath) {
      deleteObjectByPath(ipc_.get()->getPath(parentPath, name));
    }

    /**
     * Delete the object from the tree. Won't be able to delete root.
     * Cannot delete an object with children.
     *
     * @param path of the object
     */
    virtual void deleteObjectByPath(const std::string &path);

    /**
     * This is the function that will be called by the static callback
     * at Ipc when the connection is acquired. Currently, there is
     * nothing to do. User can hack their implementation here. The arg
     * will be put in the constructor to Ipc.
     */
    static void onConnAcquiredCallBack(void* arg) {
      // implementation here
    }

    /**
     * This is the function that will be called by the static callback
     * at Ipc when the connection is lost. Currently, there is
     * nothing to do. User can hack their implementation here. The arg
     * will be put in the constructor to Ipc.
     */
    static void onConnLostCallBack(void* arg) {
      // implementation here
    }

};

} // namespace ipc
} // namespace openbmc
