cmake_minimum_required(VERSION 3.13.1)

if(NOT DEFINED BOARD)
    set(BOARD "nrf52840dk_nrf52840")
endif()

# CONF_FILE

if(NOT DEFINED DTC_OVERLAY_FILE)
    set(DTC_OVERLAY_FILE "overlays/nrf52840dk_nrf52840.overlay")
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(nrf52840-ble-getting-started)

target_sources(app PRIVATE src/main.c)
