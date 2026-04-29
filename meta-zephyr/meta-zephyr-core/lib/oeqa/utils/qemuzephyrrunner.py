# Copyright (C) 2015-2017 Intel Corporation
#
# Released under the MIT license (see COPYING.MIT)

# This module provides a class for starting qemu images.
# It's used by testimage.bbclass.

import subprocess
import os
import time
import signal
import socket
import select
import bb
import tempfile
import sys
import configparser
from oeqa.utils.qemurunner import QemuRunner

class QemuZephyrRunner(QemuRunner):

    def __init__(self, machine, rootfs, display, tmpdir, deploy_dir_image, logfile, boottime, dump_dir, dump_host_cmds, use_kvm, logger, tmpfsdir):


        QemuRunner.__init__(self, machine, rootfs, display, tmpdir,
                            deploy_dir_image, logfile, boottime, None,
                            None, use_kvm, logger, tmpfsdir)

        # Popen object for runqemu
        self.socketfile = tempfile.NamedTemporaryFile()
        self.runqemu = None
        self.socketname = self.socketfile.name
        self.server_socket = None

        self.kernel = rootfs
        self.deploy_dir_image = deploy_dir_image
        self.tmpfsdir = tmpfsdir
        self.logfile = logfile
        self.use_kvm = use_kvm

        self.buffers = b''
        self._rbufsize = 4096
        # 5 minutes timeout...
        self.endtime = time.time() + 60*5

        self.qemuboot = False
        self.d = {'QB_KERNEL_ROOT': '/dev/vda'}

    def get(self, key):
        if key in self.d:
            return self.d.get(key)
        elif os.getenv(key):
            return os.getenv(key)
        else:
            return ''

    def set(self, key, value):
        self.d[key] = value

    def read_qemuboot(self):
        if not self.qemuboot:
            if self.get('DEPLOY_DIR_IMAGE'):
                deploy_dir_image = self.get('DEPLOY_DIR_IMAGE')
            else:
                bb.warning("Can't find qemuboot conf file, DEPLOY_DIR_IMAGE is NULL!")
                return

            if self.rootfs and not os.path.exists(self.rootfs):
                # Lazy rootfs
                machine = self.get('MACHINE')
                if not machine:
                    machine = os.path.basename(deploy_dir_image)
                self.qemuboot = "%s/%s-%s.qemuboot.conf" % (deploy_dir_image,
                        self.rootfs, machine)
            else:
                cmd = 'ls -t %s/*.qemuboot.conf' %  deploy_dir_image
                try:
                    qbs = subprocess.check_output(cmd, shell=True).decode('utf-8')
                except subprocess.CalledProcessError as err:
                    raise RunQemuError(err)
                if qbs:
                    for qb in qbs.split():
                        # Don't use initramfs when other choices unless fstype is ramfs
                        if '-initramfs-' in os.path.basename(qb) and self.fstype != 'cpio.gz':
                                continue
                        self.qemuboot = qb
                        break
                    if not self.qemuboot:
                        # Use the first one when no choice
                        self.qemuboot = qbs.split()[0]
                    self.qbconfload = True

        if not self.qemuboot:
            # If we haven't found a .qemuboot.conf at this point it probably
            # doesn't exist, continue without
            return

        if not os.path.exists(self.qemuboot):
            raise RunQemuError("Failed to find %s (wrong image name or BSP does not support running under qemu?)." % self.qemuboot)

        cf = configparser.ConfigParser()
        cf.read(self.qemuboot)
        for k, v in cf.items('config_bsp'):
            k_upper = k.upper()
            if v.startswith("../"):
                v = os.path.abspath(os.path.dirname(self.qemuboot) + "/" + v)
            elif v == ".":
                v = os.path.dirname(self.qemuboot)
            self.set(k_upper, v)


    def create_socket(self):
        bb.note("waiting at most %s seconds for qemu pid" % self.runqemutime)
        tries = self.runqemutime
        while tries > 0:
            time.sleep(1)
            try:
                self.server_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                self.server_socket.connect(self.socketname)
                bb.note("Created listening socket for qemu serial console.")
                break

            except socket.error:
                self.server_socket.close()
                tries -= 1

        if tries == 0:
            bb.error("Failed to create listening socket %s: " % (self.socketname))
            return False
        return True

    def start(self, params=None,runqemuparams=None, extra_bootparams=None):

        if not os.path.exists(self.tmpdir):
            bb.error("Invalid TMPDIR path %s" % self.tmpdir)
            return False
        else:
            os.environ["OE_TMPDIR"] = self.tmpdir
        if not os.path.exists(self.deploy_dir_image):
            bb.error("Invalid DEPLOY_DIR_IMAGE path %s" % self.deploy_dir_image)
            return False
        else:
            os.environ["DEPLOY_DIR_IMAGE"] = self.deploy_dir_image
        if self.tmpfsdir:
            env["RUNQEMU_TMPFS_DIR"] = self.tmpfsdir

        if not os.path.exists(self.kernel):
            bb.error("Invalid kernel path: %s" % self.kernel)
            return False

        self.qemuparams = '-serial unix:%s,server' % (self.socketname)

        self.read_qemuboot()
        qemu_binary = self.get('QB_SYSTEM_NAME')
        qemu_machine_args = self.get('QB_MACHINE')
        if qemu_binary == "" or qemu_machine_args == "":
            bb.error("Unsupported QEMU: %s" % self.machine)
            return False

        self.qemuparams += " %s " %self.get('QB_GRAPHICS')
        self.qemuparams += " %s " %self.get('QB_OPT_APPEND')
        self.qemuparams += " %s " %self.get('QB_CPU')

        self.origchldhandler = signal.getsignal(signal.SIGCHLD)
        signal.signal(signal.SIGCHLD, self.handleSIGCHLD)

        launch_cmd = '%s -kernel %s %s %s' % (qemu_binary, self.kernel, self.qemuparams, qemu_machine_args)
        bb.note(launch_cmd)
        self.runqemu = subprocess.Popen(launch_cmd,shell=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,preexec_fn=os.setpgrp)

        #
        # We need the preexec_fn above so that all runqemu processes can easily be killed
        # (by killing their process group). This presents a problem if this controlling
        # process itself is killed however since those processes don't notice the death
        # of the parent and merrily continue on.
        #
        # Rather than hack runqemu to deal with this, we add something here instead.
        # Basically we fork off another process which holds an open pipe to the parent
        # and also is setpgrp. If/when the pipe sees EOF from the parent dieing, it kills
        # the process group. This is like pctrl's PDEATHSIG but for a process group
        # rather than a single process.
        #
        r, w = os.pipe()
        self.monitorpid = os.fork()
        if self.monitorpid:
            os.close(r)
            self.monitorpipe = os.fdopen(w, "w")
        else:
            # child process
            os.setpgrp()
            os.close(w)
            r = os.fdopen(r)
            x = r.read()
            os.killpg(os.getpgid(self.runqemu.pid), signal.SIGTERM)
            os._exit(0)

        bb.note("qemu started, pid is %s" % self.runqemu.pid)
        return self.create_socket()

    def _readline(self):
        nl = self.buffers.find(b'\n')
        if nl >= 0:
            nl += 1
            line = self.buffers[:nl]
            newbuf = self.buffers[nl:]
            self.buffers = newbuf
            return line
        return None

    def serial_readline(self):
        line = self._readline()
        if line is None:
            while True:
                if time.time() >= self.endtime:
                    bb.warn("Timeout!")
                    raise Exception("Timeout")
                data = self.server_socket.recv(self._rbufsize)
                if data is None:
                    raise Exception("No data on read ready socket")

                self.buffers = self.buffers + data
                line = self._readline()
                if line is not None:
                    break

        self.log(line)
        return line
