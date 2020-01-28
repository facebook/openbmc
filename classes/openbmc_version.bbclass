# Copyright 2018-present Facebook. All Rights Reserved.

def get_openbmc_version(d):
    import os
    import oe.utils
    machine = d.getVar('MACHINE', True)
    version = '%s-v0.0' % machine
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
        git_cmd = ['git', '--git-dir=%s' % gitdir , '--work-tree=%s' % cur]
        tags_cmd = git_cmd + ['tag', '--points-at', 'HEAD']
        exitstatus, output = oe.utils.getstatusoutput(' '.join(tags_cmd))
        if exitstatus == 0:
            fmtstr = '%s-v' % machine
            tags = output.splitlines()
            for tag in reversed(tags):
                if fmtstr in tag:
                    version = tag
                    break
        if version == '':
            cmd = git_cmd + ['rev-parse', '--short', 'HEAD']
            exitstatus, output = oe.utils.getstatusoutput(' '.join(cmd))
            if exitstatus == 0:
                version = '%s-%s' % (machine, output)
        cmd = git_cmd + ['status', '--short']
        exitstatus, output = oe.utils.getstatusoutput(' '.join(cmd))
        if exitstatus == 0 and output.strip() != "":
          version += '-dirty'
    return version

OPENBMC_VERSION := "${@get_openbmc_version(d)}"
