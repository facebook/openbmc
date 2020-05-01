#pragma once

/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright 2020-present Facebook. All Rights Reserved.
 */
#include <string>
#include <vector>
#include "kv.h"

namespace kv {

static constexpr auto max_len = MAX_VALUE_LEN;
enum class region { persist, temp };

std::string get(const std::string& key, region r = region::temp);
//std::string get(const std::string& key, size_t length, region r = region::temp);
void set(const std::string& key, const std::string& value,
         region r = region::temp, bool require_create = false);

} // namespace kv
