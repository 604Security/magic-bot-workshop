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

.PHONY: build flash monitor phase0 petbot-test

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
