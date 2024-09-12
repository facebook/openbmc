# pyre


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
    def __init__(self, exception) -> None:
        self.exception = exception


class Ok(Status):
    pass


class Problem(Status):
    def __init__(
        self,
        *,
        description=None,
        exception=None,
        manual_remediation=None,
    ) -> None:
        if description is None and exception is None:
            raise TypeError("either description or exception must be provided")
        self.description = description
        self.exception = exception
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
        if self.has_manual_remediation:
            parts.append(f"Remediation: {self.manual_remediation}")
        return "\n".join(parts) or None
