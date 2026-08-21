# ====================================================================
# Wedge400 AST2500 Exclusive Python 3.14 Slimming Performance Boost Patch
# ====================================================================

# Use build server performance during compilation to achieve speedup for the onboard Python 3.14 interpreter
PACKAGECONFIG:append:class-target = " lto pgo"

# Remove the -g debugging information and upgrade to -O3
SELECTED_OPTIMIZATION:pn-python3 = "-O3 -pipe"

# Avoid throwing error when do_package_qa.
INSANE_SKIP:${PN}-dbg += "buildpaths"
INSANE_SKIP:${PN}-src += "buildpaths"
INSANE_SKIP:${PN} += "buildpaths"