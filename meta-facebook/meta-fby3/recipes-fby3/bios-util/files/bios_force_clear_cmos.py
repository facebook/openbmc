import subprocess


def force_clear_cmos(fru):
    name = {
        1: "slot1",
        2: "slot2",
        3: "slot3",
        4: "slot4",
    }
    if fru not in name:
        raise ValueError(f"Unsupported FRU: {fru}")
    cmd = ["/usr/bin/bic-util", name[fru], "--clear_cmos"]
    subprocess.check_call(cmd)
