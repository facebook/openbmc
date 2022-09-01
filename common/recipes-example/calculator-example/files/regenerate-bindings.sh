#!/usr/bin/bash

rm -rf gen/com
sdbus++-gen-meson \
    --command meson \
    --directory yaml \
    --output gen
