# boost.context ships hand-written ARM assembly for its fcontext routines.
# On the armv6 aspeed-g5 SoCs (e.g. AST2500/AST2520, arm1136/arm1176) the
# assembler has no movw/movt and emits absolute relocations into .text, which
# trips the [textrel] package QA check and fails do_package_qa.
#
# Upstream openembedded-core carries an identical skip in its meta-aspeed
# boost bbappend, but we build against our own top-level meta-aspeed layer and
# do not include the upstream one in BBLAYERS, so mirror the workaround here.
INSANE_SKIP:${PN}-context:aspeed-g5 = "textrel"
