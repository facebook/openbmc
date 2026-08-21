FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# Backport upstream tz fix "Fix C23-related conformance bug"
# (eggert/tz 9cfe9507fcc22cd4a0c4da486ea1c7f0de6b075f). Newer host GCCs report
# __has_c_attribute(noreturn), so tzcode's private.h expands ATTRIBUTE_NORETURN
# (and friends) to the C23 [[noreturn]] form. The pristine 2022g sources place
# these macros mid-declaration (e.g. "static ATTRIBUTE_NORETURN void"), which is
# a syntax error for [[...]] attributes and breaks the native zic/zdump build.
# This patch moves the attributes to the start of the declaration. Already
# present in poky's kirkstone branch; backported here for dunfell.
SRC_URI += " \
    file://0001-Fix-C23-related-conformance-bug.patch \
"
