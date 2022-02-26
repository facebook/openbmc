## Supplying local source and placing it into ${WORKDIR} causes conflicts with
## `pseudo` such that modifying the source and rebuilding causes a `pseudo
## abort`.  In order to work around that, it is best to use the fetcher to place
## the source in a non-WORKDIR location.
##
## This bbclass provides an easier way to specify that the local sources should
## go into ${S} (source location) instead of ${WORKDIR} (package root working
## directory).  Instead of placing the sources into SOURCE_URI, place them in
## LOCAL_URI and we will automatically append a `;subdir=${S}` to them to tell
## the fetcher to place them there.

local_files_to_sourcedir[vardeps] += "LOCAL_URI"
def local_files_to_sourcedir(d):
    uris = d.getVar('LOCAL_URI') or ""
    result = []
    for u in uris.split():
        if "file://" in u and ";subdir" not in u:
            # In order to avoid bitbake interpretation of the S variable, and
            # claiming a false variable dependency, split the string up.
            # bitbake will later expand the S in SRC_URI expansion.
            result.append(u + ";subdir=$" + "{S}")
        else:
            result.append(u)
    if len(result):
        return " " + " ".join(result)
    return ""

SRC_URI:append = "${@local_files_to_sourcedir(d)}"

# Check any package that we end up installing to see if it is set to have
# `S == WORKDIR` and is using C/C++ source, which ends up in the debug package.
# If so, raise a warning.
python do_package:append() {
    # If S != WORKDIR, we don't have any potential issues.
    if d.getVar('S') != d.getVar('WORKDIR'):
        return

    if d.getVar('INHIBIT_PACKAGE_DEBUG_SPLIT') == "1":
        return

    uris = d.getVar('SRC_URI') or ""
    for u in uris.split():
        if u.endswith(".c") or u.endswith(".cpp") or \
           u.endswith(".h") or u.endswith(".hpp"):
            bb.warn("SRC_URI contains source files but S == WORKDIR; this has " +
                    "likely pseudo-abort issues: " + u)
}
