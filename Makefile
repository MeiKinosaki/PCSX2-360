CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
BUILD_DIR := build
XSDK_DIR ?= ../xsdk
XSDK_TOOLCHAIN ?= $(HOME)/xsdk-gcc
XSDK_CC := $(XSDK_TOOLCHAIN)/bin/powerpc-xsdk-elf-gcc


EE_SOURCES := src/ps2/dma.c src/ps2/dma_irq.c src/ps2/device_fifo.c src/ps2/iop.c src/ps2/scheduler.c src/ps2/dma_chain.c src/ps2/dma_endpoints.c src/ps2/ee_bus.c src/ps2/ee_state.c src/ps2/ee_ops.c src/ps2/ee_immediate.c \
              src/ps2/ee_compare.c src/ps2/ee_decode.c src/ps2/ee_memory.c \
              src/ps2/ee_state_io.c
HOST_TEST := $(BUILD_DIR)/test_ee_state
LIBRARY_TEST := $(BUILD_DIR)/test_game_library
DMA_IRQ_TEST := $(BUILD_DIR)/test_dma_irq
DEVICE_FIFO_TEST := $(BUILD_DIR)/test_device_fifo
IOP_TEST := $(BUILD_DIR)/test_iop
SCHEDULER_TEST := $(BUILD_DIR)/test_scheduler
DMA_TEST := $(BUILD_DIR)/test_dma
EE_BUS_TEST := $(BUILD_DIR)/test_ee_bus
DMA_ENDPOINT_TEST := $(BUILD_DIR)/test_dma_endpoints
DMA_CHAIN_TEST := $(BUILD_DIR)/test_dma_chain
DMA_BUS_TEST := $(BUILD_DIR)/test_dma_bus

.PHONY: all check validate test-host toolchain-check xex clean
all: check

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(HOST_TEST): $(EE_SOURCES) tests/test_ee_state.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/ps2 $(EE_SOURCES) tests/test_ee_state.c -o $@

$(SCHEDULER_TEST): src/ps2/scheduler.c src/ps2/iop.c src/ps2/dma_irq.c src/ps2/dma.c src/ps2/dma_chain.c src/ps2/dma_endpoints.c src/ps2/ee_bus.c src/ps2/ee_state.c src/ps2/scheduler.h tests/test_scheduler.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/ps2 src/ps2/scheduler.c src/ps2/iop.c src/ps2/dma_irq.c src/ps2/dma.c src/ps2/dma_chain.c src/ps2/dma_endpoints.c src/ps2/ee_bus.c src/ps2/ee_state.c tests/test_scheduler.c -o $@

$(IOP_TEST): src/ps2/iop.c src/ps2/iop.h tests/test_iop.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/ps2 src/ps2/iop.c tests/test_iop.c -o $@

$(DEVICE_FIFO_TEST): src/ps2/device_fifo.c src/ps2/device_fifo.h tests/test_device_fifo.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/ps2 src/ps2/device_fifo.c tests/test_device_fifo.c -o $@

$(DMA_IRQ_TEST): src/ps2/dma_irq.c src/ps2/dma.c src/ps2/ee_bus.c src/ps2/ee_state.c src/ps2/dma.h src/ps2/ee_bus.h src/ps2/ee_state.h tests/test_dma_irq.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/ps2 src/ps2/dma_irq.c src/ps2/dma.c src/ps2/ee_bus.c src/ps2/ee_state.c tests/test_dma_irq.c -o $@

$(DMA_TEST): src/ps2/dma.c src/ps2/ee_bus.c src/ps2/dma.h src/ps2/ee_bus.h tests/test_dma.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/ps2 src/ps2/dma.c src/ps2/ee_bus.c tests/test_dma.c -o $@

$(DMA_BUS_TEST): src/ps2/dma.c src/ps2/dma_irq.c src/ps2/dma_chain.c src/ps2/dma_endpoints.c src/ps2/ee_bus.c src/ps2/ee_state.c src/ps2/dma.h src/ps2/dma_endpoints.h src/ps2/ee_bus.h src/ps2/ee_state.h tests/test_dma_bus.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/ps2 src/ps2/dma.c src/ps2/dma_irq.c src/ps2/dma_chain.c src/ps2/dma_endpoints.c src/ps2/ee_bus.c src/ps2/ee_state.c tests/test_dma_bus.c -o $@

$(DMA_CHAIN_TEST): src/ps2/dma_chain.c src/ps2/dma_chain.h src/ps2/ee_bus.c src/ps2/ee_bus.h tests/test_dma_chain.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/ps2 src/ps2/dma_chain.c src/ps2/ee_bus.c tests/test_dma_chain.c -o $@

$(DMA_ENDPOINT_TEST): src/ps2/dma.c src/ps2/dma_chain.c src/ps2/dma_endpoints.c src/ps2/ee_bus.c src/ps2/dma.h src/ps2/dma_endpoints.h src/ps2/ee_bus.h tests/test_dma_endpoints.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/ps2 src/ps2/dma.c src/ps2/dma_chain.c src/ps2/dma_endpoints.c src/ps2/ee_bus.c tests/test_dma_endpoints.c -o $@

$(EE_BUS_TEST): src/ps2/ee_bus.c src/ps2/ee_bus.h tests/test_ee_bus.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/ps2 src/ps2/ee_bus.c tests/test_ee_bus.c -o $@

$(LIBRARY_TEST): src/frontend/game_library.c src/frontend/game_library.h tests/test_game_library.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/frontend src/frontend/game_library.c tests/test_game_library.c -o $@

validate:
	@test -s src/main.c
	@test -s tests/test_ee_state.c
	@test -s src/ps2/iop.c
	@test -s src/ps2/iop.h
	@test -s tests/test_iop.c
	@test -s src/ps2/scheduler.c
	@test -s src/ps2/scheduler.h
	@test -s tests/test_scheduler.c
	@test -s src/ps2/device_fifo.c
	@test -s src/ps2/device_fifo.h
	@test -s tests/test_device_fifo.c
	@test -s src/ps2/dma_irq.c
	@test -s tests/test_dma_irq.c
	@test -s src/ps2/dma.c
	@test -s src/ps2/dma.h
	@test -s tests/test_dma.c
	@test -s src/ps2/dma_chain.c
	@test -s src/ps2/dma_chain.h
	@test -s tests/test_dma_chain.c
	@test -s tests/test_dma_bus.c
	@test -s src/ps2/dma_endpoints.c
	@test -s src/ps2/dma_endpoints.h
	@test -s tests/test_dma_endpoints.c
	@test -s src/ps2/ee_bus.c
	@test -s src/ps2/ee_bus.h
	@test -s tests/test_ee_bus.c
	@test -s src/frontend/game_library.c
	@test -s src/frontend/game_library.h
	@test -s tests/test_game_library.c
	@for f in $(EE_SOURCES); do test -s $$f || exit 2; done
	@echo "OK: fontes-base do EE e biblioteca encontradas."

test-host: $(HOST_TEST) $(LIBRARY_TEST) $(DMA_IRQ_TEST) $(DEVICE_FIFO_TEST) $(IOP_TEST) $(SCHEDULER_TEST) $(DMA_TEST) $(EE_BUS_TEST) $(DMA_ENDPOINT_TEST) $(DMA_CHAIN_TEST) $(DMA_BUS_TEST)
	./$(HOST_TEST)
	./$(LIBRARY_TEST)
	./$(DMA_IRQ_TEST)
	./$(DEVICE_FIFO_TEST)
	./$(IOP_TEST)
	./$(SCHEDULER_TEST)
	./$(DMA_TEST)
	./$(EE_BUS_TEST)
	./$(DMA_ENDPOINT_TEST)
	./$(DMA_CHAIN_TEST)
	./$(DMA_BUS_TEST)

check: validate test-host

toolchain-check:
	@test -x "$(XSDK_CC)" || { echo "ERRO: toolchain xsdk ausente em $(XSDK_TOOLCHAIN)."; exit 2; }
	@test -s "$(XSDK_DIR)/cmake/xsdk.cmake" || { echo "ERRO: xsdk não encontrado em $(XSDK_DIR)."; exit 2; }
	@test -s "$(XSDK_DIR)/tools/elf2xex.py" || { echo "ERRO: empacotador XEX do xsdk ausente."; exit 2; }
	@test -s "$(XSDK_DIR)/src/xsdk.ld" || { echo "ERRO: linker script do xsdk ausente."; exit 2; }
	@echo "OK: toolchain, runtime, linker e empacotador xsdk encontrados."

# Gera um título XEX para o kernel retail usado em consoles RGH/JTAG.
# O ELF é intermediário privado do build e é removido após o empacotamento.
xex: check toolchain-check
	@set -eu; \
	elf="$(BUILD_DIR)/xex/default"; \
	xex="$(BUILD_DIR)/xex/default.xex"; \
	trap 'rm -f "$elf"' EXIT HUP INT TERM; \
	rm -f "$(BUILD_DIR)/default.xex" "$elf" "$xex"; \
	$(MAKE) -C "$(XSDK_DIR)" TOOLCHAIN="$(XSDK_TOOLCHAIN)/bin/powerpc-xsdk-elf-" build/libxsdk.a; \
	cmake -S xex -B $(BUILD_DIR)/xex -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_TOOLCHAIN_FILE="$(XSDK_DIR)/cmake/xsdk.cmake" -DXSDK_TOOLCHAIN="$(XSDK_TOOLCHAIN)"; \
	cmake --build $(BUILD_DIR)/xex; \
	test -s "$xex"; \
	test "$(head -c 4 "$xex")" = "XEX2"; \
	cp "$xex" "$(BUILD_DIR)/default.xex"; \
	rm -f "$elf"; \
	trap - EXIT HUP INT TERM; \
	test -s "$(BUILD_DIR)/default.xex"; \
	echo "OK: build/default.xex gerado e cabeçalho XEX2 validado; ELF intermediário removido."

clean:
	rm -rf $(BUILD_DIR)
