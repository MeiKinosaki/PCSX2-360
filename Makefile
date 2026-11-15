CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
BUILD_DIR := build
XENON_CC ?= xenon-gcc
ELF2XEX ?= elf2xex
XENON_CFLAGS ?= -O2 -Wall -Wextra

EE_SOURCES := src/ps2/dma.c src/ps2/ee_bus.c src/ps2/ee_state.c src/ps2/ee_ops.c src/ps2/ee_immediate.c \
              src/ps2/ee_compare.c src/ps2/ee_decode.c src/ps2/ee_memory.c \
              src/ps2/ee_state_io.c
HOST_TEST := $(BUILD_DIR)/test_ee_state
LIBRARY_TEST := $(BUILD_DIR)/test_game_library
DMA_TEST := $(BUILD_DIR)/test_dma
EE_BUS_TEST := $(BUILD_DIR)/test_ee_bus

.PHONY: all check validate test-host toolchain-check xex clean
all: check

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(HOST_TEST): $(EE_SOURCES) tests/test_ee_state.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/ps2 $(EE_SOURCES) tests/test_ee_state.c -o $@

$(DMA_TEST): src/ps2/dma.c src/ps2/dma.h tests/test_dma.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/ps2 src/ps2/dma.c tests/test_dma.c -o $@

$(EE_BUS_TEST): src/ps2/ee_bus.c src/ps2/ee_bus.h tests/test_ee_bus.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/ps2 src/ps2/ee_bus.c tests/test_ee_bus.c -o $@

$(LIBRARY_TEST): src/frontend/game_library.c src/frontend/game_library.h tests/test_game_library.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/frontend src/frontend/game_library.c tests/test_game_library.c -o $@

validate:
	@test -s src/main.c
	@test -s tests/test_ee_state.c
	@test -s src/ps2/dma.c
	@test -s src/ps2/dma.h
	@test -s tests/test_dma.c
	@test -s src/ps2/ee_bus.c
	@test -s src/ps2/ee_bus.h
	@test -s tests/test_ee_bus.c
	@test -s src/frontend/game_library.c
	@test -s src/frontend/game_library.h
	@test -s tests/test_game_library.c
	@for f in $(EE_SOURCES); do test -s $$f || exit 2; done
	@echo "OK: fontes-base do EE e biblioteca encontradas."

test-host: $(HOST_TEST) $(LIBRARY_TEST) $(DMA_TEST) $(EE_BUS_TEST)
	./$(HOST_TEST)
	./$(LIBRARY_TEST)
	./$(DMA_TEST)
	./$(EE_BUS_TEST)

check: validate test-host

toolchain-check:
	@command -v $(XENON_CC) >/dev/null 2>&1 || { echo "ERRO: $(XENON_CC) ausente."; exit 2; }
	@command -v $(ELF2XEX) >/dev/null 2>&1 || { echo "ERRO: $(ELF2XEX) ausente."; exit 2; }
	@echo "OK: ferramentas Xenon encontradas."

# ELF é somente intermediário temporário. O único executável entregue é default.xex.
$(BUILD_DIR)/default.xex: src/main.c $(EE_SOURCES) | $(BUILD_DIR)
	$(MAKE) toolchain-check
	@rm -f $(BUILD_DIR)/pcsx2-360.elf.tmp $@.tmp
	$(XENON_CC) $(XENON_CFLAGS) -Isrc/ps2 src/main.c $(EE_SOURCES) -o $(BUILD_DIR)/pcsx2-360.elf.tmp
	@test -s $(BUILD_DIR)/pcsx2-360.elf.tmp
	$(ELF2XEX) $(BUILD_DIR)/pcsx2-360.elf.tmp $@.tmp
	@test -s $@.tmp
	@mv $@.tmp $@
	@rm -f $(BUILD_DIR)/pcsx2-360.elf.tmp
	@test -s $@
	@echo "OK: build/default.xex gerado."

xex: $(BUILD_DIR)/default.xex

clean:
	rm -rf $(BUILD_DIR)
