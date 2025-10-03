#!/bin/bash

logger "Restart eth0 after MAC and Linklock addr update"
ifdown eth0
sleep 1
ifup eth0 &
