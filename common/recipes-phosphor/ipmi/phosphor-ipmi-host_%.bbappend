FW_VER_REGEX="\\d\\d(\\d+)\\.(\\d+)[\\.-](\\d+)(-.*)?"
EXTRA_OEMESON:append = " -Dfw-ver-regex='${FW_VER_REGEX}'"
EXTRA_OEMESON:append = " -Dmatches-map=1,2,3,0,0,0"
