#!/usr/bin/env python3
#
# Copyright 2018-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#
import functools
import re
import subprocess
import sys
from unittest import TestCase


class BusyBoxSyntaxTest(TestCase):
    # Parameterised test, will run for every entry in KNOWN_USAGE_STRINGS
    def _test_usage_string_template(self, cmd, expected_strings):
        """
        For each busybox command, check if its --help output is known
        """
        found_usage_strings = self.get_cmd_usage_strings(cmd)

        self.assertTrue(
            bool(found_usage_strings),
            "could not find usage strings from `{cmd} --help`".format(cmd=cmd),
        )
        self.assertTrue(
            any(usage in expected_strings for usage in found_usage_strings),
            "Unknown usage from command `{cmd} --help`: {found_usage_strings}".format(
                cmd=cmd, found_usage_strings=found_usage_strings
            ),
        )

    ## Utils
    def get_cmd_usage_strings(self, cmd):
        "Runs `{cmd} --help` and extracts usage strings"
        output = subprocess.check_output(
            "{cmd} --help || true".format(cmd=cmd),
            stderr=subprocess.STDOUT,
            shell=True,
            timeout=10,
        )

        return [
            m.group("syntax").strip()
            for m in re.finditer(
                r"^\s*(Usage:|{cmd}:)\s*(?P<syntax>{cmd}.*)".format(cmd=cmd),
                output.decode("utf-8"),
                flags=re.MULTILINE,
            )
        ]

    @classmethod
    def _gen_parameterised_tests(cls, known_usage_strings):
        # _test_usage_string_template -> test_usage_string_{cmd}
        for cmd, expected_strings in known_usage_strings.items():
            setattr(
                cls,
                "test_usage_string_{cmd}".format(cmd=cmd),
                functools.partialmethod(
                    cls._test_usage_string_template,
                    cmd=cmd,
                    expected_strings=expected_strings,
                ),
            )


# Initial set generated using:
# $ for cmd in $(ls -l /bin /usr/bin | grep -Eo '[^ ]+ -> /bin/busybox' | grep -Eo '^[^ ]+'); do echo "'$cmd': [\""$($cmd --help 2>&1 | grep -E "^Usage: $cmd |^$cmd"] | sed "s/^Usage: //")"\"],"; done | sort  # noqa: B950

KNOWN_USAGE_STRINGS = {
    "ash": ["ash [-/+OPTIONS] [-/+o OPT]... [-c 'SCRIPT' [ARG0 [ARGS]] / FILE [ARGS]]"],
    "awk": ["awk [OPTIONS] [AWK_PROGRAM] [FILE]..."],
    "base64": ["base64 [-d] [FILE]"],
    "basename": ["basename FILE [SUFFIX]"],
    "bunzip2": ["bunzip2 [-cf] [FILE]..."],
    "bzcat": ["bzcat [FILE]..."],
    "cat": ["cat [FILE]..."],
    "chattr": ["chattr [-R] [-+=AacDdijsStTu] [-v VERSION] [FILE]..."],
    "chgrp": ["chgrp [-RhLHP]... GROUP FILE..."],
    "chmod": ["chmod [-R] MODE[,MODE]... FILE..."],
    "chown": ["chown [-Rh]... OWNER[<.|:>[GROUP]] FILE..."],
    "chpst": ["chpst [-vP012] [-u USER[:GRP]] [-U USER[:GRP]] [-e DIR]"],
    "chvt": ["chvt N"],
    "cksum": ["cksum FILES..."],
    "clear": ["clear"],
    "cmp": ["cmp [-l] [-s] FILE1 [FILE2]"],
    "cp": ["cp [OPTIONS] SOURCE... DEST"],
    "cpio": ["cpio [-dmvu] [-F FILE] [-ti] [EXTR_FILE]..."],
    "crontab": ["crontab [-c DIR] [-u USER] [-ler]|[FILE]"],
    "cut": ["cut [OPTIONS] [FILE]..."],
    "date": ["date [OPTIONS] [+FMT] [TIME]"],
    "dc": ["dc EXPRESSION..."],
    "dd": ["dd [if=FILE] [of=FILE] [bs=N] [count=N] [skip=N]"],
    "deallocvt": ["deallocvt [N]"],
    "df": ["df [-PkmhT] [FILESYSTEM]..."],
    "diff": ["diff [-abBdiNqrTstw] [-L LABEL] [-S FILE] [-U LINES] FILE1 FILE2"],
    "dirname": ["dirname FILENAME"],
    "dmesg": ["dmesg [-c] [-n LEVEL] [-s SIZE]"],
    "du": ["du [-aHLdclsxhmk] [FILE]..."],
    "dumpkmap": ["dumpkmap > keymap"],
    "env": ["env [-iu] [-] [name=value]... [PROG ARGS]"],
    "envdir": ["envdir DIR PROG ARGS"],
    "envuidgid": ["envuidgid USER PROG ARGS"],
    "expr": ["expr EXPRESSION"],
    "find": ["find [-HL] [PATH]... [OPTIONS] [ACTIONS]"],
    "flock": ["flock [-sxun] FD|{FILE [-c] PROG ARGS}"],
    "free": ["free"],
    "fuser": ["fuser [OPTIONS] FILE or PORT/PROTO"],
    "getopt": ["getopt [OPTIONS] [--] OPTSTRING PARAMS"],
    "grep": [
        "grep [-HhnlLoqvsriwFE] [-m N] [-A/B/C N] PATTERN/-e PATTERN.../-f FILE [FILE]..."  # noqa: B950
    ],
    "gunzip": ["gunzip [-cft] [FILE]..."],
    "gzip": ["gzip [-cfd] [FILE]..."],
    "head": ["head [OPTIONS] [FILE]..."],
    "hexdump": ["hexdump [-bcCdefnosvx] [FILE]..."],
    "hostname": ["hostname [OPTIONS] [HOSTNAME | -F FILE]"],
    "id": ["id [OPTIONS] [USER]"],
    "kill": [
        "kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]"  # noqa: B950
    ],
    "killall": ["killall [-l] [-q] [-SIG] PROCESS_NAME..."],
    "less": ["less [-EMmNh~] [FILE]..."],
    "ln": ["ln [OPTIONS] TARGET... LINK|DIR"],
    "logger": ["logger [OPTIONS] [MESSAGE]"],
    "logname": ["logname"],
    "ls": ["ls [-1AaCxdLHRFplinsehrSXvctu] [-w WIDTH] [FILE]..."],
    "md5sum": ["md5sum [-c[sw]] [FILE]..."],
    "microcom": ["microcom [-d DELAY] [-t TIMEOUT] [-s SPEED] [-X] TTY"],
    "mkdir": ["mkdir [OPTIONS] DIRECTORY..."],
    "mkfifo": ["mkfifo [-m MODE] NAME"],
    "mknod": ["mknod [-m MODE] NAME TYPE MAJOR MINOR"],
    "mktemp": ["mktemp [-dt] [-p DIR] [TEMPLATE]"],
    "more": ["more [FILE]..."],
    "mount": ["mount [OPTIONS] [-o OPT] DEVICE NODE"],
    "mv": ["mv [-fin] SOURCE DEST"],
    "nc": ["nc [-l] [-p PORT] [IPADDR PORT]"],
    "netstat": ["netstat [-ral] [-tuwx] [-en]"],
    "nohup": ["nohup PROG ARGS"],
    "nslookup": ["nslookup [HOST] [SERVER]"],
    "od": ["od [-aBbcDdeFfHhIiLlOovXx] [FILE]"],
    "openvt": ["openvt [-c N] [-sw] [PROG ARGS]"],
    "patch": ["patch [OPTIONS] [ORIGFILE [PATCHFILE]]"],
    "ping": ["ping [OPTIONS] HOST"],
    "ping6": ["ping6 [OPTIONS] HOST"],
    "pkill": ["pkill [-l|-SIGNAL] [-fnovx] [-s SID|-P PPID|PATTERN]"],
    "printf": ["printf [-v var] format [arguments]"],
    "ps": ["ps"],
    "pwd": ["pwd [-LP]"],
    "readlink": ["readlink [-fnv] FILE"],
    "realpath": ["realpath FILE..."],
    "renice": ["renice {{-n INCREMENT} | PRIORITY} [[-p | -g | -u] ID...]"],
    "reset": ["reset"],
    "resize": ["resize"],
    "rm": ["rm [-irf] FILE..."],
    "rmdir": ["rmdir [OPTIONS] DIRECTORY..."],
    "run-parts": [
        "run-parts [-a ARG]... [-u UMASK] [--reverse] [--test] [--exit-on-error] DIRECTORY"  # noqa: B950
    ],
    "runsv": ["runsv DIR"],
    "runsvdir": ["runsvdir [-P] [-s SCRIPT] DIR"],
    "rx": ["rx FILE"],
    "sed": ["sed [-inrE] [-f FILE]... [-e CMD]... [FILE]..."],
    "seq": ["seq [-w] [-s SEP] [FIRST [INC]] LAST"],
    "setuidgid": ["setuidgid USER PROG ARGS"],
    "sha1sum": ["sha1sum [-c[sw]] [FILE]..."],
    "sha256sum": ["sha256sum [-c[sw]] [FILE]..."],
    "shuf": ["shuf [-e|-i L-H] [-n NUM] [-o FILE] [-z] [FILE|ARG...]"],
    "sleep": ["sleep [N]..."],
    "softlimit": ["softlimit [-a BYTES] [-m BYTES] [-d BYTES] [-s BYTES] [-l BYTES]"],
    "sort": [
        "sort [-nrugMcszbdfimSTokt] [-o FILE] [-k start[.offset][opts][,end[.offset][opts]] [-t CHAR] [FILE]..."  # noqa: B950
    ],
    "stat": ["stat [OPTIONS] FILE..."],
    "strings": ["strings [-afo] [-n LEN] [FILE]..."],
    "stty": ["stty [-a|g] [-F DEVICE] [SETTING]..."],
    "sv": ["sv [-v] [-w SEC] CMD SERVICE_DIR..."],
    "sync": ["sync"],
    "tac": ["tac [FILE]..."],
    "tail": ["tail [OPTIONS] [FILE]..."],
    "tar": ["tar -[cxtZzJjahmvO] [-X FILE] [-T FILE] [-f TARFILE] [-C DIR] [FILE]..."],
    "tee": ["tee [-ai] [FILE]..."],
    "telnet": ["telnet [-a] [-l USER] HOST [PORT]"],
    "tftp": ["tftp [OPTIONS] HOST [PORT]"],
    "time": ["time [-v] PROG ARGS"],
    "timeout": ["timeout [-t SECS] [-s SIG] PROG ARGS"],
    "top": ["top [-b] [-nCOUNT] [-dSECONDS]"],
    "touch": ["touch [-c] [-d DATE] [-t DATE] [-r FILE] FILE..."],
    "tr": ["tr [-cds] STRING1 [STRING2]"],
    "traceroute": [
        "traceroute [-46FIlnrv] [-f 1ST_TTL] [-m MAXTTL] [-q PROBES] [-p PORT]"
    ],
    "traceroute6": ["traceroute6 [-nrv] [-m MAXTTL] [-q PROBES] [-p PORT]"],
    "tty": ["tty"],
    "umount": ["umount [OPTIONS] FILESYSTEM|DIRECTORY"],
    "uname": ["uname [-amnrspvio]"],
    "uniq": ["uniq [-cdu][-f,s,w N] [INPUT [OUTPUT]]"],
    "unlink": ["unlink FILE"],
    "unzip": ["unzip [-lnopq] FILE[.zip] [FILE]... [-x FILE...] [-d DIR]"],
    "uptime": ["uptime"],
    "users": ["users"],
    "usleep": ["usleep N"],
    "vi": ["vi [OPTIONS] [FILE]..."],
    "vlock": ["vlock [-a]"],
    "watch": ["watch [-n SEC] [-t] PROG ARGS"],
    "wc": ["wc [-clwL] [FILE]..."],
    "wget": [
        "wget [-c|--continue] [-s|--spider] [-q|--quiet] [-O|--output-document FILE]"  # noqa: B950
    ],
    "which": ["which [COMMAND]..."],
    "who": ["who [-a]"],
    "whoami": ["whoami"],
    "xargs": ["xargs [OPTIONS] [PROG ARGS]"],
    "yes": ["yes [STRING]"],
    "zcat": ["zcat [FILE]..."],
}

if sys.version_info.major == 2:
    # XXX cit uses python2 to retrieve the test list, but the
    # actual tests in the BMC run in python3.
    # https://fburl.com/diffusion/ka08q9wg
    # This is a temporary workaround until it gets migrated to
    # python3

    # Generating dummy methods just so they get listed
    for cmd in KNOWN_USAGE_STRINGS:
        setattr(
            BusyBoxSyntaxTest,
            "test_usage_string_{cmd}".format(cmd=cmd),
            lambda self: None,
        )

else:
    # We're using python3
    BusyBoxSyntaxTest._gen_parameterised_tests(known_usage_strings=KNOWN_USAGE_STRINGS)
