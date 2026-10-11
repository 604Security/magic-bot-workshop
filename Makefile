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
