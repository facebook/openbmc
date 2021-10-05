import subprocess


def force_clear_cmos(fru):
    if fru != 1:
        raise ValueError(f"Unsupported FRU: {fru}")

    cmd = ["/usr/bin/bic-util", "server", "--clear_cmos"]
    subprocess.check_call(cmd)
