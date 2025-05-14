FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

FW_TOOLS = "\
    bmc-tpm,clemente-sys-init.service,,multi-user.target,0 \
    nic0,network-wait-ipv6-ll@eth0.service,,multi-user.target,0 \
    nic1,network-wait-ipv6-ll@eth1.service,,multi-user.target,0 \
    pdb-vr-aux,clemente-sys-init.service,,multi-user.target,0 \
    pdb-cpld,clemente-sys-init.service,,multi-user.target,0 \
    pdb-vr-n1,clemente-sys-init.service,,multi-user.target,0 \
    pdb-vr-n2,clemente-sys-init.service,,multi-user.target,0 \
    scm-cpld,clemente-sys-init.service,,multi-user.target,0 \
    hdd-cpld,clemente-sys-init.service,,multi-user.target,0 \
    hmc-hgx-fw-bmc-0,clemente-sys-init.service,,multi-user.target,0 \
    hmc-hgx-fw-cpld-0,clemente-sys-init.service,,multi-user.target,0 \
    hmc-hgx-fw-cpu-0,clemente-sys-init.service,,multi-user.target,0 \
    hmc-hgx-fw-cpu-1,clemente-sys-init.service,,multi-user.target,0 \
    hmc-hgx-fw-erot-bmc-0,clemente-sys-init.service,,multi-user.target,0 \
    hmc-hgx-fw-erot-cpu-0,clemente-sys-init.service,,multi-user.target,0 \
    hmc-hgx-fw-erot-cpu-1,clemente-sys-init.service,,multi-user.target,0 \
    hmc-hgx-fw-erot-fpga-0,clemente-sys-init.service,,multi-user.target,0 \
    hmc-hgx-fw-erot-fpga-1,clemente-sys-init.service,,multi-user.target,0 \
    hmc-hgx-fw-fpga-0,clemente-sys-init.service,,multi-user.target,0 \
    hmc-hgx-fw-fpga-1,clemente-sys-init.service,,multi-user.target,0 \
    hmc-hgx-fw-gpu-0,clemente-sys-init.service,,multi-user.target,0 \
    hmc-hgx-fw-gpu-1,clemente-sys-init.service,,multi-user.target,0 \
    hmc-hgx-inforom-gpu-0,clemente-sys-init.service,,multi-user.target,0 \
    hmc-hgx-inforom-gpu-1,clemente-sys-init.service,,multi-user.target,0 \
    hmc-hgx-pcieswitchconfig-0,clemente-sys-init.service,,multi-user.target,0 \
"

CLEMENTE_ADD = "\
    bmc-tpm \
    hdd-cpld \
    hmc-hgx-fw-gpu-1 \
    hmc-hgx-inforom-gpu-1 \
    pdb-cpld \
    pdb-vr-aux \
    pdb-vr-n1 \
    pdb-vr-n2 \
    scm-cpld \
"

LOCAL_URI:append = " \
    ${@ ' '.join([ ('' if x.split(',')[0] not in d.getVar('CLEMENTE_ADD').split() else (f"file://" + x.split(',')[0])) \
    for x in d.getVar('FW_TOOLS', True).split() ])} \
"
