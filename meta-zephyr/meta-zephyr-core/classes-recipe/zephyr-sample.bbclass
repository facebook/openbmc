#
# Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# This bbclass is inherity by all the zephyr-kernel/zephyr-<apps>.bb recipes
# to build zephyr applications. User can also inherit this bbclass from any
# layer for custom or out-of-tree zephyr applications.

require recipes-kernel/zephyr-kernel/zephyr-image.inc
