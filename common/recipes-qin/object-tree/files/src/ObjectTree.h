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
namespace qin {

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
     *        or if ipc is nullptr
     */
    ObjectTree(const std::shared_ptr<Ipc> &ipc, const std::string &rootName);

    /**
     * Delete remove object registration
     */
    virtual ~ObjectTree() {
      for (auto &it : objectMap_) {
        ipc_.get()->unregisterObject(it.first);
      }
    }

    const Ipc* getIpc() const {
      return ipc_.get();
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
      ObjectMap::const_iterator it;
      if ((it = objectMap_.find(path)) == objectMap_.end()) {
        return nullptr;
      }
      return it->second.get();
    }

    bool containObject(const std::string &path) const {
      return getObject(path) != nullptr;
    }

    /**
     * Add an object to the objectMap_ with parent path specified.
     *
     * @param name of the object to be added
     * @param parentPath is the parent object path for the object to be
     *        added at
     * @throw std::invalid_argument if
     *        * parentPath not found
     *        * name does not meet the ipc's rule
     *        * the object with the same name has already existed under
     *          the parent
     * @return raw pointer to the created object
     */
    virtual Object* addObject(const std::string &name,
                              const std::string &parentPath);

    /**
     * Add the specified object to objectMap_ under the specified parent path.
     * It is assumed that the object should not have any parent nor children
     * objects. Otherwise, this function will throw an exception.
     *
     * @param unique_ptr to the object to be added
     * @param parentPath is the parent object path for the object
     *        to be added at
     * @throw std::invalid_argument if
     *        * the unique_ptr to object is empty
     *        * object has non-empty children or non-null parent
     *        * parentPath not found
     *        * name does not meet the ipc's rule
     *        * the object with the same name has already existed under
     *          the parent
     * @return nullptr if parentPath not found or the object with name
     *         has already existed under the parent; otherwise,
     *         object is created and added
     */
    virtual Object* addObject(std::unique_ptr<Object> object,
                              const std::string       &parentPath);

    /**
     * Delete the object with name under the parentPath. Won't be able to
     * delete root. Cannot delete objects having children.
     *
     * @param name of the object
     * @param path of the object's parent
     * @throw std::invalid_argument if
     *        * object not found
     *        * object is root
     *        * object has non-empty children
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
     * @throw std::invalid_argument if
     *        * object not found
     *        * object is root
     *        * object has non-empty children
     */
    virtual void deleteObjectByPath(const std::string &path);

    /**
     * This is the function that will be called by the static callback
     * at Ipc when the connection is acquired. Currently, there is
     * nothing to do. User can hack their implementation here. The arg
     * will be put in the constructor to Ipc.
     */
    static void onConnAcquiredCallBack() {
      // implementation here
    }

    /**
     * This is the function that will be called by the static callback
     * at Ipc when the connection is lost. Currently, there is
     * nothing to do. User can hack their implementation here. The arg
     * will be put in the constructor to Ipc.
     */
    static void onConnLostCallBack() {
      // implementation here
    }

  protected:

    /**
     * A helper function for adding an Object into the objectMap_ and
     * register it with ipc_.
     *
     * @param path to be registered with the Object
     * @param unique_ptr to the Object
     * @return raw pointer to the Object
     */
    Object* addObjectByPath(std::unique_ptr<Object> upObj,
                            const std::string       &path) {
      Object* object = upObj.get();
      objectMap_.insert(std::make_pair(path, std::move(upObj)));
      ipc_->registerObject(path, object);
      return object;
    }

    /**
     * Get a new path from parentPath and specified name through ipc_.
     *
     * @param parentPath of the object name to be created
     * @param name of the object to be created
     * @throw std::invalid_argument if path does not meet ipc's rule
     * @return path that combines parentPath and name through ipc's rule
     */
    const std::string getPath(const std::string &parentPath,
                              const std::string &name) const {
      const std::string path = ipc_.get()->getPath(parentPath, name);
      if (!ipc_->isPathAllowed(path)) {
        LOG(ERROR) << "Object path \"" << path << "\" "
          << "does not match the ipc rule.";
        throw std::invalid_argument("Invalid object path");
      }
      return path;
    }

    /**
     * Get parent from the parentPath for adding new object. Check if
     * there is a duplicated name under the parent.
     *
     * @param parentPath of the object name to be created
     * @param name of the object to be created
     * @throw std::invalid_argument if the parent does not exist or there is
     *        a duplicated child with the same name
     * @return pointer to the parent object
     */
    Object* getParent(const std::string &parentPath,
                      const std::string &name) const;

    /**
     * A helper function to check if object passed in is valid.
     *
     * @param const ptr to Object
     * @throw std::invalid_argument if object is nullptr or if the object
     *        has non-empty children
     */
    void checkObject(const Object* object) const;
};

} // namespace qin
} // namespace openbmc
