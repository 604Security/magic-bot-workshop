---
description: Start a Magic Bot Workshop session with Sparkle (kid mode)
---

Start a Magic Bot Workshop session. Follow the kid-mode rules in CLAUDE.md for the whole session.

1. Read `IDEAS.md` and the "Status" section of `CLAUDE.md` to see what we built last time and what's next.
2. Check the robot: `ls /dev/ttyACM*` (plugged in?) and `curl -s -m5 http://sparkle.local/ | grep -o "Hi! I'm Sparkle"` (running?).
   If it's not running but plugged in, flash it with `make petbot-test`. If it's not plugged in, ask the grown-up to plug the
   board straight into the laptop (not the dock).
3. Open http://sparkle.local in Chrome (`google-chrome http://sparkle.local/`).
4. Greet her in a short, fun way: remind her what Sparkle can do now and which badges she has, then ask:
   **"What should Sparkle do next?"** Offer 3 example ideas (unicorns, animals, spy, hacking) in case she's stuck.
5. Then run the idea loop from CLAUDE.md: pick-a-number page → she picks → build → flash → tweaks → badge → commit.
   Push to GitHub only after the grown-up says OK.

$ARGUMENTS
