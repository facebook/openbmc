# Copyright (c) Meta Platforms, Inc. and affiliates.

CFLAGS:remove = "-DEEPROM_BUS=0x0B"
CFLAGS:prepend = "-DEEPROM_BUS=0x03 "