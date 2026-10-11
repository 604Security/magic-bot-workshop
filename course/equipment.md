# 🧰 Equipment used so far

Everything here was used for real in the Magic Bot Workshop. Prices are rough (2026) and change. Check before buying.

## The robot: "Sparkle"

### Seeed **XIAOML Kit** (~US$39): https://www.seeedstudio.com/The-XIAOML-Kit.html
The companion kit for Harvard's free *Machine Learning Systems* book labs (https://mlsysbook.ai). It contains:

| Part | What it does in the workshop |
|---|---|
| **XIAO ESP32S3 Sense** (thumb-sized board) | The brain: dual-core 240 MHz, 8 MB PSRAM, 8 MB flash, Wi-Fi + Bluetooth, USB-C |
| **Camera** (OV3660; OV2640 on older Sense boards) | Photos and live video ("Eyes", Spy Camera) |
| **Built-in microphone** | Hears claps (dance, Snitch, Sorting Hat) |
| **Expansion board: IMU** (LSM6DS3TR-C motion sensor) | Feels shakes and wand swishes |
| **Expansion board: 0.42" OLED** (72×40, white only) | The robot's face: unicorn, owl, cauldron, spells |
| **32 GB microSD card** | Memory test; later, sound/photo recordings for AI training |
| **Wi-Fi antenna** | Clip it on; it helps the signal a lot |

Tips:
- **Take the heat sink off the camera** before fitting the expansion board.
- The OLED is tiny and one-colour. Design pixel art at 72×40 (the pick-a-number pages show real-size previews).
- It runs from USB power alone; a 3.7 V LiPo battery can plug into the expansion board to make it portable.

## Sparkle's friend: the **"Cheap Yellow Display" (CYD)** (~US$10–20)
- Model **ESP32-2432S028R**: ESP32 + 2.8" 320×240 **colour touchscreen** + RGB LED + speaker connector + microSD slot.
- USB chip **CH340** (appears as `/dev/ttyUSB0` on Linux).
- ⚠️ **Same board, two possible screen chips**: **ST7789** or **ILI9341**. You can't tell from outside. Flash a
  colour-bar test with each setting; the one where "RED" is red is yours. (Ours: ST7789.)
- Screen settings borrowed from the open-source **SquachWatch-CYD** project
  (https://github.com/604Security/SquachWatch-CYD), which supports both chips.

## Cables, computer and network
- **A good USB-C *data* cable.** Charge-only cables don't show up at all. A worn cable can work for small uploads
  and then **fail at ~76% of a big one**. We hit this; the workaround is uploading in small pieces (`make flash-safe`).
- **Plug straight into the laptop**, not through a dock or hub.
- **Laptop:** ours runs Linux. Anything that runs the Arduino tools and Chrome will do.
- **2.4 GHz home Wi-Fi.** ESP32 boards can't use 5 GHz. Robot and touchscreen join the same network.

## Software (all free)
| Tool | Used for |
|---|---|
| **Claude Code** (AI coding assistant in the terminal) | Writes the code, builds the pick-a-number pages, flashes the boards |
| **arduino-cli** 1.5 (+ Arduino IDE 2 optional) | Compiling and uploading |
| **esp32 board package 2.0.17** by Espressif | Pinned on purpose: the Harvard/Edge Impulse ML labs need 2.0.x |
| Libraries: **U8g2** (OLED), **TFT_eSPI** (CYD screen), **XPT2046_Touchscreen** (CYD touch), **TJpg_Decoder** (show photos on the CYD) | |
| **Google Chrome** | Pick-a-number pages (with read-aloud) and the robot's web page |
| **git + GitHub** | Saving snapshots; sharing |

## Gotchas we hit (so you don't have to)
- **Upload drops right after "Stub running…"** on the XIAO's built-in USB: upload with esptool's `--no-stub`.
- **Upload dies at ~76%** with "Input/output error": a flaky cable. Upload in pieces, then replace the cable.
- **Mic + camera together:** start the microphone *before* the camera, or camera photos time out.
- **Garbled serial text:** wrong speed. Our robot uses 115200; SquachWatch on the CYD used 2,000,000.
- **Web page blank right after flashing:** give the robot a few seconds, then refresh.
