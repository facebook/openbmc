#!/usr/bin/env python
#
# Copyright 2016-present Facebook. All Rights Reserved.
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

import syslog
import subprocess
import bmc_command

def _parse_eeprom_data(output):
  eeprom_data = {}

  for line in output.splitlines():
    parts = line.split(':', 1)
    if len(parts) != 2:
      # This generally shouldn't happen, but it's possible there was some
      # error message logged on the console in the middle of the w|seutil
      # output.
      continue
    key = parts[0].strip()
    value = parts[1].strip()
    eeprom_data[key] = value

  return eeprom_data

def _get_eeprom_data(util):
  # syslog.openlog(ident=__name__, logoption=syslog.LOG_PID)
  proc = subprocess.Popen([util],
                          stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE)

  try:
    # bmc_command will kill an errant timed out process for us
    data, err = bmc_command.timed_communicate(proc)
  except bmc_command.TimeoutError as ex:
      # syslog.syslog(syslog.LOG_ERR, '{}: {}. {} process timed out.'.format(
      #     ex.error, ex.output, util))
    raise

  return _parse_eeprom_data(data)


def get_fruid_scm():
  weutil_data = _get_eeprom_data('weutil')
  seutil_data = _get_eeprom_data('seutil')

  # Get location and BMC SN info from weutil, put in seutil data
  seutil_data['Location on Fabric'] = weutil_data['Location on Fabric']
  seutil_data['BMC Serial'] = weutil_data['Product Serial Number']

  return seutil_data
