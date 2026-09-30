HOMEPAGE = "https://github.com/AspeedTech-BMC/u-boot"
SECTION = "bootloaders"
DEPENDS += "flex-native bison-native"

LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://Licenses/README;md5=2ca5f2c35c8cc335f0a19756634782f1"
PE = "1"

BRANCH = "aspeed-master-v2023.10"
SRC_URI = "git://github.com/AspeedTech-BMC/u-boot.git;protocol=https;branch=${BRANCH}"

# Tag for v00.05.13
SRCREV = "03a8c08746a657145b036ab61ba1d299394cce74"

B = "${UNPACKDIR}/build"
do_configure[cleandirs] = "${B}"

# We don't have 'u-boot-initial-env' target in our u-boot version
UBOOT_INITIAL_ENV = ""

PV = "v2023.10+git"

python do_set_local_version() {
    import os
    import re
    s = d.getVar("S")
    b = d.getVar("B")
    try:
        os.remove('%s/.scmversion' % s)
        os.remove('%s/.scmversion' % b)
    except:
        pass

    scm_ver = ""
    sub_ver = "none"

    try:
        res=bb.process.run("git -C %s describe --tags --exact-match" %  s)[0].strip("\n")

        if res.find("devtool") != -1:
            version_ext = bb.process.run("git -C %s rev-parse --verify --short HEAD" % s)[0].strip("\n")
            scm_ver = ('-%s-%s' % res, version_ext)
        else:
            scm_ver = ('-%s' % res)
    except Exception as e:
        version=bb.process.run("git -C %s rev-parse --verify --short HEAD" % s)[0].strip("\n")
        scm_ver = ('-dirty-%s' % version)

    with open("%s/.scmversion" % s, "a") as f:
        f.write(scm_ver)
    with open("%s/.scmversion" % b, "a") as f:
        f.write(scm_ver)

    match = re.search(r'[^-]*$', scm_ver)
    if match:
        sub_ver = match.group(0)

    with open("%s/.subversion" % s, "a") as f:
        f.write(sub_ver)
    with open("%s/.subversion" % b, "a") as f:
        f.write(sub_ver)
}

addtask set_local_version before do_compile after do_configure
