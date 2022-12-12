# Copyright 2015-present Facebook. All Rights Reserved.

do_install:append() {
  update-rc.d -f -r ${D} setup-ipmbd-cover.sh remove
}
