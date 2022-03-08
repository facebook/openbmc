#!/bin/sh
# Copyright 2021-present Facebook. All Rights Reserved.
ln -s /etc/aspeed_uart.conf /etc/rackmon.conf
exec /usr/local/bin/rackmond

