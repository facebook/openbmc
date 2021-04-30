Since openbmc distro disables perl, including recipes which
has ptest which use perl creates an un-buildable dependency

Thus, as a workaround, the bbappends in this subdirectory
specifically disables ptest for these recipes.
