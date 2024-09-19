# pyre
from subprocess import CompletedProcess
from typing import List


class Status:
    def __str__(self) -> str:
        if self.info is not None:
            return self.info
        else:
            return self.name

    @property
    def has_manual_remediation(self) -> bool:
        return False

    @property
    def info(self) -> None:
        """Information that should be displayed prominently to the user"""
        return None

    @property
    def name(self) -> str:
        return self.__class__.__name__.lower()


class Error(Status):
    def __init__(
        self,
        exception=None,
        description=None,
        cmd_status=None,
    ) -> None:
        self.exception = exception
        self.description = description
        self.cmd_status = cmd_status

    @property
    def info(self):
        parts = []
        if self.exception is not None:
            parts.append(self.exception)
        if self.description is not None:
            parts.append(self.description)
        if self.cmd_status:
            parts += get_cmd_status_text(self.cmd_status)
        return "\n".join(parts) or None


class Ok(Status):
    pass


class Problem(Status):
    def __init__(
        self,
        *,
        description=None,
        exception=None,
        cmd_status=None,
        manual_remediation=None,
    ) -> None:
        if description is None and exception is None:
            raise TypeError("either description or exception must be provided")
        self.description = description
        self.exception = exception
        self.cmd_status = cmd_status
        self.manual_remediation = manual_remediation

    @property
    def has_manual_remediation(self) -> bool:
        return self.manual_remediation is not None

    @property
    def info(self):
        parts = []
        if self.description is not None:
            parts.append(self.description)
        if self.exception is not None:
            parts.append(f"{self.exception.__class__.__qualname__}: {self.exception}")
        if self.cmd_status:
            parts += get_cmd_status_text(self.cmd_status)
        if self.has_manual_remediation:
            parts.append(f"Remediation:\n{self.manual_remediation}")
        return "\n".join(parts) or None


def get_cmd_status_text(cmd_status: CompletedProcess) -> List[str]:
    parts = []
    if cmd_status.args:
        parts.append(f"Command: {' '.join(cmd_status.args)}")
    if cmd_status.stdout:
        parts.append(f"stdout:\n{cmd_status.stdout}")
    if cmd_status.stderr:
        parts.append(f"stderr:\n {cmd_status.stderr}")
    return parts
