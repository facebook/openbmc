/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright 2020-present Facebook. All Rights Reserved.
 */
#include <dirent.h>
#include <unistd.h>
#include <array>
#include <iostream>
#include <limits>
#include <regex>

#include "fileops.hpp"
#include "kv.hpp"
#include "log.hpp"

namespace fs = std::filesystem;

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
  if (fs::exists(dir)) {
    return;
  }

  fs::create_directories(dir);
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
    present = fs::exists(fpath);
    fp = fopen(fpath.c_str(), present ? "r+" : "w");
  }

  if (!fp) {
    throw fs::filesystem_error(
        "kv: error opening file", fpath,
        std::error_code(errno, std::system_category()));
  }

  if (flock(fileno(fp), LOCK_EX) != 0) {
    throw fs::filesystem_error(
        "kv: error calling flock", fpath,
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
    throw fs::filesystem_error(
        "kv: error reading from file", fpath,
        std::error_code(errno, std::system_category()));
  }

  return std::string{std::begin(data), std::begin(data) + bytes};
}

void FileHandle::write(std::string value) {
  // Reset file descriptor back to the beginning.
  ::rewind(fp);

  if (ftruncate(fileno(fp), 0) < 0) { // truncate file to erase.
    throw fs::filesystem_error(
        "kv: error calling ftruncate", fpath,
        std::error_code(errno, std::system_category()));
  }

  auto bytes = fwrite(value.data(), 1, value.size(), fp);
  if ((bytes == 0) && ferror(fp)) {
    throw fs::filesystem_error(
        "kv: error writing to file", fpath,
        std::error_code(errno, std::system_category()));
  }

  if (bytes != value.size()) {
    throw fs::filesystem_error(
        "kv: error writing full contents to file", fpath,
        std::error_code(ENOSPC, std::system_category()));
  }

  if (0 != fflush(fp)) {
    throw fs::filesystem_error(
        "kv: error calling fflush", fpath,
        std::error_code(errno, std::system_category()));
  }
}

void FileHandle::remove(const std::string& key, region r)
{
  //If a file is passed as key and it exists, remove it.
  auto fpath = get_key_path(key, r);
  if (fs::exists(fpath)) {
    fs::remove(fpath);
  } else {   //if a regex is passed, handle it if possible
    FileHandle::path p = r == region::persist ? kv_store : cache_store;
    const std::regex search(key);
    std::smatch match;
    bool keyfound = false;
    //Recurse through the directory for files that match the regex
    for (auto & next_dir_entry: fs::recursive_directory_iterator(p)) {
      if (fs::is_regular_file(next_dir_entry)) {
        std::string testname = next_dir_entry.path().string().substr(p.string().length() + 1);
        //If the found file matches the pattern, remove it
        if (std::regex_match(testname, match, search)) {
          fs::remove(next_dir_entry);
          keyfound = true;
        }
      }
    }
    if (!keyfound) {
      throw kv::key_does_not_exist(key);
    }
  }
}

} // namespace kv
