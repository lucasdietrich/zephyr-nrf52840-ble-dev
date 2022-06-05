WEST_PATH = ../.venv/bin/west
SN_DK = 683339521
SN_ACN = 50106017
SN = $(SN_ACN)

build_dk:
	$(WEST_PATH) build -b nrf52840dk_nrf52840
	
build_module:
	$(WEST_PATH) build -b acn52840

build_custom:
	$(WEST_PATH) build -b acn52840 -- -DDTC_OVERLAY_FILE="overlays/acn52840.overlay overlays/custom_acn52840.overlay"

flash_dk:
	$(WEST_PATH) -v flash -r nrfjprog --snr $(SN_DK)

flash_acn:
	$(WEST_PATH) -v flash -r nrfjprog --snr $(SN_ACN)

build:
	$(WEST_PATH) build -b nrf52840dk_nrf52840

flash:
	$(WEST_PATH) -v flash -r nrfjprog --snr $(SN)"

clean:
	rm -rf build