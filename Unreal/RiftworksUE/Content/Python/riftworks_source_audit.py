from __future__ import annotations

import ast
import os

import unreal
import riftworks_setup as rw


PYTHON_DIR = os.path.abspath(os.path.dirname(__file__))


def _is_unreal_rotator(call: ast.Call) -> bool:
    func = call.func
    return (
        isinstance(func, ast.Attribute)
        and func.attr == "Rotator"
        and isinstance(func.value, ast.Name)
        and func.value.id == "unreal"
    )


def _is_project_rotator(call: ast.Call) -> bool:
    func = call.func
    return (
        isinstance(func, ast.Attribute)
        and func.attr == "rotator"
        and isinstance(func.value, ast.Name)
        and func.value.id == "rw"
    )


def _scan_file(path: str) -> list[str]:
    violations: list[str] = []
    try:
        with open(path, "r", encoding="utf-8") as handle:
            source = handle.read()
        tree = ast.parse(source, filename=path)
    except Exception as exc:
        return [f"{os.path.basename(path)}: parse failed: {exc}"]

    for node in ast.walk(tree):
        if not isinstance(node, ast.Call):
            continue

        # Unreal Python exposes Rotator positional arguments in (roll, pitch, yaw)
        # order. RIFTWORKS never permits positional values here because most
        # gameplay/design code reasons in the C++/editor convention
        # (pitch, yaw, roll). Zero-argument construction is harmless.
        if _is_unreal_rotator(node) and node.args:
            violations.append(
                f"{os.path.basename(path)}:{node.lineno}: unreal.Rotator uses positional arguments"
            )

        # Keep the central project helper explicit too. This prevents a future
        # caller from hiding the same ambiguity behind rw.rotator(0, 90, 0).
        if _is_project_rotator(node) and node.args:
            violations.append(
                f"{os.path.basename(path)}:{node.lineno}: rw.rotator uses positional arguments; use named pitch/yaw/roll"
            )

    return violations


def audit_sources(strict: bool = True) -> list[str]:
    violations: list[str] = []
    scanned = 0

    for name in sorted(os.listdir(PYTHON_DIR)):
        if not name.endswith(".py"):
            continue
        path = os.path.join(PYTHON_DIR, name)
        if not os.path.isfile(path):
            continue
        scanned += 1
        violations.extend(_scan_file(path))

    if violations:
        for violation in violations:
            unreal.log_error(f"[RIFTWORKS ROTATION AUDIT] {violation}")
        message = f"Rotation source audit FAILED: {len(violations)} violation(s) across {scanned} Python files"
        if strict:
            raise RuntimeError(message)
        unreal.log_warning(f"[RIFTWORKS ROTATION AUDIT] {message}")
    else:
        unreal.log(
            f"[RIFTWORKS ROTATION AUDIT] PASS: {scanned} Python files, zero positional Rotator calls"
        )

    return violations


def apply_all() -> None:
    audit_sources(strict=True)


if __name__ == "__main__":
    apply_all()
