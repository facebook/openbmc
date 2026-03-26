# SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: LicenseRef-NvidiaProprietary
#
# NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
# property and proprietary rights in and to this material, related
# documentation and any modifications thereto. Any use, reproduction,
# disclosure or distribution of this material and related documentation
# without an express license agreement from NVIDIA CORPORATION or
# its affiliates is strictly prohibited.

SUMMARY = "NVIDIA L4T T23x TCU Muxer"
PR = "r1"
PV = "0.1"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = "git://github.com/NVIDIA/tcu_muxer;protocol=https;branch=develop"
SRCREV = "eceba41c7ba16540f83ac7c13405c41eda7f4ea4"

inherit pkgconfig gettext

EXTRA_OEMAKE = "CC='${CC} ${LDFLAGS}' -C '${S}' CFLAGS='${CFLAGS}'"
FILES:${PN} = "${bindir}/tcu_muxer"

do_install() {
   install -d ${D}/${bindir}
   install -m 0755 ${S}/tcu_muxer ${D}/${bindir}/
}
