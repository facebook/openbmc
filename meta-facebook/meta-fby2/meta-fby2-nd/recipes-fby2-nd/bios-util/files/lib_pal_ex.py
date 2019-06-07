#!/usr/bin/env python
import subprocess


def pal_print_postcode(fru):
    bic_cmd = ["bic-util", "slot%d" % fru, "--get_post_code"]
    try:
        # remove the first line of bic output
        # as bic-util postcode output will not be much (4 * 1024) around 4K
        # we use simple check_output
        bic_output = subprocess.check_output(bic_cmd).decode("utf-8").splitlines()[1:]
        for line in bic_output:
            print(line)
    except subprocess.CalledProcessError as e:
        print("execute [%s] failed (%d)" % (" ".join(bic_cmd), e.returncode))
    except Exception as e:
        print("execute [%s] failed" % " ".join(bic_cmd))
        print(e)
