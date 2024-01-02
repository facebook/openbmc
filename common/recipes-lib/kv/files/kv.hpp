#pragma once

/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright 2020-present Facebook. All Rights Reserved.
 */
#include <stdexcept>
#include <string>
#if (__GNUC__ < 8)
#include <experimental/filesystem>
namespace std {
namespace filesystem = experimental::filesystem;
}
#else
#include <filesystem>
#endif

#include "kv.h"

namespace kv {

static constexpr auto max_len = MAX_VALUE_LEN;
enum class region { persist, temp };

std::string get(const std::string& key, region r = region::temp);
void set(const std::string& key, const std::string& value,
    region r = region::temp, bool require_create = false);
void del(const std::string& key, region r = region::temp);

struct key_already_exists : public std::filesystem::filesystem_error {
  explicit key_already_exists(const std::string& msg)
      : std::filesystem::filesystem_error(
            msg,
            std::error_code(EEXIST, std::system_category())) {}
};

struct key_does_not_exist : public std::filesystem::filesystem_error {
  explicit key_does_not_exist(const std::string& msg)
      : std::filesystem::filesystem_error(
            msg,
            std::error_code(ENOENT, std::system_category())) {}
};

} // namespace kv
