#!/usr/bin/env python3
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

# No enum in BMC's version of python


class CardType:
    LC = "lc"
    FC = "fc"
    CMM = "cmm"


class BmcCard:
    def __init__(self, card_type, addr_hextet):
        self.card_type = card_type
        self.addr_hextet = addr_hextet
        self.has_scm = card_type in [CardType.LC, CardType.FC]

    def link_local_addr(self):
        return "fe80::{}:1%eth0.4088".format(self.addr_hextet)

    def __repr__(self):
        return "{}-{}".format(self.card_type, self.addr_hextet)


BACKPACK_ALL_CARDS = [
    BmcCard(CardType.LC, "101"),
    BmcCard(CardType.LC, "102"),
    BmcCard(CardType.LC, "201"),
    BmcCard(CardType.LC, "202"),
    BmcCard(CardType.LC, "301"),
    BmcCard(CardType.LC, "302"),
    BmcCard(CardType.LC, "401"),
    BmcCard(CardType.LC, "402"),
    BmcCard(CardType.FC, "1"),
    BmcCard(CardType.FC, "2"),
    BmcCard(CardType.FC, "3"),
    BmcCard(CardType.FC, "4"),
    BmcCard(CardType.CMM, "c1"),
]
