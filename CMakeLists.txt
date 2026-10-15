# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

option(ADD_HPGL "Add HPGL plugin" OFF)
 # Note: WiFi and Ethernet cannot be enabled at the same time!
option(ADD_WIFI "Add WiFi networking" OFF)
option(ADD_ETHERNET "Add Ethernet networking" OFF)
option(ADD_BLUETOOTH "Add Bluetooth" OFF)
# Set LWIP_DIR below to the root path of lwIP sources when the following is set ON!
option(ADD_mDNS "Add mDNS service" OFF) 
option(ADD_MQTT "Add MQTT client API" OFF)
#
option(AddMyPlugin "Add my_plugin.c to build" OFF)

# Select the correct board in VSCode, the following line will be updated accordingly
set(PICO_BOARD pico CACHE STRING "Board type")

include(pico_sdk_import.cmake)

if(ADD_WIFI OR ADD_ETHERNET)
    include(networking/CMakeLists.txt)
    include(webui/CMakeLists.txt)
    if(ADD_mDNS OR ADD_MQTT)
        set(LWIP_DIR ${PICO_SDK_PATH}/lib/lwip)
        include(${LWIP_DIR}/src/Filelists.cmake)
    endif()
endif()

include(grbl/CMakeLists.txt)
include(fatfs/CMakeLists.txt)
include(motors/CMakeLists.txt)
include(trinamic/CMakeLists.txt)
include(spindle/CMakeLists.txt)
include(sdcard/CMakeLists.txt)
include(keypad/CMakeLists.txt)
include(eeprom/CMakeLists.txt)
include(fans/CMakeLists.txt)
include(bluetooth/CMakeLists.txt)
include(embroidery/CMakeLists.txt)
include(plasma/CMakeLists.txt)
include(laser/CMakeLists.txt)
include(plugins/CMakeLists.txt)
if(ADD_HPGL)
    include(hpgl/CMakeLists.txt)
endif()

project(grblHAL)
pico_sdk_init()

if(AddMyPlugin)
    add_executable(grblHAL
    main.c
    driver.c
    serial.c
    usb_serial.c
    stdio_usb_descriptors.c
    flash.c
    i2c.c
    spi.c
    thcad2.c
    sr16_out.c
    ioports.c
    ioports_analog.c
    tmc_uart.c
    my_plugin.c
    boards/pico_cnc.c
    boards/flexihal2350.c
    boards/btt_skr_pico_10.c
    littlefs/lfs.c
    littlefs/lfs_util.c
    littlefs_hal.c
    )
else()
    add_executable(grblHAL
    main.c
    driver.c
    serial.c
    usb_serial.c
    stdio_usb_descriptors.c
    flash.c
    i2c.c
    spi.c
    thcad2.c
    sr16_out.c
    ioports.c
    ioports_analog.c
    tmc_uart.c
    boards/pico_cnc.c
    boards/flexihal2350.c
    boards/btt_skr_pico_10.c
    littlefs/lfs.c
    littlefs/lfs_util.c
    littlefs_hal.c
    )
endif()

pico_generate_pio_header(grblHAL ${CMAKE_CURRENT_LIST_DIR}/driverPIO.pio)

if(PICO_BOARD STREQUAL "pico" OR PICO_BOARD STREQUAL "pico_w")
target_compile_definitions(grblHAL PUBLIC RP_MCU=2040)
if(PICO_SDK_VERSION_STRING VERSION_GREATER_EQUAL "2.1.1")
target_compile_definitions(grblHAL PUBLIC PICO_USE_FASTEST_SUPPORTED_CLOCK=1)
endif()
else()
target_compile_definitions(grblHAL PUBLIC RP_MCU=2350)
endif()
target_compile_definitions(grblHAL PUBLIC RP2040)
target_compile_definitions(grblHAL PUBLIC NEW_FATFS)
# grbl/*.c sources include grbl/config.h directly (not my_machine.h), so the
# #ifndef N_AXIS guard there would default them to 3 - producing [AXS:3:XYZ]
# while driver code sees N_AXIS=4. Mirror my_machine.h's value here so every
# translation unit agrees. Keep this in sync with my_machine.h:N_AXIS.
target_compile_definitions(grblHAL PUBLIC N_AXIS=4)
# HotWire4X STOP button on the safety-door input (GP22, board map): mirrored
# here for the same reason, so grbl/*.c and the driver both see the door.
# Keep this in sync with my_machine.h:SAFETY_DOOR_ENABLE.
target_compile_definitions(grblHAL PUBLIC SAFETY_DOOR_ENABLE=1)

# HotWire4X per-machine defaults (R. ILTIS).
# Same root cause as N_AXIS: grbl/settings.c reads DEFAULT_* via grbl/config.h,
# which never sees my_machine.h. Without these -D overrides the user gets the
# upstream defaults (250 steps/mm, 500 mm/min, 10 mm/s^2, 200mm travel, 0
# invert masks). TMC values (currents, microsteps) would work via my_machine.h
# alone since motors/trinamic.* sees it, but mirrored here for consistency.
target_compile_definitions(grblHAL PUBLIC
    DEFAULT_X_STEPS_PER_MM=160.0f
    DEFAULT_Y_STEPS_PER_MM=160.0f
    DEFAULT_Z_STEPS_PER_MM=160.0f
    DEFAULT_A_STEPS_PER_MM=160.0f
    DEFAULT_X_MAX_RATE=2400.0f
    DEFAULT_Y_MAX_RATE=2400.0f
    DEFAULT_Z_MAX_RATE=2400.0f
    DEFAULT_A_MAX_RATE=2400.0f
    DEFAULT_X_ACCELERATION=2000.0f
    DEFAULT_Y_ACCELERATION=2000.0f
    DEFAULT_Z_ACCELERATION=2000.0f
    DEFAULT_A_ACCELERATION=2000.0f
    DEFAULT_X_MAX_TRAVEL=850.0f
    DEFAULT_Y_MAX_TRAVEL=450.0f
    DEFAULT_Z_MAX_TRAVEL=850.0f
    DEFAULT_A_MAX_TRAVEL=450.0f
    DEFAULT_X_CURRENT=800.0f
    DEFAULT_Y_CURRENT=800.0f
    DEFAULT_Z_CURRENT=800.0f
    DEFAULT_A_CURRENT=800.0f
    DEFAULT_LIMIT_SIGNALS_INVERT_MASK=15
    DEFAULT_CONTROL_SIGNALS_INVERT_MASK=14
    DEFAULT_SPINDLE_PWM_FREQ=200.0f
    TRINAMIC_DEFAULT_MICROSTEPS=32
)

# HotWire4X heat-output variant selector (option A - two distributable firmwares).
#   SERVO (default): spindle PWM on GP29 (SERVO header) -> external opto-isolated
#                    MOSFET module. For wire supplies above 24V / larger tables.
#   HB             : spindle PWM on GP21 (on-board heated-bed MOSFET). For small
#                    tables (e.g. MiniCut2D-class) with a short wire powered at
#                    12-24V, no external module. Same PWM (freq/duty/overrides),
#                    different output pin. WARNING: no opto-isolation in this path.
# Build the HB variant with:  cmake -DHOTWIRE_HEAT=HB -B build_hb -S .
# The generated firmware is copied to build/grblHAL_<variant>.uf2 for download.
set(HOTWIRE_HEAT "SERVO" CACHE STRING "Hotwire heat output pin: SERVO (GP29, external MOSFET) or HB (GP21, on-board MOSFET)")
set_property(CACHE HOTWIRE_HEAT PROPERTY STRINGS SERVO HB)
if(HOTWIRE_HEAT STREQUAL "HB")
    target_compile_definitions(grblHAL PUBLIC HOTWIRE_HEAT_ON_HB=1)
    set(HOTWIRE_UF2_NAME "grblHAL_HB.uf2")
    message(STATUS "HotWire4X heat output: GP21 (on-board HB MOSFET, 12-24V variant)")
else()
    set(HOTWIRE_UF2_NAME "grblHAL_SERVOS.uf2")
    message(STATUS "HotWire4X heat output: GP29 (SERVOS header, external MOSFET variant)")
endif()

#target_compile_definitions(grblHAL PUBLIC LITTLEFS_ENABLE=1)

if(ADD_WIFI)
    target_compile_definitions(grblHAL PUBLIC WIFI_ENABLE=1)
    target_compile_definitions(grblHAL PUBLIC NDEBUG)
    target_sources(grblHAL PRIVATE
     wifi.c
    )
    target_link_libraries(grblHAL PRIVATE
     pico_cyw43_arch_lwip_poll
    )
endif()

if(ADD_ETHERNET)
    target_compile_definitions(grblHAL PUBLIC ETHERNET_ENABLE=1)
    target_compile_definitions(grblHAL PUBLIC CYW43_LWIP=0)
    target_compile_definitions(grblHAL PUBLIC LWIP_NOASSERT)
    include(networking/wiznet/CMakeLists.txt)
    target_sources(grblHAL PRIVATE
     w5x00_ll_driver.c
    )
    target_link_libraries(grblHAL PRIVATE
     wiznet
     pico_lwip
     pico_lwip_nosys
    )
endif()

if(ADD_WIFI OR ADD_ETHERNET)
    target_compile_definitions(grblHAL PUBLIC WEBUI_AUTO_REPORT_INTERVAL=0)
    if(ADD_mDNS)
        target_compile_definitions(grblHAL PUBLIC LWIP_IGMP=1)
        target_compile_definitions(grblHAL PUBLIC LWIP_NUM_NETIF_CLIENT_DATA=1)
        target_compile_definitions(grblHAL PUBLIC LWIP_MDNS_RESPONDER=1)
        target_compile_definitions(grblHAL PUBLIC MDNS_MAX_SERVICES=8)
        target_compile_definitions(grblHAL PUBLIC MDNS_ENABLE=1)
        target_sources(grblHAL PRIVATE
         ${lwipmdns_SRCS}
        )
    endif()
    if(ADD_MQTT)
        target_compile_definitions(grblHAL PUBLIC MQTT_ENABLE=1)
        target_sources(grblHAL PRIVATE
         ${lwipmqtt_SRCS}
        )
        target_link_libraries(grblHAL PRIVATE
            pico_lwip_mqtt
        )
    endif()
    target_link_libraries(grblHAL PRIVATE
     networking
     webui
    )
endif()

if(ADD_BLUETOOTH)
    target_compile_definitions(grblHAL PUBLIC BLUETOOTH_ENABLE=1)
    target_compile_definitions(grblHAL PUBLIC NDEBUG)
    target_sources(grblHAL PRIVATE
     bt_native.c
    )
    target_include_directories(grblHAL PRIVATE ${PICO_SDK_PATH}/lib/btstack/src)
    target_link_libraries(grblHAL PRIVATE
     pico_btstack_classic 
     pico_btstack_ble
     pico_btstack_cyw43
    )
    if(NOT ADD_WIFI)
        target_compile_definitions(grblHAL PUBLIC CYW43_LWIP=0)
        target_link_libraries(grblHAL PRIVATE
         pico_cyw43_arch_poll
        )
    endif()
endif()

if(AddMyPlugin)
    target_sources(grblHAL PRIVATE
     my_plugin.c
    )
endif()

if(ADD_HPGL)
    target_sources(grblHAL PRIVATE
     boards/citoh_cx6000.c
    )
    target_link_libraries(grblHAL PRIVATE
     hpgl
    )
endif()

target_include_directories(grblHAL PRIVATE ${CMAKE_CURRENT_LIST_DIR})
if(PICO_PLATFORM STREQUAL "rp2040" OR PICO_BOARD STREQUAL "pico" OR PICO_BOARD STREQUAL "pico_w")
target_compile_definitions(grblHAL PUBLIC RP_MCU=2040)
target_link_libraries(grblHAL PRIVATE
 hardware_rtc
)
else()
target_compile_definitions(grblHAL PUBLIC RP_MCU=2350)
endif()

target_link_libraries(grblHAL PRIVATE
 grbl
 fatfs
 sdcard
 motors
 trinamic
 spindle
 keypad
 fans
 laser
 bluetooth
 eeprom
 plasma
 misc_plugins
 embroidery
 tinyusb_device_unmarked
 pico_stdlib
 pico_unique_id
 cmsis_core
 hardware_dma
 hardware_uart
 hardware_pio
 hardware_i2c
 hardware_spi
 hardware_gpio
 hardware_pwm
 hardware_clocks
 hardware_flash
)

pico_add_extra_outputs(grblHAL)

# Copy the generated .uf2 to a variant-named file for distribution / download.
add_custom_command(TARGET grblHAL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE_DIR:grblHAL>/grblHAL.uf2
        $<TARGET_FILE_DIR:grblHAL>/${HOTWIRE_UF2_NAME}
    COMMENT "HotWire4X: copying firmware to ${HOTWIRE_UF2_NAME}"
    VERBATIM)

unset(PICO_BOARD CACHE)
unset(ADD_WIFI CACHE)
unset(ADD_ETHERNET CACHE)
unset(ADD_BLUETOOTH CACHE)
unset(ADD_HPGL CACHE)
unset(ADD_mDNS CACHE)
unset(AddMyPlugin CACHE)
