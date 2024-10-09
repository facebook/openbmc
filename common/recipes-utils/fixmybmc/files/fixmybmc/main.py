#!/usr/bin/env python3
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import sys
from typing import Dict, List

from fixmybmc import status

from fixmybmc.bmccheck import _checks, BmcCheck, load_checks
from fixmybmc.utils import (
    bold,
    clear_line,
    color,
    color_bg,
    indent,
    overwrite_print,
    styled,
)


def main() -> int:
    load_checks()
    overwrite_print(f"Loaded {len(_checks)} checks")

    overwrite_print("Running checks...")
    statuses = run_checks(_checks)

    show_result_summary(_checks.values())
    return all(isinstance(s, status.Ok) for s in statuses)


def run_checks(checks: Dict[str, BmcCheck]) -> List[status.Status]:
    results: List[status.Status] = []
    for name, check in checks.items():
        overwrite_print(f"Checking: {name}")
        check.run()
        results.append(check.result)
    clear_line()
    return results


def show_result_summary(checks: List[BmcCheck]) -> None:
    print(f"Ran {len(checks)} checks")
    print("-----")
    passed = [check for check in checks if isinstance(check.result, status.Ok)]
    problems = [check for check in checks if isinstance(check.result, status.Problem)]
    errors = [check for check in checks if isinstance(check.result, status.Error)]
    print("Result summary:")
    if len(passed) > 0:
        print(indent(styled(f"Passed: {len(passed)}", color(status.Ok.color), bold())))
        print(indent(", ".join([check.name for check in passed]), level=2))
    if len(problems) > 0:
        print(
            indent(
                styled(
                    f"Problems: {len(problems)}", color(status.Problem.color), bold()
                )
            )
        )
    if len(errors) > 0:
        print(
            indent(styled(f"Errors: {len(errors)}", color(status.Error.color), bold()))
        )
    print("-----")

    for check in problems + errors:
        print_res_name = styled(
            f"[  {check.result.name}  ]", color_bg(check.result.color), bold()
        )
        print(f"{print_res_name} {check.name}")
        print(f"{indent(str(check.result))}")


if __name__ == "__main__":
    sys.exit(main())
