#!/usr/bin/python3

import argparse
import os
import sys
import time
from shutil import copy
from typing import Tuple

import paramiko
import pexpect
from paramiko.channel import ChannelFile, ChannelStderrFile
from pexpect import EOF, TIMEOUT

PATH = "/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin"
DEFAULT_BMC_USER = "root"
DEFAULT_OPENBMC_PASSWORD = "0penBmc"
FLASH_SIZE = 128 * 1024 * 1024
HOST = "localhost"
HOST_SSH_FORWARD_PORT = 2222
QEMU_PARAMETER = "  -machine {} -nographic  -drive file={},format=raw,if=mtd \
    -drive file={},format=raw,if=mtd \
    -netdev user,id=mlx,mfr-id=0x8119,oob-eth-addr=aa:bb:cc:dd:ee:ff,hostfwd=:127.0.0.1:{}-:22,hostname=qemu \
    -net nic,model=ftgmac100,netdev=mlx"

class TestWrapper(object):
    def __init__(self):
        self.ssh = None
        self.sftp = None
        self.tests = []

    def start_qemu(self, platform: str) -> bool:
        """Use pexect to boot up image, which can also be used to verify console
           login

        Args:
            platform ([type]): [description]

        Returns:
            bool: [description]
        """
        cmd = "/tmp/job/project/qemu-system-aarch64"
        cmd += QEMU_PARAMETER.format(
            platform + "-bmc",
            self._golden_image_file,
            self._primary_image_file,
            HOST_SSH_FORWARD_PORT,
        )

        print(f"qemu cmd = {cmd}")
        self._qemu_instance = pexpect.spawn(
            cmd, timeout=600, logfile=sys.stdout.buffer, use_poll=True
        )
        if self._qemu_instance is not None:
            if self._qemu_instance.isalive():
                print("Finished starting QEMU run")
                self._qemu_instance.expect("login:")
                self._qemu_instance.sendline("root")
                self._qemu_instance.expect("Password:")
                self._qemu_instance.sendline(DEFAULT_OPENBMC_PASSWORD)
                self._qemu_instance.expect("#")
                return True
        return False

    def wait_for_qemu_oob_sshd_running(self) -> None:
        if self._qemu_instance is None or not self._qemu_instance.isalive():
            return
        for _ in range(10):
            self._qemu_instance.sendline("ps | grep ssh")
            try:
                self._qemu_instance.expect("sshd", timeout=30)
                break
            except TIMEOUT:
                continue
            except EOF:
                continue

    def init_ssh(self) -> None:
        self.ssh = paramiko.SSHClient()
        self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        count = 10
        while count != 0:
            try:
                self.ssh.connect(
                    HOST,
                    HOST_SSH_FORWARD_PORT,
                    DEFAULT_BMC_USER,
                    DEFAULT_OPENBMC_PASSWORD,
                    banner_timeout=200,
                )
                break
            except paramiko.ssh_exception.SSHException:
                print("unable to connect to bmc via SSH, wait 30s to retry..")
                time.sleep(30)
                count -= 1
        if count == 0:
            print("exhaust retry, giving up..")
            raise paramiko.ssh_exception.SSHException

    def run_cmd_on_oob(self, cmd: str) -> Tuple[int, ChannelFile, ChannelStderrFile]:
        stdin, stdout, stderr = self.ssh.exec_command(f"PATH={PATH} {cmd}")
        exit_status = stdout.channel.recv_exit_status()
        return exit_status, stdout, stderr

    def mark_qemu_for_cit(self):
        """
        create a dummy file: /usr/local/bin/tests2/dummy_qemu
        this file will use as indicator for CIT test run
        """
        self.run_cmd_on_oob("touch /usr/local/bin/tests2/dummy_qemu")

    def scp_oob_all(self, localpath: str, remotepath: str):
        if not self.sftp:
            self.sftp = paramiko.SFTPClient.from_transport(self.ssh.get_transport())
        for walker in os.walk(localpath):
            try:
                self.sftp.mkdir(os.path.join(remotepath, walker[0]))
            except Exception:
                pass
            for file in walker[2]:
                self.sftp.put(
                    os.path.join(walker[0], file),
                    os.path.join(remotepath, walker[0], file),
                )

    def qemu_prepare_bootable_image(self, platform: str) -> None:
        """
        Create padded image to 128MB for now
        """
        flashFile = (
            f"/tmp/job/project/build/tmp/deploy/images/{platform}/flash-{platform}"
        )
        zeroSize = FLASH_SIZE - os.path.getsize(flashFile)
        self._padded_image_file = flashFile + "-padded"
        try:
            with open(self._padded_image_file, "wb") as outf, open(
                flashFile, "rb"
            ) as inf:
                outf.write(inf.read())
                outf.write(b"\0" * zeroSize)
        except Exception as e:
            print(f"fail to pad the flash file: {str(e)}")
            sys.exit(1)

        self._golden_image_file = "/tmp/golden.mtd"
        self._primary_image_file = "/tmp/primary.mtd"
        copy(self._padded_image_file, self._golden_image_file)
        copy(self._padded_image_file, self._primary_image_file)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "platform", type=str, help="specify the running platform, e.g. fby3"
    )
    parser.add_argument(
        "-t",
        "--test",
        type=str,
        help="specify the test to run, e.g. tests.fby2.test_slot_util.SlotUtilTest.test_show_slot_by_name",
    )
    parser.add_argument(
        "--skip-qemu-setup",
        action="store_true",
        help="Skip QEMU setup if it's already running with port 22 forwarded to localhost:2222",
    )
    args = parser.parse_args()
    platform = args.platform
    print(f"start qemu cit test on platform: {platform}")
    testWrapper = TestWrapper()

    if not args.skip_qemu_setup:
        testWrapper.qemu_prepare_bootable_image(platform=platform)
        print("bootable image ready..")
        testWrapper.start_qemu(platform=platform)
        testWrapper.wait_for_qemu_oob_sshd_running()

    print("qemu started, checking SSH connection..")
    testWrapper.init_ssh()
    print("SSH connection ok, continue..")

    testWrapper.scp_oob_all("tests2", "/usr/local/bin/")
    print("scp tests to BMC complete..")
    testWrapper.run_cmd_on_oob("touch /usr/local/bin/tests2/dummy_qemu")
    print("qemu flag set..")
    if not args.test:
        status, out, err = testWrapper.run_cmd_on_oob(
            "python3 /usr/local/bin/tests2/cit_runner.py"
            f" --platform {platform} --list-tests"
            f" --denylist /usr/local/bin/tests2/qemu_denylist.txt"
        )
        if status != 0:
            sys.exit(1)
        for line in out.readlines():
            testWrapper.tests.append(line.strip())
    else:
        testWrapper.tests.append(args.test)
    print("start CIT run in QEMU")
    test_pass = True
    pass_count = 0
    fail_count = 0
    for test in testWrapper.tests:
        status, out, err = testWrapper.run_cmd_on_oob(
            f"python3 /usr/local/bin/tests2/cit_runner.py --run-test {test}"
            f" --denylist /usr/local/bin/tests2/qemu_denylist.txt"
        )
        if status != 0:
            fail_count += 1
            test_pass = False
            print(f"{test}: fail")
            for line in err.readlines():
                print(line.strip())
        else:
            pass_count += 1
            print(f"{test}: pass")
    print(
        f"CIT test complete, PASS: {pass_count}, "
        + f"FAIL: {fail_count}, TOTAL: {len(testWrapper.tests)}"
    )
    sys.exit(1) if not test_pass else sys.exit(0)
