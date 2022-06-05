cmake_minimum_required(VERSION 3.13.1)

set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/.)

if(NOT DEFINED BOARD)
    set(BOARD "nrf52840dk_nrf52840")
endif()

if(NOT DEFINED DTC_OVERLAY_FILE)
    if("${BOARD}" STREQUAL "nrf52840dk_nrf52840")
        set(DTC_OVERLAY_FILE "overlays/nrf52840dk_nrf52840.overlay")
    elseif("${BOARD}" STREQUAL "acn52840")
        set(DTC_OVERLAY_FILE "overlays/acn52840.overlay")
    endif()
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(nrf52840-ble-getting-started)

FILE(GLOB_RECURSE app_sources src/*.c*)
target_sources(app PRIVATE ${app_sources} src/main.c)
