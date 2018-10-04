from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import subprocess
import logging


def psumuxmon_test():
    status = "failed"
    # Check process state
    proc = subprocess.Popen(
        "/usr/bin/sv status psumuxmon",
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    info, err = proc.communicate()
    if len(err) != 0:
        raise Exception(err)
    info = info.decode('utf-8')
    if "run" in info:
        print("psumuxmon: Process status is good [PASSED]")
        status = "pass"
    else:
        print("psumuxmon: Process status is not good with info={} [FAILED]".format(info))
        status = "failed"

    # Check ltc data from sensors
    proc = subprocess.Popen(
        'sensors ltc4151-i2c-7-6f',
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    info, err = proc.communicate()
    info = info.decode('utf-8')
    if len(err) != 0:
        raise Exception(err)
    if "NA" not in info:
        print("psumuxmon: LTC driver reading is good [PASSED]")
        status = "pass"
    else:
        print("psumuxmon: LTC driver reading is not good with info={} [FAILED]".format(info))
        status = "fail"

    return status

if __name__ == "__main__":
    try:
        test = psumuxmon_test()
        if test is "pass":
            print("psumuxmon test [PASSED]")
            exit(0)
        else:
            print("psumuxmon test [FAILED]")
            exit(1)
    except Exception as e:
        print("psumuxmon test failed with exception e={} [FAILED]".format(e))
        exit(1)
