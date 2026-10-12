#!/usr/bin/env python3
"""Make the yellow screen's badge list from Megaspy's Idea Board (IDEAS.md).

Every line like "- 🏅 **Seeker**: invented the 2-second catch rule ... (2026-10-11)" becomes a badge.
  python3 tools/badges.py > firmware/cyd/cyd_menu/badges.h      (make cyd-build does this for you)
"""
import re

badges = []
for line in open("IDEAS.md", encoding="utf-8"):
    m = re.match(r"- 🏅 \*\*(.+?)\*\*:\s*(.*?)\s*(?:\((\d{4}-\d{2}-\d{2})\))?\s*$", line)
    if m:
        name, what, date = m.group(1), m.group(2), m.group(3) or ""
        badges.append((name, what, date))

def c(s):  # plain ASCII for the screen's fonts
    s = s.replace("’", "'").replace("“", '"').replace("”", '"').replace("—", "-").replace("–", "-")
    s = s.encode("ascii", "ignore").decode().replace("\\", "\\\\").replace('"', '\\"')
    return s.strip()

print("// Generated from IDEAS.md by tools/badges.py. Don't edit by hand: add badges to IDEAS.md.")
print("#pragma once")
print(f"#define BADGE_COUNT {len(badges)}")
print("const char* const BADGE_NAMES[BADGE_COUNT > 0 ? BADGE_COUNT : 1] = {")
for name, _, _ in badges or [("", "", "")]:
    print(f'  "{c(name)}",')
print("};")
print("const char* const BADGE_DATES[BADGE_COUNT > 0 ? BADGE_COUNT : 1] = {")
for _, _, date in badges or [("", "", "")]:
    print(f'  "{date}",')
print("};")
