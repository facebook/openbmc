import subprocess


def indent(text: str, level: int = 1) -> str:
    indention = "\t" * level
    return "\n".join(f"{indention}{line}" for line in text.splitlines())


def run_cmd(cmd, **kwargs) -> subprocess.CompletedProcess:
    result = subprocess.run(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs
    )
    return result


def check_cmd(cmd, **kwargs) -> bool:
    return run_cmd(cmd, **kwargs).returncode == 0
