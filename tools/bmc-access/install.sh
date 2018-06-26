#!/bin/bash

INSTALL_LOCATION=~/bin
if [ ! -z "$1" ]; then
  INSTALL_LOCATION=$1
fi

if [ ! -d $INSTALL_LOCATION ]; then
  echo "WARNING: Creating $INSTALL_LOCATION"
  mkdir -p $INSTALL_LOCATION
fi

dir=$(dirname ${BASH_SOURCE[0]})
files=$(find $dir -name 'bmc*')
for file in ${files}; do
  if [ ! -d $file ]; then
    cp $file $INSTALL_LOCATION/
    name=$(basename $file)
    chmod +x $INSTALL_LOCATION/$name
  fi
done
