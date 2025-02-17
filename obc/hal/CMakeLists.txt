cmake_minimum_required(VERSION 3.15)

set(RM46_LAUNCHPAD_HAL_LIB rm46-launchpad-hal)
set(OBC_REV1_HAL_LIB obc-rev1-hal)
set(OBC_REV2_HAL_LIB obc-rev2-hal)

set(RM46_LAUNCHPAD_HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/common/os_required_fns.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/can.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/dabort.s
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/errata_SSWF021_45.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/esm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/gio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/het.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/i2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/notification.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/os_croutine.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/os_event_groups.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/os_heap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/os_list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/os_port.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/os_portasm.s
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/os_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/os_tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/os_timer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/pinmux.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/sci.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/sys_core.s
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/sys_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/sys_intvecs.s
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/sys_mpu.s
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/sys_pcr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/sys_phantom.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/sys_pmm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/sys_pmu.s
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/sys_vim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/system.c
)

set(RM46_LAUNCHPAD_NO_OPTIMIZE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/sys_selftest.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/sys_startup.c
    ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/source/os_mpu_wrappers.c
)

set(OBC_REV1_HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/common/os_required_fns.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/can.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/dabort.s
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/errata_SSWF021_45.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/esm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/gio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/het.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/i2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/notification.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/os_croutine.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/os_event_groups.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/os_heap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/os_list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/os_port.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/os_portasm.s
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/os_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/os_tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/os_timer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/pinmux.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/sci.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/sys_core.s
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/sys_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/sys_intvecs.s
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/sys_mpu.s
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/sys_pcr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/sys_phantom.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/sys_pmm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/sys_pmu.s
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/sys_vim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/system.c
)

set(OBC_REV1_NO_OPTIMIZE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/sys_selftest.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/sys_startup.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/source/os_mpu_wrappers.c
)

set(OBC_REV2_HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/common/os_required_fns.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/can.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/dabort.s
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/errata_SSWF021_45.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/esm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/gio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/het.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/i2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/notification.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/os_croutine.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/os_event_groups.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/os_heap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/os_list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/os_port.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/os_portasm.s
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/os_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/os_tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/os_timer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/pinmux.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/sci.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/sys_core.s
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/sys_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/sys_intvecs.s
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/sys_mpu.s
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/sys_pcr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/sys_phantom.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/sys_pmm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/sys_pmu.s
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/sys_vim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/system.c
)

set(OBC_REV2_NO_OPTIMIZE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/sys_selftest.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/sys_startup.c
    ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/source/os_mpu_wrappers.c
)

if (${BOARD_TYPE} MATCHES RM46_LAUNCHPAD)
    set(HAL_LIB rm46-launchpad-hal)
    set(HAL_SOURCES ${RM46_LAUNCHPAD_HAL_SOURCES})
    set(HAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/launchpad/include)

    set(NO_OPTIMIZE_SOURCES ${RM46_LAUNCHPAD_NO_OPTIMIZE_SOURCES})
elseif(${BOARD_TYPE} MATCHES OBC_REVISION_1)
    set(HAL_LIB obc-rev1-hal)
    set(HAL_SOURCES ${OBC_REV1_HAL_SOURCES})
    set(HAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev1/include)

    set(NO_OPTIMIZE_SOURCES ${OBC_REV1_NO_OPTIMIZE_SOURCES})
elseif(${BOARD_TYPE} MATCHES OBC_REVISION_2)
    set(HAL_LIB obc-rev2-hal)
    set(HAL_SOURCES ${OBC_REV2_HAL_SOURCES})
    set(HAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/obc_rev2/include)

    set(NO_OPTIMIZE_SOURCES ${OBC_REV2_NO_OPTIMIZE_SOURCES})
else()
    message(FATAL_ERROR "Invalid board type: ${BOARD_TYPE}")
endif()

target_sources(OBC-firmware.out PRIVATE ${HAL_SOURCES})
target_include_directories(OBC-firmware.out PUBLIC ${HAL_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/common)

# Higher optimization levels on some hal files will break firmware for some reason
# Compile them with no optimizations
target_sources(NO_OPTIMIZE_FILES PUBLIC ${NO_OPTIMIZE_SOURCES})
target_include_directories(NO_OPTIMIZE_FILES  PUBLIC ${HAL_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/common)

# TODO: Create a static library for the HAL and link it to the main firmware
