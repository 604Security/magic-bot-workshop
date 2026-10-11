# Prompts in CO-STAR form

**CO-STAR** = **C**ontext · **O**bjective · **S**tyle · **T**one · **A**udience · **R**esponse.
These are cleaned-up versions of real prompts from the Magic Bot Workshop sessions. The grown-up types them; the kid's
words go in quotes. Short real prompts worked too, because `CLAUDE.md` holds the standing context. CO-STAR shows
everything that context carries, so you can reuse it in a fresh project.

---

## 0. Setting up the workshop
- **Context:** I have an ESP32 robot project (repo X). My 7-year-old loves unicorns, animals, spy stuff, a bit of
  hacking, and Harry Potter. She doesn't type yet.
- **Objective:** Copy the project into a new repo for teaching her to vibe code. Let her name it.
- **Style:** A local web page of 10 numbered, animated name tiles with a 🔊 read-aloud button.
- **Tone:** Playful, sparkly, encouraging.
- **Audience:** A 7-year-old picks; a grown-up runs the commands.
- **Response:** The picker page opened in Chrome, then (after her pick) the new repo with kid-mode rules in CLAUDE.md.
  Ask before creating anything online.

## 1. Name the robot (first real build)
- **Context:** The robot has a 72×40 white OLED and a web page at petbot.local.
- **Objective:** Let her name the robot and show the name on the screen and web page.
- **Style:** Pixel-accurate previews of the real screen; names of 7 letters max.
- **Tone:** Celebrate the reveal.
- **Audience:** Kid picks the number; grown-up approves the flash.
- **Response:** Picker page → flash → "look at the screen!" plus one line of code shown as a magic spell.

## 2. Sensor → action (clap to dance)
- **Context:** The robot hears loudness 20×/second (quiet ~20, talking ~100–500).
- **Objective:** "When you clap, the unicorn dances."
- **Style:** 8 dance moves as moving previews; she may pick several.
- **Tone:** Party! 🎉
- **Audience:** Kid.
- **Response:** Build her pick; expose the clap threshold as a named number she can tweak. Then her tweak, verbatim:
  *"longer dance with way more hearts"*.

## 3. Make a game fair (tuning)
- **Context:** Catch the Snitch: clap when the snitch is in the box.
- **Objective:** Her feedback: *"too hard, needs to stay in the box longer… maybe if it was in the box 2 seconds ago
  it will be caught"*.
- **Style:** Keep her rule's name in the code (`CATCH_GRACE_MS`, "Megaspy's rule").
- **Tone:** Credit her as the game designer.
- **Audience:** Kid + grown-up.
- **Response:** Implement her rule, re-test, reset the scores so the high score is hers.

## 4. Randomness and fairness
- **Context:** Sorting Hat picks 1 of 4 houses.
- **Objective:** First *"make Gryffindor come up more"* (her house), then *"it comes up too much, make it even"*.
- **Style:** One tunable number (`GRYFFINDOR_CHANCE`, out of 100).
- **Tone:** Turn it into a lesson: you tried it, noticed it wasn't fair, and fixed it.
- **Audience:** Kid.
- **Response:** Change the number, flash, explain "25 out of 100 each".

## 5. Look & feel
- **Context:** Kid web page on the robot; she likes Harry Potter.
- **Objective:** "Reskin the web page to be Harry Potter themed."
- **Style:** 8 mini live previews of the menu in different looks; the chosen theme applied as one removable CSS block.
- **Tone:** Magical.
- **Audience:** Kid picks.
- **Response:** Picker → apply → screenshot check → open the real page.

## 6. Privacy by design
- **Context:** The app should greet her by her real name, but the repo is shared on GitHub.
- **Objective:** Use her real name in the app, her spy name ("Megaspy") anywhere public.
- **Style:** A git-ignored `config.h` with a committed `config.h.example`.
- **Tone:** Matter-of-fact; explain it to the grown-up.
- **Audience:** Grown-up.
- **Response:** Config file + fallback; verify the real name isn't in any committed file.

## 7. Two devices talking
- **Context:** A second board (colour touchscreen) on the same Wi-Fi as the robot.
- **Objective:** "Something that can connect and run the camera and send the image over."
- **Style:** Reuse the robot's existing web API; no changes on the robot.
- **Tone:** Grown-up test first (a surprise for the kid later).
- **Audience:** Grown-up.
- **Response:** Identify the board, ask before overwriting it, flash a screen test, then the camera viewer.

## 8. Logic, gently (planned)
- **Context:** She has picked a game; see `LEARNING.md`.
- **Objective:** Introduce IF/THEN/ELSE *inside* her game, one level per session; later "friends & feelings" rules
  (IF the friend says no, THEN what else could we try?) that always end in success after a couple of tries.
- **Style:** Moving pictures (a token travels to a fork and goes to THEN or ELSE); Harry Potter themes.
- **Tone:** Easy and slow; celebrate trying again.
- **Audience:** Kid.
- **Response:** One rule she builds, the real `if` line shown in green "hacker" text, and a badge.

---

### Template
```
Context:   <project, hardware, what the kid loves, what already exists>
Objective: "<the kid's idea in their own words>"
Style:     <pick-a-number page / moving previews / one tunable number / ...>
Tone:      <playful, celebratory, gentle...>
Audience:  <kid picks / grown-up approves>
Response:  <picker → build → flash → where to look → one line of code as a magic spell>
```
