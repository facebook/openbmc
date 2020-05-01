/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright 2020-present Facebook. All Rights Reserved.
 */

#include <limits>
#include <unistd.h>

#include "fileops.hpp"
#include "kv.hpp"
#include "log.hpp"

namespace kv
{

/* Paths to the kv stores. */
#ifndef __TEST__
constexpr auto cache_store = "/tmp/cache_store";
constexpr auto kv_store    = "/mnt/data/kv_store";
#else
constexpr auto cache_store = "./test/tmp";
constexpr auto kv_store    = "./test/persist";
#endif

static void create_dir(const FileHandle::path& dir) {
  if (std::filesystem::exists(dir)) {
    return;
  }

  std::filesystem::create_directories(dir);
}

static FileHandle::path get_key_path(const std::string& key, region r) {
  FileHandle::path p = r == region::persist ? kv_store : cache_store;

  auto key_path = p / key;
  create_dir(key_path.parent_path());

  return p / key;
}

template <FileHandle::access method>
void FileHandle::open_and_lock(const std::string& key, region r) {
  fpath = get_key_path(key, r);

  if constexpr (method == access::read)
  {
    fp = fopen(fpath.c_str(), "r");
    present = (fp != nullptr);
  }
  else
  {
    present = std::filesystem::exists(fpath);
    fp = fopen(fpath.c_str(), present ? "r+" : "w");
  }

  if (!fp) {
    throw std::filesystem::filesystem_error(
        "kv: error opening file: " + fpath.string(),
        std::error_code(errno, std::system_category()));
  }

  if (flock(fileno(fp), LOCK_EX) != 0) {
    throw std::filesystem::filesystem_error(
        "kv: error calling flock: " + fpath.string(),
        std::error_code(errno, std::system_category()));
  }
  locked = true;
}

// explicit instantiations
template
void FileHandle::open_and_lock<FileHandle::access::read>(
    const std::string&, region);
template
void FileHandle::open_and_lock<FileHandle::access::write>(
    const std::string&, region);

std::string FileHandle::read() {

  // Reset file descriptor back to the beginning.
  ::rewind(fp);

  std::array<char, max_len> data{};

  auto bytes = fread(data.data(), 1, max_len, fp);

  if ((0 == bytes) && ferror(fp)) {
    throw std::filesystem::filesystem_error(
        "kv: error reading from file: " + fpath.string(),
        std::error_code(errno, std::system_category()));
  }

  return std::string{std::begin(data), std::begin(data) + bytes};
}

void FileHandle::write(std::string value) {
  // Reset file descriptor back to the beginning.
  ::rewind(fp);

  if (ftruncate(fileno(fp), 0) < 0) { // truncate file to erase.
    throw std::filesystem::filesystem_error(
        "kv: error calling ftruncate: " + fpath.string(),
        std::error_code(errno, std::system_category()));
  }

  auto bytes = fwrite(value.data(), 1, value.size(), fp);
  if ((bytes == 0) && ferror(fp)) {
    throw std::filesystem::filesystem_error(
        "kv: error writing to file: " + fpath.string(),
        std::error_code(errno, std::system_category()));
  }

  if (bytes != value.size()) {
    throw std::filesystem::filesystem_error(
        "kv: error writing full contents to file: " + fpath.string(),
        std::error_code(ENOSPC, std::system_category()));
  }

  if (0 != fflush(fp)) {
    throw std::filesystem::filesystem_error(
        "kv: error calling fflush: " + fpath.string(),
        std::error_code(errno, std::system_category()));
  }
}

} // namespace kv
