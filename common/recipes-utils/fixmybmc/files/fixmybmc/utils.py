def indent(text: str, level: int = 1) -> str:
    indention = "\t" * level
    return "\n".join(f"{indention}{line}" for line in text.splitlines())
