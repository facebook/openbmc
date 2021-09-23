# meta-phosphor has a BUILD_ID, which comes from `git describe`.  Since we have
# many tags, for many different machines, in our repo this ends up often
# pointing to a tag for a different machine.  Just remove it so it doesn't end
# up in `/etc/os-release` until we can fix upstream to make it work better.
OS_RELEASE_FIELDS:remove = "BUILD_ID"
