/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright 2015-present Facebook. All Rights Reserved.
 */

#include <array>
#include <cassert>
#include <unistd.h>

#include "kv.hpp"

int main(int argc, char *argv[])
{
  char value[MAX_VALUE_LEN*2];
  size_t len;

  assert(kv_get("test1", value, NULL, KV_FPERSIST) != 0);
  printf("SUCCESS: Non-existent file results in error.\n");

  assert(kv_set("test1", "val", 0, KV_FPERSIST) == 0);
  printf("SUCCESS: Creating persist key func call\n");
  assert(access("./test/persist/test1", F_OK) == 0);
  printf("SUCCESS: key file created as expected!\n");
  assert(kv_get("test1", value, NULL, KV_FPERSIST) == 0);
  printf("SUCCESS: Read of key succeeded!\n");
  assert(strcmp(value, "val") == 0);
  printf("SUCCESS: Read key is of value what was expected!\n");
  assert(kv_set("test1", "val", 0, KV_FCREATE | KV_FPERSIST) != 0);
  printf("SUCCESS: KV_FCREATE failed on existing persistent key.\n");

  assert(kv_set("test1", "val", 0, 0) == 0);
  printf("SUCCESS: Creating non-persist key func call\n");
  assert(access("./test/tmp/test1", F_OK) == 0);
  printf("SUCCESS: key file created as expected!\n");
  assert(kv_get("test1", value, NULL, 0) == 0);
  printf("SUCCESS: Read of key succeeded!\n");
  assert(strcmp(value, "val") == 0);
  printf("SUCCESS: Read key is of value what was expected!\n");

  assert(kv_set("test3/test", "test3-1234", 0, 0) == 0);
  printf("SUCCESS: Creating non-persist key in subdirectory\n");
  assert(access("./test/tmp/test3/test", F_OK) == 0);
  printf("SUCCESS: key file created as expected!\n");
  assert(kv_get("test3/test", value, NULL, 0) == 0);
  printf("SUCCESS: Read of key succeeded!\n");
  assert(strcmp(value, "test3-1234") == 0);
  printf("SUCCESS: Read key is of value what was expected!\n");

  assert(kv_set("test1", "val", 2, KV_FPERSIST) == 0);
  printf("SUCCESS: Updating key with len\n");
  len = 0;
  memset(value, 0, sizeof(value));
  assert(kv_get("test1", value, &len, KV_FPERSIST) == 0);
  printf("SUCCESS: Read of key succeeded!\n");
  assert(strcmp(value, "va") == 0);
  assert(len == 2);
  printf("SUCCESS: Read key is of value & len what was expected!\n");

  assert(kv_set("test1", "v2", 0, KV_FCREATE) != 0);
  memset(value, 0, sizeof(value));
  assert(kv_get("test1", value, NULL, 0) == 0);
  assert(strcmp(value, "val") == 0);
  printf("SUCCESS: KV_FCREATE failed on existing key and did not modify existing value\n");

  assert(kv_set("test2", "val2", 0, KV_FCREATE) == 0);
  memset(value, 0, sizeof(value));
  assert(kv_get("test2", value, NULL, 0) == 0);
  assert(strcmp(value, "val2") == 0);
  printf("SUCCESS: KV_FCREATE succeeded on non-existing key\n");

  memset(value, 'a', sizeof(value));
  errno = 0;
  assert(kv_set("test2", value, 0, 0) < 0);
  assert(errno == E2BIG);
  printf("SUCCESS: string longer than MAX_VALUE_LEN caught\n");

  memset(value, 'a', sizeof(value));
  errno = 0;
  assert(kv_set("test2", value, MAX_VALUE_LEN+1, 0) < 0);
  assert(errno == E2BIG);
  printf("SUCCESS: non-string longer than MAX_VALUE_LEN caught\n");

  memset(value, 'a', sizeof(value));
  assert(kv_set("test2", "asdfasdf", 4, 0) == 0);
  assert(kv_get("test2", value, NULL, 0) == 0);
  assert(strcmp(value, "asdf") == 0);
  printf("SUCCESS: Read key adds nul terminator for strings\n");

  {
    constexpr auto key = "test4";
    std::array<char, 7> v = { 'a', 'b', 'c', '\0', 'd', 'e', 'f' };
    size_t len = v.size();

    assert(kv_set(key, v.data(), len, 0) == 0);
    assert(kv_get(key, value, &len, 0) == 0);
    assert(len == v.size());
    assert(memcmp(value, v.data(), len) == 0);
    printf("SUCCESS: Read and write raw binary data.\n");

    auto s = kv::get(key);
    assert(s.size() == v.size());
    assert(memcmp(s.data(), v.data(), v.size()) == 0);
    printf("SUCCESS: Read raw binary using C++ interface.\n");
  }

  {
    constexpr auto key = "test5";
    auto s = "this is a test";

    kv::set(key, s);
    auto s2 = kv::get(key);

    assert(s == s2);
    printf("SUCCESS: Read and write using C++ interface.\n");
  }

  assert(system("rm -rf ./test") == 0);

  return 0;
}

