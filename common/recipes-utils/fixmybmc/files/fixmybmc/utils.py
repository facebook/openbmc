import subprocess
from enum import Enum


def indent(text: str, level: int = 1) -> str:
    indention = "  " * level
    return "\n".join(f"{indention}{line}" for line in text.splitlines())


def run_cmd(cmd, **kwargs) -> subprocess.CompletedProcess:
    result = subprocess.run(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, **kwargs
    )
    return result


def check_cmd(cmd, **kwargs) -> bool:
    return run_cmd(cmd, **kwargs).returncode == 0


def overwrite_print(text: str) -> None:
    print("\033[K" + text, end="\r")


def clear_line() -> None:
    print("\033[K", end="\r")


class Color(Enum):
    RED = "\033[91m"
    GREEN = "\033[92m"
    ORANGE = "\033[38;5;208m"


color_to_bg = {
    Color.RED: "\033[41m",
    Color.GREEN: "\033[42m",
    Color.ORANGE: "\033[48;5;208m",
}


def color(c: Color):
    return lambda text: f"{c.value}{text}"


def color_bg(c: Color):
    return lambda text: f"{color_to_bg[c]}{text}"


def bold():
    return lambda text: f"\033[1m{text}"


def styled(text, *styles):
    reset_style = "\033[0m"  # Reset all stying
    for style in styles:
        text = style(text)
    return f"{text}{reset_style}"
