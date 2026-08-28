#!/usr/bin/env python3
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GDD = ROOT / "docs" / "GDD.md"
STATUS = ROOT / "tools" / "gdd_status.json"

status = json.loads(STATUS.read_text(encoding="utf-8"))
text = GDD.read_text(encoding="utf-8")

if status.get("perspective") == "first_person":
    text = text.replace(
        "- [x] Basic third-person placeholder movement/camera.",
        "- [x] First-person movement/camera foundation."
    ).replace(
        "- [ ] Basic third-person placeholder movement/camera.",
        "- [x] First-person movement/camera foundation."
    )

for label, done in status.get("checklist", {}).items():
    if not done:
        continue
    escaped = re.escape(label)
    pattern = rf"(?m)^- \[[ xX]\] {escaped}$"
    replacement = f"- [x] {label}"
    text, _ = re.subn(pattern, replacement, text)

if status.get("perspective") == "first_person":
    perspective_block = """## 4. Perspective and controls

RIFTWORKS is a **first-person game**. This is a locked project decision, not a temporary prototype preference.

First-person is central to the intended experience because it strengthens:

- the physicality of scavenging and manipulating machinery;
- the scale and terror of Colossi;
- immersion in darkness, flashlight use and underground exploration;
- precision when wiring, assembling and operating player-built systems;
- the feeling that the player personally inhabits the base and infrastructure they create.

The player body may still exist invisibly for collision, shadows and systemic interactions, but normal gameplay presentation remains first-person. Core systems should be designed around this perspective rather than requiring a third-person camera.

The movement target is grounded, responsive and readable rather than simulation-heavy. Camera motion should remain restrained: subtle head movement is welcome, but excessive bob, sway or forced cinematic motion must not interfere with long play sessions.

Desktop gameplay should launch in **fullscreen** by default. UI and rendering must scale cleanly across common desktop aspect ratios and resolutions.
"""
    text, count = re.subn(
        r"(?s)## 4\. Perspective and controls\n.*?(?=\n---\n\n## 5\.)",
        perspective_block.rstrip(),
        text,
        count=1,
    )
    if count == 0:
        raise RuntimeError("Could not locate GDD perspective section for first-person lock")

marker = "## Current implementation snapshot"
snapshot = status.get("snapshot", [])
block = marker + "\n\n" + "\n".join(f"- {item}" for item in snapshot) + "\n"
if marker in text:
    text = re.sub(r"(?s)## Current implementation snapshot\n.*?(?=\n## |\Z)", block.rstrip(), text)
else:
    text = text.rstrip() + "\n\n---\n\n" + block

GDD.write_text(text.rstrip() + "\n", encoding="utf-8")
print("GDD status synchronized")
