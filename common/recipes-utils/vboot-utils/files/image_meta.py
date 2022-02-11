#!/usr/bin/env python3
#
# Copyright 2018-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

import hashlib
import json


class StubLogger(object):
    def debug(self, message):
        pass

    def info(self, message):
        pass

    def warn(self, message):
        pass

    def error(self, message):
        pass

    def exception(self, message):
        # This doesn't (yet) print the stack trace.
        pass

    def add_syslog_handler(self, logger):
        # type(object) -> None
        pass

    def get_logger(self):
        # type: () -> object
        logger = StubLogger()
        logger.handlers = []
        return logger


class MetaPartitionNotFound(Exception):
    pass


class MetaPartitionCorrupted(Exception):
    pass


class MetaPartitionVerNotSupport(Exception):
    pass


class MetaPartitionMissingPartInfos(Exception):
    pass


class PartitionNotFound(Exception):
    pass


class FBOBMCImageMeta:
    """
        The image meta schema refer to:
        meta-facebook/recipes-core/images/image-meta-schema.json
        The image-meta is designed to be a raw image partition in the BMC firmware.
        located at 0x000F_0000, with maximum size 64KB.
        This image-meta partition contains two lines of ASCII strings,
        each ASCII string is a JSON:
        First line: The image-meta JSON
        Second line: A simple image-meta-chksum JSON which contain the checksum
                    of image-meta JSON
        Newline (b'\n') is append to both meta and meta-checksum JSON to simplify
        the loading of the JSON objects.

        The example image meta partition dumpped as following with json.tool formatted
        to help read:
        strings /dev/mtd3 | while read line; do echo $line | python -m json.tool; done
        {
            "FBOBMC_IMAGE_META_VER": 1,
            "version_infos": {
                "uboot_build_time": "Aug 11 2020 - 22:16:35 +0000",
                "fw_ver": "fby3vboot2-4f840058283c",
                "uboot_ver": "2019.04"
            },
            "meta_update_action": "Signed",
            "meta_update_time": "2020-08-11T22:20:40.844432",
            "part_infos": [
                {
                    "size": 262144,
                    "type": "rom",
                    "name": "spl",
                    "md5": "602f024562092ac69563f0268ac67265",
                    "offset": 0
                },
                {
                    "size": 655360,
                    "type": "raw",
                    "name": "rec-u-boot",
                    "md5": "5036726386d728e1d37f32702a8f3701",
                    "offset": 262144
                },
                {
                    "size": 65536,
                    "type": "data",
                    "name": "u-boot-env",
                    "offset": 917504
                },
                {
                    "size": 65536,
                    "type": "meta",
                    "name": "image-meta",
                    "offset": 983040
                },
                {
                    "num-nodes": 1,
                    "size": 655360,
                    "type": "fit",
                    "name": "u-boot-fit",
                    "offset": 1048576
                },
                {
                    "num-nodes": 3,
                    "size": 31850496,
                    "type": "fit",
                    "name": "os-fit",
                    "offset": 1703936
                }
            ]
        }
        {
            "meta_md5": "b5a716b8516b3e6e4abb0ca70a535269"
        }

        PS.
        the python json module will encode(save) the tuple into array,
        and decode(load) the array as list
    """

    FBOBMC_IMAGE_META_LOCATION = 0xF0000
    FBOBMC_IMAGE_META_SIZE = 64 * 1024
    FBOBMC_IMAGE_META_VER = 1
    FBOBMC_PART_INFO_KEY = "part_infos"

    def __init__(self, image, logger=None):
        self.image = image
        self.logger = logger
        if self.logger is None:
            self.logger = StubLogger()
        self.meta = self._load_image_meta()

    def _load_image_meta(self):
        self.logger.info("Try loading image meta from %s" % self.image)
        len_remain = self.FBOBMC_IMAGE_META_SIZE
        with open(self.image, "rb") as fh:
            try:
                fh.seek(self.FBOBMC_IMAGE_META_LOCATION)
                meta_data = fh.readline(len_remain)
                meta_data_md5 = hashlib.md5(meta_data.strip()).hexdigest()
                len_remain -= len(meta_data)
                meta_data_chksum = fh.readline(len_remain)
                meta_md5 = json.loads(meta_data_chksum.strip())["meta_md5"]
            except Exception as e:
                raise MetaPartitionNotFound(
                    "Error while attempting to load meta: {}".format(repr(e))
                )

            if meta_data_md5 != meta_md5:
                raise MetaPartitionCorrupted(
                    "Meta partition md5 %s does not match expected %s"
                    % (meta_data_md5, meta_md5)
                )

            meta_info = json.loads(meta_data.strip())
            self.logger.info(
                "loaded image meta ver(%d) %s at %s with chksum '%s' "
                % (
                    meta_info["FBOBMC_IMAGE_META_VER"],
                    meta_info["meta_update_action"],
                    meta_info["meta_update_time"],
                    meta_data_md5,
                )
            )

            if (
                type(meta_info["FBOBMC_IMAGE_META_VER"]) is not int
                or self.FBOBMC_IMAGE_META_VER < meta_info["FBOBMC_IMAGE_META_VER"]
                or meta_info["FBOBMC_IMAGE_META_VER"] <= 0
            ):
                raise MetaPartitionVerNotSupport(
                    "Unsupported meta version {}".format(
                        repr(meta_info["FBOBMC_IMAGE_META_VER"])
                    )
                )

            if self.FBOBMC_PART_INFO_KEY not in meta_info:
                raise MetaPartitionMissingPartInfos(
                    "Required metadata entry '{}' not found".format(
                        self.FBOBMC_PART_INFO_KEY
                    )
                )

            meta_info[self.FBOBMC_PART_INFO_KEY] = tuple(
                meta_info[self.FBOBMC_PART_INFO_KEY]
            )

            return meta_info

    def get_part_info(self, part_name):
        for part_info in self.meta[self.FBOBMC_PART_INFO_KEY]:
            if part_name == part_info["name"]:
                return part_info

        raise PartitionNotFound("partion {} not found".format(part_name))

    def get_partitons(self, part_type):
        part_infos = []
        for part_info in self.meta[self.FBOBMC_PART_INFO_KEY]:
            if part_type == part_info["name"]:
                part_infos.append(part_info)

        return part_infos
