#pragma once

/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright 2020-present Facebook. All Rights Reserved.
 */
#include <stdexcept>
#include <string>
#include <vector>
#include "kv.h"

namespace kv {

static constexpr auto max_len = MAX_VALUE_LEN;
enum class region { persist, temp };

std::string get(const std::string& key, region r = region::temp);
void set(const std::string& key, const std::string& value,
         region r = region::temp, bool require_create = false);

struct key_already_exists : public std::logic_error {
    using logic_error::logic_error;
};

} // namespace kv
