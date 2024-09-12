#!/usr/bin/env python3
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

import sys
from typing import Dict, List

from fixmybmc import status

from fixmybmc.bmccheck import _checks, BmcCheck, load_checks
from fixmybmc.utils import indent


def main() -> int:
    load_checks()
    print(f"Loaded {len(_checks)} checks")

    print("Running checks...")
    statuses = run_checks(_checks)

    show_result_summary(_checks.values())
    return all(isinstance(s, status.Ok) for s in statuses)


def run_checks(checks: Dict[str, BmcCheck]) -> List[status.Status]:
    results: List[status.Status] = []
    for name, check in checks.items():
        print(f"Running check: {name}")
        check.run()
        results.append(check.result)
    return results


def show_result_summary(checks: List[BmcCheck]) -> None:
    num_passed = len(
        [check.result for check in checks if isinstance(check.result, status.Ok)]
    )
    num_problems = len(
        [check.result for check in checks if isinstance(check.result, status.Problem)]
    )
    num_errors = len(
        [check.result for check in checks if isinstance(check.result, status.Error)]
    )
    print("Result summary:")
    print(f"\tPassed: {num_passed}")
    print(f"\tProblems: {num_problems}")
    print(f"\tErrors: {num_errors}")

    for check in checks:
        if not isinstance(check.result, status.Ok):
            print(f"{check.name}")
            print(f"{indent(str(check.result))}")


if __name__ == "__main__":
    sys.exit(main())
