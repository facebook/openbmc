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
#

import asyncio
import collections


class AsyncRateLimiter:
    """
    Async ratelmiter records successful requests per client,
    method and endpoint into a counter.
    Ratelimits (if enabled) are applied for all endpoints.
    Ratelimiter is configurable from rest.cfg (slidewindow size and request limit)
    If a client hits limit within the slidewindow, requests will be denied,
    until older request records time out. Then requests are allowed again.
    """

    def __init__(self, slidewindow_size: int, limit: int):
        self._request_counter = collections.Counter()
        self.slidewindow_size = slidewindow_size
        self.limit = limit

    async def is_limited(self, endpoint: str, method: str, user_agent: str) -> bool:
        """
        Check if request has hit the rate limit for given slidewindow.
        """
        # If the limit is set to 0, it means that we dont want ratelimiting. Bail early
        if self.limit == 0:
            return False
        if self._request_counter[(endpoint, method, user_agent)] >= self.limit:
            return True
        else:
            self._request_counter[(endpoint, method, user_agent)] += 1
            asyncio.ensure_future(self._cleanup(endpoint, method, user_agent))
            return False

    async def _cleanup(self, endpoint: str, method: str, user_agent: str):
        await asyncio.sleep(self.slidewindow_size)
        self._request_counter[(endpoint, method, user_agent)] -= 1
