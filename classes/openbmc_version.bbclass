# Copyright 2018-present Facebook. All Rights Reserved.

def get_openbmc_version(d):
    import os
    import oe.utils
    machine = d.getVar('MACHINE', True)
    version_suffix = d.getVar('OPENBMC_VERSION_SUFFIX', True) or ''
    version = '%s%s-v0.0' % (machine, version_suffix)
    cur = os.path.realpath(d.getVar('COREBASE', True))
    is_openbmc_root = lambda cur: \
        os.path.isdir(os.path.join(cur, '.git')) and \
        os.path.isfile(os.path.join(cur, 'openbmc-init-build-env'))

    while cur and cur != '/' and not is_openbmc_root(cur):
        cur = os.path.dirname(cur)

    bb.debug(2, 'Found OpenBMC root %s, is_openbmc=%s'
             % (cur, cur and is_openbmc_root(cur)))
    gitdir = os.path.join(cur, '.git')
    if cur and is_openbmc_root(cur):
        version = ''
        git_cmd = ['export PSEUDO_DISABLED=1;', \
                   'git', '--git-dir=%s' % gitdir , '--work-tree=%s' % cur]
        tags_cmd = git_cmd + ['tag', '--points-at', 'HEAD']
        exitstatus, output = oe.utils.getstatusoutput(' '.join(tags_cmd))
        if exitstatus == 0:
            fmtstr = '%s-v' % machine
            tags = output.splitlines()
            for tag in reversed(tags):
                if fmtstr in tag:
                    # Insert version suffix between machine name and version
                    # e.g. "yosemite-v2024.13.0" -> "yosemite_s-v2024.13.0"
                    if version_suffix:
                        version = tag.replace(fmtstr, '%s%s-v' % (machine, version_suffix), 1)
                    else:
                        version = tag
                    break
        if version == '':
            cmd = git_cmd + ['rev-parse', '--short', 'HEAD']
            exitstatus, output = oe.utils.getstatusoutput(' '.join(cmd))
            if exitstatus == 0:
                version = '%s%s-%s' % (machine, version_suffix, output)
        cmd = git_cmd + ['status', '--short']
        exitstatus, output = oe.utils.getstatusoutput(' '.join(cmd))
        if exitstatus == 0 and output.strip() != "":
          version += '-dirty'
    return version

OPENBMC_VERSION := "${@get_openbmc_version(d)}"
DISTRO_VERSION := "${OPENBMC_VERSION}"
