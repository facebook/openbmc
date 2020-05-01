#pragma once

/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright 2020-present Facebook. All Rights Reserved.
 */

#if (__GNUC__ < 8)
#include <experimental/filesystem>
namespace std
{
    namespace filesystem = experimental::filesystem;
}
#else
#include <filesystem>
#endif
#include <string>
#include <sys/file.h>

#include "kv.hpp"

namespace kv
{

class FileHandle
{
  public:

    using path = std::filesystem::path;
    enum class access { read, write };

    FileHandle() {};

    ~FileHandle() {
      if (fp) {
        if (locked) {
          flock(fileno(fp), LOCK_UN);
          locked = false;
        }
        fclose(fp);
      }
    }

    template <access>
    void open_and_lock(const std::string& key, region r);

    std::string read();
    void write(std::string value);

    FileHandle(const FileHandle&) = delete;
    FileHandle(FileHandle&&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    FileHandle& operator=(FileHandle&&) = delete;

    operator bool() const { return fp != nullptr; }

    bool was_present() const { return present; }

  private:

    FILE* fp = nullptr;
    path fpath = {};
    bool present = false;
    bool locked = false;
};

} // namespace kv
