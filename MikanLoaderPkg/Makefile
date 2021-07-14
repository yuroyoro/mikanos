SHELL=/bin/bash

EDK2_DIR   = $(HOME)/edk2
BUILD_DIR  = $(EDK2_DIR)/Build/
LOADER     = $(BUILD_DIR)/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi

.PHONY: all
all: $(LOADER)

$(LOADER): loader

.PHONY: loader
loader: Main.c
	@cd $(EDK2_DIR) && source ./edksetup.sh && build

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)/*