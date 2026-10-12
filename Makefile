# Magic Bot Workshop build helpers (arduino-cli in ~/tools, shares config with Arduino IDE 2.x)
#   make build SKETCH=firmware/phase0/blink
#   make flash SKETCH=firmware/phase0/blink
#   make monitor
#   make petbot-test

CLI    ?= $(HOME)/tools/arduino-cli/arduino-cli --config-file $(HOME)/.arduinoIDE/arduino-cli.yaml
FQBN   ?= esp32:esp32:XIAO_ESP32S3:PSRAM=opi,UploadSpeed=115200
PORT   ?= /dev/ttyACM0
SKETCH ?= firmware/phase0/blink
# esptool's stub loader drops the USB-serial link on this setup; the ROM loader works
UPLOAD_FLAGS ?= --upload-property upload.flags=--no-stub

.PHONY: build flash flash-safe monitor phase0 petbot-test

build:
	$(CLI) compile --fqbn $(FQBN) $(SKETCH)

flash: build
	$(CLI) upload --fqbn $(FQBN) -p $(PORT) $(UPLOAD_FLAGS) $(SKETCH)

monitor:
	$(CLI) monitor -p $(PORT) -c baudrate=115200

phase0:
	@for s in blink mic_test wifi_test sd_test camera_webserver; do \
		$(MAKE) --no-print-directory build SKETCH=firmware/phase0/$$s || exit 1; done

# Fun test menu for petbot (web page + OLED unicorn): flash it, then open the link it prints
petbot-test:
	$(MAKE) --no-print-directory flash SKETCH=firmware/petbot_test

# Upload in 256 KB pieces, each its own short esptool session. Use this when `make flash` dies partway
# ("Could not configure port: Input/output error" at ~76%), e.g. with a flaky USB cable.
#   make flash-safe SKETCH=firmware/petbot_test
ESPTOOL ?= python3 $(HOME)/.arduino15/packages/esp32/tools/esptool_py/4.5.1/esptool.py --chip esp32s3 -p $(PORT) --no-stub
BUILD   ?= /tmp/magic-bot-build
flash-safe:
	$(CLI) compile --fqbn $(FQBN) --output-dir $(BUILD) $(SKETCH)
	$(ESPTOOL) write_flash -z 0x0 $(BUILD)/$(notdir $(SKETCH)).ino.bootloader.bin 0x8000 $(BUILD)/$(notdir $(SKETCH)).ino.partitions.bin
	cd $(BUILD) && rm -f part_* && split -b 262144 -d $(notdir $(SKETCH)).ino.bin part_ && \
	i=0; for f in part_*; do $(ESPTOOL) write_flash -z $$(printf '0x%x' $$((0x10000 + i * 262144))) $$f || exit 1; i=$$((i + 1)); sleep 1; done

# ---- CYD (Cheap Yellow Display, ESP32-2432S028R, CH340 on /dev/ttyUSB0) ----
# The 2.8" CYD ships with one of two screen chips; pick it with CYD_SCREEN=st7789 or ili9341.
#   make cyd-flash CYD_SKETCH=firmware/cyd/cyd_screen_test CYD_SCREEN=st7789
CYD_FQBN   ?= esp32:esp32:esp32:UploadSpeed=460800
CYD_PORT   ?= /dev/ttyUSB0
CYD_SCREEN ?= st7789
CYD_SKETCH ?= firmware/cyd/cyd_screen_test
CYD_SETUP   = $(CURDIR)/firmware/cyd/screens/$(CYD_SCREEN).h
CYD_FLAGS   = --build-property "compiler.cpp.extra_flags=-DUSER_SETUP_LOADED=1 -include $(CYD_SETUP)" \
              --build-property "compiler.c.extra_flags=-DUSER_SETUP_LOADED=1 -include $(CYD_SETUP)"

.PHONY: cyd-build cyd-flash
cyd-build:
	$(CLI) compile --fqbn $(CYD_FQBN) $(CYD_FLAGS) $(CYD_SKETCH)

cyd-flash: cyd-build
	$(CLI) upload --fqbn $(CYD_FQBN) -p $(CYD_PORT) $(CYD_SKETCH)

# ---- CYD emulator: runs the real cyd_menu sketch on the laptop and saves pictures (needs g++ and Python PIL)
#   make sim   -> docs/screenshots/cyd-*.png
SIM_FLAGS = -std=gnu++17 -O1 -w -Isim/shim -I$(HOME)/Arduino/libraries/TFT_eSPI -include firmware/cyd/screens/$(CYD_SCREEN).h
.PHONY: sim
sim:
	@mkdir -p sim/out
	g++ $(SIM_FLAGS) sim/main.cpp -o sim/out/cyd-sim
	./sim/out/cyd-sim sim/out
	python3 sim/to_png.py sim/out docs/screenshots
