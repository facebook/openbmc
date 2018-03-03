# Copyright 2018-present Facebook. All Rights Reserved.

def get_openbmc_version(d):
    import os
    import oe.utils
    machine = d.getVar('MACHINE', True)
    version = '%s-v0.0' % machine
    cur = os.path.realpath(os.getcwd())
    is_openbmc_root = lambda cur: \
        os.path.isdir(os.path.join(cur, '.git')) and \
        os.path.isfile(os.path.join(cur, 'openbmc-init-build-env'))

    while cur and cur != '/' and not is_openbmc_root(cur):
        cur = os.path.dirname(cur)

    bb.debug(2, 'Found OpenBMC root %s, is_openbmc=%s'
             % (cur, cur and is_openbmc_root(cur)))
    gitdir = os.path.join(cur, '.git')
    if cur and is_openbmc_root:
        cmd = ['git', '--git-dir=%s' % gitdir , '--work-tree=%s' % cur,
               'describe', '--tags', '--dirty', '--always']
        exitstatus, output = oe.utils.getstatusoutput(' '.join(cmd))
        if exitstatus != 0:
            output = ''
        fmtstr = '%s-v' % machine
        if fmtstr in output:
            version = output
        else:
            cmd = ['git', '--git-dir=%s' % gitdir,
                   '--work-tree=%s' % cur,
                   'rev-parse', '--short', 'HEAD']
            exitstatus, output = oe.utils.getstatusoutput(' '.join(cmd))
            if exitstatus == 0:
                version = '%s-%s' % (machine, output)
    return version


OPENBMC_VERSION := "${@get_openbmc_version(d)}"
