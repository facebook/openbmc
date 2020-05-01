/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright 2015-present Facebook. All Rights Reserved.
 */

#include <syslog.h>
#include <limits>

#include "kv.hpp"
#include "fileops.hpp"
#include "log.hpp"

using namespace kv;

/*
*  set key::value
*  len is the size of value. If 0, it is assumed value is
*      a string and strlen() is used to determine the length.
*  flags is bitmask of options.
*
*  return 0 on success, negative error code on failure.
*/
int kv_set(const char *key, const char *value, size_t len, unsigned int flags) {
  if (key == nullptr || value == nullptr) {
    errno = EINVAL;
    return -1;
  }

  /* Length of zero implies we should treat it like a string. */
  if (len == 0) {
    /* The typical buffer allocated is exactly MAX_VALUE_LEN bytes, so we
     * cannot go past this when calculating strlen. */
    len = strnlen(value, MAX_VALUE_LEN);

    /* Unfortunately, this means we do not know if the buffer was null
     * terminated at value[MAX_VALUE_LEN] or missing a null terminator
     * (and we cannot look at it without possibly exceeding the buffer).
     * Assume it is missing and give E2BIG error. */
    if (len >= MAX_VALUE_LEN) {
      errno = E2BIG;
      return -1;
    }
  }
  if (len > MAX_VALUE_LEN) {
    errno = E2BIG;
    return -1;
  }

  try {
    std::string data{value, value + len};
    auto r = flags & KV_FPERSIST ? region::persist : region::temp;
    kv::set(key, data, r, flags & KV_FCREATE);

  } catch (std::exception& e) {
    KV_WARN("kv_set: %s", e.what());
    return -1;
  }

  return 0;
}

/*
*  get key::value
*  len is the return size of value. If NULL then this information
*      is not returned to the user (And assumed to be a string).
*  flags is bitmask of options.
*
*  return 0 on success, negative error code on failure.
*/
int kv_get(const char *key, char *value, size_t *len, unsigned int flags) {
  if (key == nullptr || value == nullptr) {
    errno = EINVAL;
    return -1;
  }

  try {
    auto r = flags & KV_FPERSIST ? region::persist : region::temp;
    auto result = kv::get(key, r);
    auto bytes = result.size();

    // result is required to be less than or equal to 'MAX_VALUE_LEN' and
    // value is required to have enough space to store at least that, so
    // this copy is always safe.
    std::copy(std::begin(result), std::end(result), value);

    // Update length variable.
    if (len != nullptr)
      *len = bytes;
    // If no length was given, treat it as a string.  Ensure we have a null
    // terminator.
    else if ((bytes + 1) < max_len)
      value[bytes] = '\0';
    else if (value[max_len - 1] != '\0') {
      KV_WARN("kv_get: truncating string-type value for key %s with length %zd",
          key, bytes);
      value[max_len - 1] = '\0';
    }

  } catch (std::exception& e) {
    KV_WARN("kv_get: %s", e.what());
    return -1;
  }

  return 0;
}

namespace kv {

void set(const std::string& key, const std::string& value,
         region r, bool require_create)
{

  FileHandle fp;
  fp.open_and_lock<FileHandle::access::write>(key, r);


  if (fp.was_present() && require_create) {
    throw std::logic_error("kv_set: key " + key + " already exists");
  }

  // Check if we are writing the same value. If so, exit early
  // to save on number of times flash is updated.
  if (fp.was_present() && r == region::persist && (fp.read() == value)) {
    return;
  }

  fp.write(value);
}

std::string get(const std::string& key, region r)
{
  FileHandle fp;
  fp.open_and_lock<FileHandle::access::read>(key, r);

  return fp.read();
}

} // namespace kv
