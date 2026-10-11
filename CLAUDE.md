# Magic Bot Workshop 🪄🤖

A coding workshop where a 7-year-old **vibe codes** her robot: she has the ideas, Claude writes the code, and
she sees it come alive on the robot. The robot is a Seeed **XIAO ESP32S3 Sense** (XIAOML Kit: camera, mic,
OLED, IMU), copied (with git history) from the `ai-petbot` repo on 2026-10-10. She chose the name (option 9).

She loves **unicorns, animals, spy stuff, a bit of hacking, and Harry Potter**.
She goes by **Megaspy** (her spy name, from the SIA spy game). The project is credited to her: use "Megaspy", never her real name, in anything committed.
The app greets her by her real name from `firmware/petbot_test/config.h` (`KID_NAME`, gitignored). Committed fallback and
`config.h.example` say "Megaspy". Never write the real name into a tracked file.

## Kid-mode rules (read every session)

**Who does what**
- **She** brings the ideas, names things, and picks options by saying a number. She does **not** type.
- **The grown-up** (the user) does the typing and approves anything technical or anything that goes online
  (git push, new services, internet access).
- **Claude** turns her idea into choices, builds the one she picks, flashes it, and celebrates.

**The loop for every idea**
1. Write the idea in `IDEAS.md` in her words, even if it's silly.
2. Make a **pick-a-number page** in `pickers/NN-<idea>.html`: about 6–10 numbered tiles, each one a *moving* preview
   (pixel-accurate for the 72×40 OLED), plus a 🔊 "Say it" button (speechSynthesis), since she may not read every word.
   Check it with a headless Chrome screenshot, then open it in Chrome.
3. She says a number. Record the pick in `IDEAS.md`.
4. Build it, `make flash`, and tell her exactly where to look (OLED, http://sparkle.local, the LED).
5. Tweaks ("faster!", "more pink!") are the best part: do them quickly and flash again.
6. When she's happy: give her a badge in `IDEAS.md` and commit (explain to the grown-up in plain words).

**How to talk**
- Use short sentences and lots of emoji. Tell her what's happening ("I'm teaching the robot a new trick…").
- Call errors **"oopsies"**. Fix them quietly and give the grown-up the real details.
- Every so often, show her a tiny bit of the real code ("see this number? that's how fast the unicorn blinks")
  and explain it like a magic spell. Keep it to **one line**, and only when it's fun.
- Theme it with unicorns, animals, spy missions, secret codes, and "hacker" green text. Hacking means
  *her own* robot only (passwords on her pages, secret ciphers, find-the-bug games), never other people's stuff.

**Safety**
- Nothing personal on the robot or in the repo: no full names, school, address, or photos of faces in commits.
- The robot stays on the home network. No cloud or internet services unless the grown-up says yes.
- Never `git push`, create repos, or delete things without the grown-up's OK.

**Where things live**
- `firmware/petbot_test/`: the robot's web menu + OLED unicorn. This is the main sketch her ideas go into.
  Flash it with `make petbot-test`, then open http://sparkle.local (or the IP on serial).
- `IDEAS.md`: her Idea Board (ideas, picks, badges).
- `pickers/`: every pick-a-number page she has used.
- `ROADMAP.md`: the grown-up's TinyML plan from ai-petbot (wake word, pet vision), handy for "make it hear me" or
  "make it see the cat" ideas.

## Hardware

### Seeed XIAO ESP32S3 Sense
- SoC: ESP32-S3R8, dual-core Xtensa LX7 @ 240 MHz
- Memory: 512 KB SRAM, **8 MB OPI PSRAM**, 8 MB flash
- Camera: detachable DVP camera (OV2640 on the original Sense, OV3660 in the XIAOML Kit;
  OV5640 compatible), max 1600×1200
- Microphone: onboard PDM digital mic — CLK **GPIO42**, DATA **GPIO41** (I2S/PDM)
- microSD: FAT32, up to 32 GB, CS **GPIO21**
- Wi-Fi 2.4 GHz + BLE 5.0, U.FL antenna connector
- USB-C for power and programming; LiPo charge circuit; deep sleep ~14 µA
- Size: 21 × 17.5 mm

### XIAOML Kit IMU/OLED expansion board (if used)
- IMU: 6-axis **LSM6DS3TR-C**, I2C addr **0x6A** — library "Seeed Arduino LSM6DS3"
- OLED: 0.42" mono, 72×40 px, SSD1315 (SSD1306-compatible), I2C addr **0x3C** — library "U8g2"
- Uses the XIAO default I2C pins (D4 = SDA, D5 = SCL)
- Reset button, 3.7 V battery connector
- **Remove the camera heat sink before fitting the expansion board.**

### CYD: Sparkle's friend (Cheap Yellow Display, added 2026-10-11)
- **ESP32-2432S028R**, 2.8" 320x240 colour touchscreen. ESP32-D0WD-V3, 4 MB flash, **CH340 USB → `/dev/ttyUSB0`**.
- Screen chip: **ST7789** (verified with `firmware/cyd/cyd_screen_test`: colour bars correct). Screen settings come from
  the user's fork https://github.com/604Security/SquachWatch-CYD (`include/cyd_user_setup.h`), saved as
  `firmware/cyd/screens/st7789.h` (+ `ili9341.h` for the other variant). Backlight GPIO21.
- Touch: XPT2046 on its own SPI: CLK 25, MISO 39, MOSI 32, CS 33, IRQ 36. Also has an RGB LED and a speaker connector.
- Build/flash: **`make cyd-flash CYD_SKETCH=firmware/cyd/<sketch> CYD_SCREEN=st7789`** (FQBN `esp32:esp32:esp32`).
  TFT_eSPI 2.5.43 is configured by `-include`-ing the screen header (no editing the library's User_Setup).
- It was running SquachWatch (serial 2,000,000 baud) before we overwrote it; flash SquachWatch back from the fork if needed.

## Toolchain

- Arduino IDE 2.x (AppImage in `~/tools/`)
- Board package: **"esp32 by Espressif Systems"**
  - Board: `XIAO_ESP32S3` (FQBN `esp32:esp32:XIAO_ESP32S3`)
  - Tools → PSRAM: **OPI PSRAM** (required for camera + ML)
  - Do **not** use the "Arduino ESP32 Boards" package (`arduino:esp32`) — that is for the Nano ESP32.
- **Version:** esp32 core pinned at **2.0.17** (downgraded from 3.3.12 on 2026-09-28), as the
  Harvard/Rovai labs require for Edge Impulse–exported libraries. Do not update it in Boards Manager.
  Lab sketches written for 3.x (e.g. `XIAOML_Kit_Mic_Test`, which uses `ESP_I2S.h`) won't build on it;
  use the 2.x `I2S.h` API instead.
- CLI: `arduino-cli` 1.5.1 in `~/tools/arduino-cli/`, using the IDE's config
  (`~/.arduinoIDE/arduino-cli.yaml`). The repo `Makefile` wraps it:
  `make build|flash SKETCH=firmware/phase0/blink`, `make monitor`, `make phase0` (build all).
  FQBN with options: `esp32:esp32:XIAO_ESP32S3:PSRAM=opi,UploadSpeed=115200`.
  Run `make` from the repo root.
- Serial port: `/dev/ttyACM0` (native USB). If upload fails, hold BOOT while plugging in.
- Camera pin map: `#define CAMERA_MODEL_XIAO_ESP32S3`
- ML: Edge Impulse Studio (train → export Arduino library), plus local
  `ArduTFLite` / `Chirale_TensorFlowLite` libraries in `~/Arduino/libraries`
- OLED: U8g2 2.36 installed; constructor `U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE)`.
  The OLED is one-colour (white), so colour art goes on the web page instead.

### USB / upload troubleshooting (learned 2026-09-28)
- The board does **not** mount as a USB drive. It shows up as a serial port: `lsusb` lists
  `303a:1001 Espressif USB JTAG/serial debug unit`, and `/dev/ttyACM0` appears.
  Check with `watch -n1 'lsusb | grep -i 303a; ls /dev/ttyACM*'`.
- Not in `lsusb` at all → the cable is likely charge-only (use a data cable), or the board is behind the dock/hub.
- **Plug the board directly into the laptop**, not through the ThinkPad USB-C dock.
- **Upload fails with "No serial data received" / "Unable to verify flash chip connection"**
  right after "Stub running..." → esptool's stub loader drops the link. Changing the baud rate
  (921600, 115200, even 9600) does not help, because the port is virtual.
  **Fix: `--no-stub`**, which `make flash` passes by default (`UPLOAD_FLAGS` in the Makefile).
  The Arduino IDE Upload button can't pass it, so always flash with `make flash`.
- Quick link check: `python3 ~/.arduino15/packages/esp32/tools/esptool_py/4.5.1/esptool.py --chip esp32s3 -p /dev/ttyACM0 --no-stub flash_id`
  (should report 8 MB flash).
- Reading serial non-interactively (e.g. from Claude's shell): `arduino-cli monitor` under `timeout`/pipes
  prints nothing. Use `stty -F /dev/ttyACM0 115200 raw -echo; timeout 5 cat /dev/ttyACM0` instead.
  `make monitor` is fine in an interactive terminal.
- **Mic + camera together:** start I2S (the mic) *before* `esp_camera_init()`. If I2S starts after the
  camera (even once, then stopped), `esp_camera_fb_get()` times out from then on. See `petbot_test.ino`.
- **Upload dies at ~76% with "Could not configure port: (5, 'Input/output error')"** and the board drops off USB
  (seen 2026-10-11 after the laptop was moved; cable suspected): use **`make flash-safe SKETCH=firmware/petbot_test`**,
  which uploads in 256 KB pieces. Each piece finishes in ~2 s and verifies its hash.
- ModemManager is running and can probe ACM ports. It didn't block uploads, but if the port
  acts up: `sudo systemctl stop ModemManager`.

## Reference material

### Harvard "Machine Learning Systems" book (CS249r)
- Free open textbook by Prof. Vijay Janapa Reddi (Harvard) and contributors: https://mlsysbook.ai
- Source: https://github.com/harvard-edge/cs249r_book
- Hardware labs for this board were written with Marcelo Rovai (UNIFEI):
  - Kit overview: https://mlsysbook.ai/kits/contents/seeed/xiao_esp32s3/xiao_esp32s3.html
  - Setup: https://mlsysbook.ai/kits/contents/seeed/xiao_esp32s3/setup/setup.html
  - Image classification: https://mlsysbook.ai/kits/contents/seeed/xiao_esp32s3/image_classification/image_classification.html
  - Object detection: https://mlsysbook.ai/kits/contents/seeed/xiao_esp32s3/object_detection/object_detection.html
  - Keyword spotting: https://mlsysbook.ai/kits/contents/seeed/xiao_esp32s3/kws/kws.html
  - Motion classification & anomaly detection: https://mlsysbook.ai/kits/contents/seeed/xiao_esp32s3/motion_classification/motion_classification.html

### Seeed XIAOML Kit
- The companion hardware kit for the book (XIAO ESP32S3 Sense + IMU/OLED board, antenna,
  32 GB microSD, ~$39): https://www.seeedstudio.com/The-XIAOML-Kit.html

### Marcelo Rovai's lab code (local clone)
- `~/workstation/github/Mjrovai/XIAO-ESP32S3-Sense` (upstream: https://github.com/Mjrovai/XIAO-ESP32S3-Sense)
  - `XIAOML_Kit_code/` — kit sketches: image classification (+OLED), KWS (+OLED),
    motion classification/anomaly detection, IMU/OLED/mic tests
  - `Camera_HTTP_Server_STA/`, `Streeming_Video/` — camera over Wi-Fi
  - `take_photos_command/`, `camera_round_display_save_jpeg/` — dataset capture
  - `Wav_Record_dataset/`, `xiao_esp32s3_microphone*/` — audio capture
  - PDFs of the image classification, object detection, motion, and KWS tutorials

### Seeed docs
- XIAO ESP32S3 getting started: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/


## Repo conventions

- Firmware sketches live in `firmware/<area>/<sketch>/<sketch>.ino` (or `firmware/<sketch>/`).
- Wi-Fi credentials go in a per-sketch `secrets.h` (gitignored); copy it from `secrets.h.example`.
- Installers/tooling live in `~/tools/`, not in this repo (see `.gitignore`).
- Keep "Status" below and `IDEAS.md` current at the end of each session.

## Status

- [x] 2026-10-10: workshop created from ai-petbot; she named it **Magic Bot Workshop** (`pickers/01-lab-name.html`, pick 9)
- [x] 2026-10-10: she named the robot **Sparkle** (`pickers/02-robot-name.html`, pick 1). `ROBOT_NAME`/`ROBOT_HOST` in
  `firmware/petbot_test/petbot_test.ino`; the OLED says "Hi! Sparkle"; the web page is http://sparkle.local (flashed + verified)
- [x] 2026-10-11: Unicorn Dance Party (`pickers/03-dance-party.html`, pick 8 Heart Twirl). A clap (`CLAP_LEVEL` 1200) on the
  home or Dance page starts an 8 s dance with 10 hearts; web 💃 Dance page + `/api/dance`. Real claps tested by her ✅
- [x] 2026-10-11: Wizard School (`pickers/04-wizard-school.html`): Owl Post (pick 3) ⭐, Catch the Snitch (pick 7) ⭐, Sorting Hat ⭐ (pick 2,
  `GRYFFINDOR_CHANCE` 25 = all houses even (55 was "too much"); shake via IMU `shaken()` sorts/resets, `SHAKE_G`). Each has a web panel + `/api/owl`, `/api/snitch`, `/api/hat` and an OLED view.
- [x] 2026-10-11: Web reskin **Potions Dungeon** (`pickers/05-wizard-makeover.html`, pick 7; one THEME block at the end of the CSS).
  Potion Mixer ⭐ (`/api/potion`, 10x10 `ICONS`), Wand Wave ⭐ (`/api/wand`, IMU swish `WAVE_G`).
- [x] 2026-10-11: Main page menu: ✨ Magic Stuff (#magic) and 🔧 Tests (#tests); name via `/api/info` `kid` → `.kid` spans.
- [x] 2026-10-11: CYD **Spy Camera** ⭐ (`firmware/cyd/cyd_spycam`): CYD finds sparkle.local via mDNS, pulls `/capture` JPEGs, shows
  them at 1/2 scale (TJpg_Decoder). "Amazing!" To be added to the SIA spy game (SIA-ESP32_spy repo) later.
- **Plan: final game menu.** When the experiments are done, take every ⭐ keeper in `IDEAS.md` and build a polished game
  menu on the web page. Keep the keeper code intact until then (don't refactor it away).
