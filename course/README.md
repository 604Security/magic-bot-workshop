# 🪄 Vibe Coding for Kids: course outline (rough draft)

A hands-on course where a child (age ~6–10) **designs** and an AI coding assistant (Claude Code) **builds**, on a real
robot. The child never has to type: they bring ideas, pick from moving "pick-a-number" previews, and ask for tweaks.
A grown-up drives the keyboard, approves anything technical or online, and turns each build into a small lesson.

**Shared in the hope that other families find it useful.** Built from the real **Magic Bot Workshop** sessions, created by **Megaspy** (age 7) with the robot **Sparkle**,
and **DFIR-life** (parent & mentor).

## Who it's for
- **Kid:** ages 6–10, no reading fluency needed (every choice has 🔊 read-aloud). Loves stories, animals, magic, spies.
- **Grown-up:** comfortable with a terminal, doesn't need to be a programmer. Learns prompting along the way.

## Hardware
- **Sparkle:** Seeed XIAO ESP32S3 Sense in the XIAOML Kit (camera, mic, motion sensor, tiny 72×40 screen), ~US$39.
- **Optional friend:** "Cheap Yellow Display" (ESP32-2432S028R), 2.8" colour touchscreen, ~US$10–20.
- A laptop, a good USB-C **data** cable, home Wi-Fi.
- Full list, tips and gotchas: **[equipment.md](equipment.md)**.

## The method (used in every lab)
1. **Idea**: the kid says it in their own words; write it on the Idea Board (`IDEAS.md`).
2. **Pick-a-number page**: the AI makes 6–10 numbered, *moving* previews with a 🔊 button.
3. **Pick**: the kid says a number.
4. **Build & flash**: the AI writes the code and puts it on the robot; tell the kid exactly where to look.
5. **Tweak**: "longer!", "more hearts!", "too hard!" The best part, and real iteration.
6. **Badge & save**: a badge on the Idea Board, and a commit ("a saved snapshot") explained in plain words.

**Roles:** kid = designer/boss, grown-up = keyboard + safety, AI = builder.
**Safety:** her spy name in anything public (real name only in a git-ignored config file), robot stays on the home
network, the grown-up approves pushes and anything online.

## Labs

| # | Lab | What the kid does | Big idea (kid words) | Grown-up / CS concept |
|---|-----|-------------------|----------------------|------------------------|
| 0 | **Name your lab** | Picks a name from 10 animated tiles | "I'm the boss of the ideas" | Repos, forking, the pick-a-number loop |
| 1 | **Name your robot** | Picks a name; sees it on the robot's screen | "Computers do what the words say" | Constants (`#define ROBOT_NAME`), flashing firmware |
| 2 | **Clap to dance** | Picks a dance; asks for "longer, more hearts" | "Robots can hear" | Sensors → actions, thresholds (`CLAP_LEVEL`), animation timing |
| 3 | **Owl Post** | Sends letters to the robot | "Messages travel through the air" | Web pages talking to devices (HTTP API) |
| 4 | **Catch the Snitch** | Plays; says "too hard"; invents the 2-second rule | "Game designers make games fair" | Game loops, timing windows, difficulty tuning |
| 5 | **Sorting Hat** | Gets sorted; notices one house wins too much | "Random isn't always fair" | Randomness, probability, weighting (`GRYFFINDOR_CHANCE`) |
| 6 | **Shake it** | Shakes the robot to sort / reset | "Robots can feel moving" | Accelerometers (IMU), detecting gestures |
| 7 | **Potion Mixer** | Picks 3 ingredients, discovers 12 potions | "Same recipe, same result" | Combinations, deterministic functions, collections (potion book) |
| 8 | **Wand Wave** | Swishes the robot like a wand | "One big move = one spell" | Thresholds vs. patterns, cooldowns |
| 9 | **Makeover** | Picks a whole new look for the web page | "Looks are code too" | CSS themes, design choices, fonts |
| 10 | **Menus & secrets** | Picks menu names (Hogwarts, Great Hall) | "Keep your real name secret" | Navigation, config files, `.gitignore`, privacy |
| 11 | **A robot friend** | Watches the robot's camera on another screen | "Robots can talk to each other" | Two devices, Wi-Fi, mDNS, sending images |
| 12 | **Magic rules** | Builds IF/THEN/ELSE rules in a game | "IF this THEN that ELSE…" | Conditionals; see `../LEARNING.md` (try-again loops, friends & feelings, AND/OR/NOT, find the bug) |

Each lab will get its own page in `course/labs/` with: goal, materials, the CO-STAR prompts (`prompts-costar.md`),
what to say to the kid, the "magic spell" line of real code to show, common oopsies, and stretch ideas.

## Lessons learned so far (for grown-ups)
- **Let the kid's words lead.** "Way more hearts" is a better spec than anything we'd write.
- **Ask, build, show, tweak** beats explaining. One line of real code per lab, framed as a magic spell.
- **Kids are great testers.** She found the Snitch too hard and Gryffindor too common, and fixed both.
- **Don't polish what doesn't matter.** Spend the time on fun features, not cosmetic bugs.
- **Emulate before you flash.** A tiny laptop emulator for the colour screen caught invisible text and scrambled
  colours before they reached her (`make sim`).
- **Hardware is real.** Loose cables happen; have a plan B (`make flash-safe`).
