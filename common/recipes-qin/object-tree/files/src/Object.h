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
#include <system_error>
#include <memory>
#include <nlohmann/json.hpp>
#include <glog/logging.h>
#include <unordered_map>
#include "Attribute.h"

namespace openbmc {
namespace qin {

/**
 * Object contains a map of attributes and a map of children objects
 */
class Object {
  public:
    // map from name to attr
    typedef std::unordered_map<std::string, std::unique_ptr<Attribute>> AttrMap;

    // child object map from object name to object
    // The object itself does not have the ownership of the children. It
    // merely keeps track of the pointer.
    typedef std::unordered_map<std::string, Object*> ChildMap;

  protected:
    std::string name_;
    AttrMap     attrMap_;
    Object*     parent_{nullptr};   // pointer to the parent object
    ChildMap    childMap_;

  public:
    /**
     * Constructor with default parent as nullptr if not specified
     *
     * @param name of the object
     * @param parent of the object; not allowed to be nullptr
     */
    Object(const std::string &name, Object* parent = nullptr) {
      name_ = name;
      parent_ = parent;
      LOG(INFO) << "Creating Object \"" << name << "\"";
      if (parent_ != nullptr) {
        parent_->addChildObject(*this);
      }
    }

    /**
     * Virtual Destructor for inheritance
     */
    virtual ~Object() {};

    const std::string& getName() const {
      return name_;
    }

    Object* getParent() const {
      return parent_;
    }

    int getChildCount() const {
      return childMap_.size();
    }

    const ChildMap& getChildMap() const {
      return childMap_;
    }

    /**
     * Get the child object.
     *
     * @param name of the child object
     * @return nullptr if not found; Object* otherwise
     */
    Object* getChildObject(const std::string &name) const {
      ChildMap::const_iterator it;
      if ((it = childMap_.find(name)) == childMap_.end()) {
        return nullptr;
      }
      return it->second;
    }

    int getAttrCount() const {
      return attrMap_.size();
    }

    const AttrMap& getAttrMap() const {
      return attrMap_;
    }

    /**
     * Get attribute of the given name.
     *
     * @param number of the attribute
     * @return nullptr if not found; attribute otherwise
     */
    virtual Attribute* getAttribute(const std::string &name) const;

    /**
     * Read attribute value of the given name. It is a read function
     * instead of get just to match the modes in Attribute.
     *
     * @param name of the attribute to be read
     * @return value of the attribute
     * @throw std::invalid_argument if name not found
     * @throw std::system_error EPERM if attr has no read modes
     */
    virtual const std::string& readAttrValue(const std::string &name) const;

    /**
     * Write attribute value of the given name. It is a write function
     * instead of set just to match the modes in Attribute.
     *
     * @param name of the attribute to be written
     * @param value to be written to attribute
     * @throw std::invalid_argument if name not found
     * @throw std::system_error EPERM if attr has no read modes
     */
    virtual void writeAttrValue(const std::string &name,
                                const std::string &value);

    /**
     * Add attribute to the object.
     *
     * @param name of attribute
     * @throw std::invalid_argument if an attribute with the same name
     *        has existed
     * @return nullptr if name conflict; Attribute pointer otherwise
     */
    virtual Attribute* addAttribute(const std::string &name);

    /**
     * Delete attribute from the object.
     *
     * @param name of attribute
     * @throw std::invalid_argument if attribute not found
     */
    virtual void deleteAttribute(const std::string &name);

    /**
     * Add child object to the childChildMap. Will also set the
     * parent of the child.
     *
     * @param child object
     * @throw std::invalid_argument if the child already has a parent
     *        or if this object already has a child with the same name
     */
    void addChildObject(Object &child);

    /**
     * Remove the object from the childChildMap but not delete it.
     * Will clear the parent of child to nullptr. Not deleting it
     * because the child object is not owned by the object class.
     *
     * @param child object
     * @throw std::invalid_argument if child object not found or
     *        if child object has non-empty children
     * @return nullptr if not found or child object has children;
     *         child object otherwise
     */
    Object* removeChildObject(const std::string &name);

    /**
     * Dump the object info including its attributes into json format.
     * Will not iteratively dump the full info of the child objects.
     * Instead, will only dump the names and number of them.
     *
     * @return nlohmann json object with the following entries.
     *
     *         objectName: name of object
     *         objectType: type of object; "Generic" here
     *         parentName: name of parent object; null if parent is nullptr
     *         attrCount:  number of attributes in object
     *         attributes: this array entry follows the sensor-info.schema.json
     *                     and will contain the attribute dump
     *         childObjectCount: number of child objects
     *         childObjectNames: this entry contains an array of names of the
     *                           child object
     */
    virtual nlohmann::json dumpToJson() const;

    /**
     * Dump the object info including its attributes into json format. The
     * child objects will be recursively dumpped.
     *
     * @return nlohmann json object with the following entries.
     *
     *         objectName: name of object
     *         objectType: type of object; "Generic" here
     *         parentName: name of parent object; null if parent is nullptr
     *         attrCount:  number of attributes in object
     *         attributes: this array entry follows the sensor-info.schema.json
     *                     and will contain the attribute dump
     *         childObjectCount: number of child objects
     *         childObjects: this entry contains an array of the child object
     */
    virtual nlohmann::json dumpToJsonRecursive() const;

    /**
     * Get object path from root.
     *
     * @return std::string path from root of the tree separated by '/'
     */
    std::string getObjectPath() const;

  protected:

    void setParent(Object* parent) {
      parent_ = parent;
    }

    /**
     * Get attribute of the given name and ensure it exists.
     *
     * @param name of the attribute
     * @throw std::system_error EPERM if attr is not readable
     * @return pointer to the attribute
     */
    virtual Attribute* getNonNullAttribute(const std::string &name) const {
      Attribute* attr;
      if ((attr = getAttribute(name)) == nullptr) {
        LOG(ERROR) << "Attribute \"" << name << "\" not found";
        throw std::invalid_argument("Attribute not found");
      }
      return attr;
    }

    /**
     * Get attribute of the given name and ensure it is readable.
     *
     * @param name of the attribute
     * @throw std::invalid_argument if attribute not found
     * @throw std::system_error EPERM if attr is not readable
     * @return pointer to the attribute
     */
    virtual Attribute* getReadableAttribute(const std::string &name) const {
      Attribute* attr = getNonNullAttribute(name);
      if (!attr->isReadable()) {
        LOG(ERROR) << "Attribute \"" << name
          << "\" does not support read";
        throw std::system_error(
            EPERM, std::system_category(), "Attribute read not supported");
      }
      return attr;
    }

    /**
     * Get attribute of the given name and ensure it is writable.
     *
     * @param name of the attribute
     * @throw std::invalid_argument if attribute not found
     * @throw std::system_error EPERM if attr is not writable
     * @return pointer to the attribute
     */
    virtual Attribute* getWritableAttribute(const std::string &name) const {
      Attribute* attr = getNonNullAttribute(name);
      if (!attr->isWritable()) {
        LOG(ERROR) << "Attribute \"" << name
          << "\" does not support write";
        throw std::system_error(
            EPERM, std::system_category(), "Attribute write not supported");
      }
      return attr;
    }

    /**
     * Dump the object info into json format. This is a helper function for
     * dumpToJson() and dumpToJsonIterative(). All the info is dumpped
     * except the info regarding the child objects.
     *
     * @return nlohmann json object with the following entries.
     *
     *         objectName: name of object
     *         objectType: type of object; "Generic" here
     *         parentName: name of parent object; null if parent is nullptr
     *         attrCount:  number of attributes in object
     *         attributes: this array entry follows the sensor-info.schema.json
     *                     and will contain the attribute dump
     */
    nlohmann::json dump() const;
};

} // namespace qin
} // namespace openbmc
