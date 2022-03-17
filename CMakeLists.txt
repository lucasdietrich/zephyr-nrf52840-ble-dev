cmake_minimum_required(VERSION 3.13.1)

set(BOARD nrf52840dk_nrf52840)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(nrf52840-ble-getting-started)

FILE(GLOB app_sources src/*.c*)
target_sources(app PRIVATE ${app_sources})
