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
                r"^\s*(Usage:|{cmd}:)?\s*(?P<syntax>{cmd}.*)".format(cmd=cmd),
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
    "ash": [
        "ash [-/+OPTIONS] [-/+o OPT]... [-c 'SCRIPT' [ARG0 [ARGS]] / FILE [ARGS]]",
        "ash [-/+OPTIONS] [-/+o OPT]... [-c 'SCRIPT' [ARG0 [ARGS]] / FILE [ARGS] / -s [ARGS]]",  # BusyBox 1.30.1  # noqa: B950
    ],
    "awk": ["awk [OPTIONS] [AWK_PROGRAM] [FILE]..."],
    "base64": ["base64 [-d] [FILE]"],
    "basename": ["basename FILE [SUFFIX]"],
    "bunzip2": [
        "bunzip2 [-cf] [FILE]...",
        "bunzip2 [-cfk] [FILE]...",  # BusyBox 1.30.1
    ],
    "bzcat": ["bzcat [FILE]..."],
    "cat": ["cat [FILE]...", "cat [-nb] [FILE]..."],  # BusyBox 1.30.1
    "chattr": [
        "chattr [-R] [-+=AacDdijsStTu] [-v VERSION] [FILE]...",
        "chattr [-pRVf] [-+=aAcCdDeijPsStTu] [-v version] files...",  # BusyBox 1.30.1
    ],
    "chgrp": ["chgrp [-RhLHP]... GROUP FILE..."],
    "chmod": ["chmod [-R] MODE[,MODE]... FILE..."],
    "chown": [
        "chown [-Rh]... OWNER[<.|:>[GROUP]] FILE...",
        "chown [-Rh]... USER[:[GRP]] FILE...",  # BusyBox 1.30.1
    ],
    "chpst": ["chpst [-vP012] [-u USER[:GRP]] [-U USER[:GRP]] [-e DIR]"],
    "chvt": ["chvt N"],
    # fmt: off
    "cksum": [
        "cksum FILES...",
        "cksum FILE...",  # BusyBox 1.30.1
    ],
    # fmt: on
    "clear": ["clear"],
    "cmp": ["cmp [-l] [-s] FILE1 [FILE2]"],
    "cp": ["cp [OPTIONS] SOURCE... DEST"],
    "cpio": [
        "cpio [-dmvu] [-F FILE] [-ti] [EXTR_FILE]...",
        "cpio [-dmvu] [-F FILE] [-R USER[:GRP]] [-ti] [EXTR_FILE]...",  # BusyBox 1.30.1
    ],
    "crontab": ["crontab [-c DIR] [-u USER] [-ler]|[FILE]"],
    "cut": ["cut [OPTIONS] [FILE]..."],
    "date": ["date [OPTIONS] [+FMT] [TIME]"],
    "dc": [
        "dc EXPRESSION...",  # noqa: B950
        "dc [-eSCRIPT]... [-fFILE]... [FILE]...",  # BusyBox 1.30.1
    ],
    "dd": [
        "dd [if=FILE] [of=FILE] [bs=N] [count=N] [skip=N]",  # noqa: B950
        "dd [if=FILE] [of=FILE] [bs=N] [count=N] [skip=N] [seek=N]",  # BusyBox 1.30.1
    ],
    "deallocvt": ["deallocvt [N]"],
    "df": ["df [-PkmhT] [FILESYSTEM]..."],
    "diff": ["diff [-abBdiNqrTstw] [-L LABEL] [-S FILE] [-U LINES] FILE1 FILE2"],
    "dirname": ["dirname FILENAME"],
    "dmesg": [
        "dmesg [-c] [-n LEVEL] [-s SIZE]",  # noqa: B950
        "dmesg [options]",  # BusyBox 1.30.1
    ],
    "du": ["du [-aHLdclsxhmk] [FILE]..."],
    "dumpkmap": ["dumpkmap > keymap"],
    "env": ["env [-iu] [-] [name=value]... [PROG ARGS]"],
    "envdir": ["envdir DIR PROG ARGS"],
    "envuidgid": ["envuidgid USER PROG ARGS"],
    "expr": ["expr EXPRESSION"],
    "find": ["find [-HL] [PATH]... [OPTIONS] [ACTIONS]"],
    "flock": [
        "flock [-sxun] FD|{FILE [-c] PROG ARGS}",
        "flock [options] <file>|<directory> <command> [<argument>...]",  # BusyBox 1.30.1  # noqa: B950
    ],
    "free": ["free"],
    "fuser": ["fuser [OPTIONS] FILE or PORT/PROTO"],
    "getopt": [
        "getopt [OPTIONS] [--] OPTSTRING PARAMS",
        "getopt <optstring> <parameters>",  # BusyBox 1.30.1
    ],
    "grep": [
        "grep [-HhnlLoqvsriwFE] [-m N] [-A/B/C N] PATTERN/-e PATTERN.../-f FILE [FILE]..."  # noqa: B950
    ],
    "gunzip": [
        "gunzip [-cft] [FILE]...",  # noqa: B950
        "gunzip [-cfkt] [FILE]...",  # BusyBox 1.30.1
    ],
    "gzip": [
        "gzip [-cfd] [FILE]...",  # noqa: B950
        "gzip [-cfkdt] [FILE]...",  # BusyBox 1.30.1
    ],
    "head": ["head [OPTIONS] [FILE]..."],
    "hexdump": [
        "hexdump [-bcCdefnosvx] [FILE]...",  # noqa: B950
        "hexdump [options] <file>...",  # BusyBox 1.30.1
    ],
    "hostname": ["hostname [OPTIONS] [HOSTNAME | -F FILE]"],
    "id": ["id [OPTIONS] [USER]"],
    "kill": [
        "kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]"  # noqa: B950
    ],
    "killall": ["killall [-l] [-q] [-SIG] PROCESS_NAME..."],
    "less": [
        "less [-EMmNh~] [FILE]...",  # noqa: B950
        "less [-EFMmNh~] [FILE]...",  # BusyBox 1.30.1
    ],
    "ln": ["ln [OPTIONS] TARGET... LINK|DIR"],
    "logger": [
        "logger [OPTIONS] [MESSAGE]",  # noqa: B950
        "logger [options] [<message>]",  # BusyBox 1.30.1
    ],
    "logname": ["logname"],
    "ls": [
        "ls [-1AaCxdLHRFplinsehrSXvctu] [-w WIDTH] [FILE]...",  # noqa: B950
        "ls [-1AaCxdLHRFplinshrSXvctu] [-w WIDTH] [FILE]...",  # BusyBox 1.30.1
    ],
    "md5sum": ["md5sum [-c[sw]] [FILE]..."],
    "microcom": ["microcom [-d DELAY] [-t TIMEOUT] [-s SPEED] [-X] TTY"],
    "mkdir": ["mkdir [OPTIONS] DIRECTORY..."],
    "mkfifo": ["mkfifo [-m MODE] NAME"],
    "mknod": [
        "mknod [-m MODE] NAME TYPE MAJOR MINOR",
        "mknod [-m MODE] NAME TYPE [MAJOR MINOR]",  # BusyBox 1.30.1
    ],
    "mktemp": ["mktemp [-dt] [-p DIR] [TEMPLATE]"],
    "more": [
        "more [FILE]...",  # noqa: B950
        "more [options] <file>...",  # BusyBox 1.30.1
    ],
    "mount": [
        "mount [OPTIONS] [-o OPT] DEVICE NODE",
        "mount <operation> <mountpoint> [<target>]",  # BusyBox 1.30.1
    ],
    "mv": ["mv [-fin] SOURCE DEST"],
    "nc": ["nc [-l] [-p PORT] [IPADDR PORT]"],
    "netstat": ["netstat [-ral] [-tuwx] [-en]"],
    "nohup": ["nohup PROG ARGS"],
    "nslookup": [
        "nslookup [HOST] [SERVER]",
        "nslookup HOST [DNS_SERVER]",  # BusyBox 1.30.1
    ],
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
    "renice": [
        "renice {{-n INCREMENT} | PRIORITY} [[-p | -g | -u] ID...]",  # noqa: B950
        "renice [-n] <priority> [-p|--pid] <pid>...",  # BusyBox 1.30.1
        "renice [-n] <priority>  -g|--pgrp <pgid>...",  # BusyBox 1.30.1
        "renice [-n] <priority>  -u|--user <user>...",  # BusyBox 1.30.1
    ],
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
    "sed": [
        "sed [-inrE] [-f FILE]... [-e CMD]... [FILE]...",  # noqa: B950
        "sed [-i[SFX]] [-nrE] [-f FILE]... [-e CMD]... [FILE]...",  # BusyBox 1.30.1
    ],
    "seq": ["seq [-w] [-s SEP] [FIRST [INC]] LAST"],
    "setuidgid": ["setuidgid USER PROG ARGS"],
    "sha1sum": ["sha1sum [-c[sw]] [FILE]..."],
    "sha256sum": ["sha256sum [-c[sw]] [FILE]..."],
    "shuf": ["shuf [-e|-i L-H] [-n NUM] [-o FILE] [-z] [FILE|ARG...]"],
    "sleep": ["sleep [N]..."],
    "softlimit": ["softlimit [-a BYTES] [-m BYTES] [-d BYTES] [-s BYTES] [-l BYTES]"],
    "sort": [
        "sort [-nrugMcszbdfimSTokt] [-o FILE] [-k start[.offset][opts][,end[.offset][opts]] [-t CHAR] [FILE]...",  # noqa: B950
        "sort [-nrugMcszbdfiokt] [-o FILE] [-k start[.offset][opts][,end[.offset][opts]] [-t CHAR] [FILE]...",  # BusyBox 1.30.1  # noqa: B950
    ],
    "stat": ["stat [OPTIONS] FILE..."],
    "strings": [
        "strings [-afo] [-n LEN] [FILE]...",
        "strings [-fo] [-t o/d/x] [-n LEN] [FILE]...",  # BusyBox 1.30.1
    ],
    "stty": ["stty [-a|g] [-F DEVICE] [SETTING]..."],
    "sv": ["sv [-v] [-w SEC] CMD SERVICE_DIR..."],
    "sync": ["sync"],
    "tac": ["tac [FILE]..."],
    "tail": ["tail [OPTIONS] [FILE]..."],
    "tar": [
        "tar -[cxtZzJjahmvO] [-X FILE] [-T FILE] [-f TARFILE] [-C DIR] [FILE]...",  # noqa: B950
        "tar c|x|t [-ZzJjahmvokO] [-f TARFILE] [-C DIR] [-T FILE] [-X FILE] [FILE]...",  # BusyBox 1.30.1  # noqa: B950
    ],
    "tee": ["tee [-ai] [FILE]..."],
    "telnet": ["telnet [-a] [-l USER] HOST [PORT]"],
    "tftp": ["tftp [OPTIONS] HOST [PORT]"],
    "time": [
        "time [-v] PROG ARGS",
        "time [-vpa] [-o FILE] PROG ARGS",  # BusyBox 1.30.1
    ],
    "timeout": [
        "timeout [-t SECS] [-s SIG] PROG ARGS",  # noqa: B950
        "timeout [-s SIG] SECS PROG ARGS",  # BusyBox 1.30.1
    ],
    "top": ["top [-b] [-nCOUNT] [-dSECONDS]"],
    "touch": ["touch [-c] [-d DATE] [-t DATE] [-r FILE] FILE..."],
    "tr": ["tr [-cds] STRING1 [STRING2]"],
    "traceroute": [
        "traceroute [-46FIlnrv] [-f 1ST_TTL] [-m MAXTTL] [-q PROBES] [-p PORT]"
    ],
    "traceroute6": ["traceroute6 [-nrv] [-m MAXTTL] [-q PROBES] [-p PORT]"],
    "tty": ["tty"],
    "umount": [
        "umount [OPTIONS] FILESYSTEM|DIRECTORY",  # noqa: B950
        "umount [options] <source> | <directory>",  # BusyBox 1.30.1
    ],
    "uname": ["uname [-amnrspvio]"],
    "uniq": ["uniq [-cdu][-f,s,w N] [INPUT [OUTPUT]]"],
    "unlink": ["unlink FILE"],
    "unzip": [
        "unzip [-lnopq] FILE[.zip] [FILE]... [-x FILE...] [-d DIR]",  # noqa: B950
        "unzip [-lnojpq] FILE[.zip] [FILE]... [-x FILE...] [-d DIR]",  # BusyBox 1.30.1
    ],
    "uptime": ["uptime"],
    "users": ["users"],
    "usleep": ["usleep N"],
    "vi": ["vi [OPTIONS] [FILE]..."],
    "vlock": ["vlock [-a]"],
    "watch": ["watch [-n SEC] [-t] PROG ARGS"],
    "wc": ["wc [-clwL] [FILE]..."],
    "wget": [
        "wget [-c|--continue] [-s|--spider] [-q|--quiet] [-O|--output-document FILE]",  # noqa: B950
        "wget [-c|--continue] [--spider] [-q|--quiet] [-O|--output-document FILE]",  # BusyBox 1.30.1  # noqa: B950
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
