# Pin phosphor-ipmi-ssif behind master. Upstream commit 49c757ce adds
# handling for Get System Interface Capabilities in ssifbridge, which
# supersedes the OEM-specific implementation in phosphor-host-ipmid
# used by the Arm Phoenix platform.
SRCREV = "53d76975b49443da877a15d66106119b7133bcbf"
