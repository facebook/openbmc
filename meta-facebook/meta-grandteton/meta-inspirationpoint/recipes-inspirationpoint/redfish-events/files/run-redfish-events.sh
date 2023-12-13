#!/bin/bash
config=$(/usr/bin/kv get gpu_config persistent)
if [ "${config}" == "ubb" ]; then
  rm /etc/redfish-events.cfg
  ln -s /etc/redfish-events-ubb.cfg /etc/redfish-events.cfg
fi
exec /usr/bin/redfish-events.py
