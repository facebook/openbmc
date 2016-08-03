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
#include <ipc-interface/Ipc.h>

namespace openbmc {
namespace qin {

/**
 * For testing ObjectTree.
 */
class DummyIpc : public Ipc {
  public:
    ~DummyIpc() {}

    void registerConnection() {}

    void unregisterConnection() {}

    void registerObject(const std::string &path, void* userData) {}

    void unregisterObject(const std::string &path) {}

    bool isPathAllowed(const std::string &path) const {
      return true;
    }

    const std::string getPath(const std::string &parentPath,
                              const std::string &name) const {
      return parentPath + "/" + name;
    }
};
} // namespace qin
} // namespace openbmc
