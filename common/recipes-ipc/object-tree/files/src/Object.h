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
#include <memory>
#include <glog/logging.h>
#include <unordered_map>
#include "Attribute.h"

namespace openbmc {
namespace ipc {

/**
 * Object contains a two-level map of attributes and
 * a map of children objects
 */
class Object {
  public:
    // two-level maps from type to name to attribute
    // map from name to attr
    typedef std::unordered_map<std::string, std::unique_ptr<Attribute>> AttrMap;
    // map from type to AttrMap
    typedef std::unordered_map<std::string, std::unique_ptr<AttrMap>> TypeMap;

    // child object map from object name to object
    // The object itself does not have the ownership of the children. It
    // merely keeps track of the pointer.
    typedef std::unordered_map<std::string, Object*> ChildMap;

  protected:
    std::string name_;
    TypeMap     typeMap_;
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

    int getChildNumber() const {
      return childMap_.size();
    }


    const ChildMap* getChildMap() const {
      return &childMap_;
    }

    /**
     * Get the child object.
     *
     * @param name of the child object
     * @return nullptr if not found; Object* otherwise
     */
    Object* getChildObject(const std::string &name) const {
      if (childMap_.find(name) == childMap_.end()) {
        return nullptr;
      }
      return childMap_.find(name)->second;
    }

    /**
     * Return pointer to typeMap_. Provide the use of iterating through
     * all the attributes.
     */
    const TypeMap* getTypeMap() const {
      return &typeMap_;
    }

    /**
     * Return AttrMap with the given type in typeMap_. Not
     * transferring ownership.
     *
     * @return nullptr if not found; AttrMap otherwise.
     */
    const AttrMap* getAttrMap(const std::string &type) const {
      if (typeMap_.find(type) == typeMap_.end()) {
        return nullptr;
      }
      return typeMap_.find(type)->second.get();
    }

    /**
     * Get attribute of the given name and type "Generic".
     *
     * @param number of the attribute
     * @return nullptr if not found; attribute otherwise
     */
    virtual Attribute* getAttribute(const std::string &name) const {
      return getAttribute(name, "Generic");
    }

    /**
     * Get attribute of the given name and type.
     *
     * @param number of the attribute
     * @param type of the attribute
     * @return nullptr if not found; attribute otherwise
     */
    virtual Attribute* getAttribute(const std::string &name,
                                    const std::string &type) const;

    /**
     * Add attribute to the object. Create the corresponding AttrMap and
     * Attribute.  Here, the attribute will be added with default type
     * "Generic."
     *
     * @param name of attribute
     * @return nullptr if name conflict; Attribute pointer otherwise
     */
    virtual Attribute* addAttribute(const std::string &name) {
      return addAttribute(name, "Generic");
    }

    /**
     * Add attribute to the object but with type specified.
     *
     * @param name of attribute
     * @param type of attribute
     * @return nullptr if name conflict; Attribute pointer otherwise
     */
    virtual Attribute* addAttribute(const std::string &name,
                                    const std::string &type);

    /**
     * Delete attribute from the object. Remove the AttrMap
     * from the typeAttrMap if there are no attributes of the
     * corresponding type. Default type is "Generic."
     *
     * @param name of attribute
     */
    virtual void deleteAttribute(const std::string &name) {
      deleteAttribute(name, "Generic");
    }

    /**
     * Delete attribute from the object with specified type.
     *
     * @param name of attribute
     * @param type of attribute
     */
    virtual void deleteAttribute(const std::string &name,
                                 const std::string &type);

    /**
     * Add child object to the childChildMap. Will also set the
     * parent of the child.
     *
     * @param child object
     */
    void addChildObject(Object &child);

    /**
     * Remove the object from the childChildMap but not delete it.
     * Will clear the parent of child to nullptr. Not deleting it
     * because the child object is not owned by the object class.
     *
     * @param child object
     * @return nullptr if not found or child object has children;
     *         child object otherwise
     */
    Object* removeChildObject(const std::string &name);

  protected:
    void setParent(Object* parent) {
      parent_ = parent;
    }
};

} // namespace openbmc
} // namespace ipc
