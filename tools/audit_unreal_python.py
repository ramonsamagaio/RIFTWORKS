#!/usr/bin/env python3
"""Static sanitation checks for RIFTWORKS Unreal editor Python.

The Unreal Python Rotator constructor exposes positional fields in an order that
is easy to confuse with C++ FRotator. RIFTWORKS therefore forbids positional
arguments entirely and requires explicit roll=/pitch=/yaw= keywords.
"""

from __future__ import annotations

import ast
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
PYTHON_ROOT = ROOT / "Unreal" / "RiftworksUE" / "Content" / "Python"


def is_unreal_rotator(call: ast.Call) -> bool:
    func = call.func
    return (
        isinstance(func, ast.Attribute)
        and func.attr == "Rotator"
        and isinstance(func.value, ast.Name)
        and func.value.id == "unreal"
    )


def audit_file(path: Path) -> list[str]:
    issues: list[str] = []
    try:
        source = path.read_text(encoding="utf-8")
        tree = ast.parse(source, filename=str(path))
    except (OSError, SyntaxError) as exc:
        return [f"{path.relative_to(ROOT)}: parse/read failure: {exc}"]

    lines = source.splitlines()
    for node in ast.walk(tree):
        if isinstance(node, ast.Call) and is_unreal_rotator(node) and node.args:
            text = lines[node.lineno - 1].strip() if 0 < node.lineno <= len(lines) else ""
            issues.append(
                f"{path.relative_to(ROOT)}:{node.lineno}: positional unreal.Rotator is forbidden: {text}"
            )
    return issues


def main() -> int:
    if not PYTHON_ROOT.is_dir():
        print(f"Missing Unreal Python directory: {PYTHON_ROOT}", file=sys.stderr)
        return 2

    issues: list[str] = []
    files = sorted(PYTHON_ROOT.glob("*.py"))
    for path in files:
        issues.extend(audit_file(path))

    if issues:
        print("RIFTWORKS Unreal Python transform audit FAILED")
        for issue in issues:
            print(f" - {issue}")
        print(f"\n{len(issues)} positional Rotator call(s) found across {len(files)} Python files.")
        return 1

    print(f"RIFTWORKS Unreal Python transform audit passed across {len(files)} Python files.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
