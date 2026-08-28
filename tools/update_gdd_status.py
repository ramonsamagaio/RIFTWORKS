#!/usr/bin/env python3
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GDD = ROOT / "docs" / "GDD.md"
STATUS = ROOT / "tools" / "gdd_status.json"

status = json.loads(STATUS.read_text(encoding="utf-8"))
text = GDD.read_text(encoding="utf-8")

for label, done in status.get("checklist", {}).items():
    if not done:
        continue
    escaped = re.escape(label)
    pattern = rf"(?m)^- \[[ xX]\] {escaped}$"
    replacement = f"- [x] {label}"
    text, _ = re.subn(pattern, replacement, text)

marker = "## Current implementation snapshot"
snapshot = status.get("snapshot", [])
block = marker + "\n\n" + "\n".join(f"- {item}" for item in snapshot) + "\n"
if marker in text:
    text = re.sub(r"(?s)## Current implementation snapshot\n.*?(?=\n## |\Z)", block.rstrip(), text)
else:
    text = text.rstrip() + "\n\n---\n\n" + block

GDD.write_text(text.rstrip() + "\n", encoding="utf-8")
print("GDD status synchronized")
