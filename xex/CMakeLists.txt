cmake_minimum_required(VERSION 3.25)
project(PCSX2_360 C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS ON)
set(PCSX2_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

add_executable(default
    "${PCSX2_ROOT}/src/main.c"
    "${PCSX2_ROOT}/src/ps2/dma.c"
    "${PCSX2_ROOT}/src/ps2/dma_irq.c"
    "${PCSX2_ROOT}/src/ps2/device_fifo.c"
    "${PCSX2_ROOT}/src/ps2/iop.c"
    "${PCSX2_ROOT}/src/ps2/scheduler.c"
    "${PCSX2_ROOT}/src/ps2/dma_chain.c"
    "${PCSX2_ROOT}/src/ps2/dma_endpoints.c"
    "${PCSX2_ROOT}/src/ps2/ee_bus.c"
    "${PCSX2_ROOT}/src/ps2/ee_state.c"
    "${PCSX2_ROOT}/src/ps2/ee_ops.c"
    "${PCSX2_ROOT}/src/ps2/ee_immediate.c"
    "${PCSX2_ROOT}/src/ps2/ee_compare.c"
    "${PCSX2_ROOT}/src/ps2/ee_decode.c"
    "${PCSX2_ROOT}/src/ps2/ee_memory.c"
    "${PCSX2_ROOT}/src/ps2/ee_state_io.c"
)

target_include_directories(default PRIVATE
    "${PCSX2_ROOT}/src/ps2"
    "${PCSX2_ROOT}/src/frontend"
)
target_compile_options(default PRIVATE -Wall -Wextra -Werror)
xsdk_add_xex(default TITLE_ID 0x50533236 NAME "PCSX2-360")
