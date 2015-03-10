# eglibc-mtrace is a Perl script analyzing the mtrace output data.
# It is small but depends on perl, which increases image size by 1.35M.
# Remove it explicitly. When we need to use it, we can always copy the
# mtrace results out and run mtrace utility outside of BMC.
MTRACE = ""
MTRACE_libc-glibc = ""
