MODULE_NAME := tsx73a-ec
DKMS_CONF_FILE := dkms.conf
MODULE_VERSION := $(shell grep "^PACKAGE_VERSION" $(DKMS_CONF_FILE) | sed 's/.*= *//')

.PHONY: all install uninstall

all:
	@echo "Available targets:"
	@echo "  install   - Install the DKMS module"
	@echo "  uninstall - Uninstall the DKMS module"

install:
	dkms install .

uninstall:
	dkms remove -m $(MODULE_NAME) -v $(MODULE_VERSION) --all