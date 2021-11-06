# Copyright 2021 by Daniel Winkelman. All rights reserved.

cmake_minimum_required (VERSION 3.16)

set (CMAKE_BUILD_TYPE Debug)
set (CMAKE_CXX_STANDARD 17)

# Define targets
project (Clef C CXX)
set (Target_tests clef-tests)
set (Target_fw_atmega2560 clef-atmega2560)

include_directories (src)

file (GLOB FW_COMMON_IF_SOURCES src/if/*.cc src/fw/*.cc src/if/fw/kalman/*cc)

if (${CLEF_TARGET} MATCHES "atmega2560")
    # Build Atmega2560 firmware
    # https://github.com/mkleemann/cmake-avr/blob/master/generic-gcc-avr.cmake
    set (AVR_TOOLCHAIN_PATH /home/dwinkelman/Downloads/arduino-1.8.12/hardware/tools/avr/bin)
    set (AVRDUDE_CONFIG_PATH /home/dwinkelman/Downloads/arduino-1.8.12/hardware/tools/avr/etc/avrdude.conf)
    find_program (AVR_CC avr-gcc REQUIRED PATHS ${AVR_TOOLCHAIN_PATH})
    find_program (AVR_CXX avr-g++ REQUIRED PATHS ${AVR_TOOLCHAIN_PATH})
    find_program (AVR_OBJCOPY avr-objcopy REQUIRED PATHS ${AVR_TOOLCHAIN_PATH})
    find_program (AVR_SIZE_TOOL avr-size REQUIRED PATHS ${AVR_TOOLCHAIN_PATH})
    find_program (AVR_OBJDUMP avr-objdump REQUIRED PATHS ${AVR_TOOLCHAIN_PATH})
    find_program (AVRDUDE avrdude REQUIRED PATHS ${AVR_TOOLCHAIN_PATH})

    set (CMAKE_SYSTEM_NAME Generic)
    set (CMAKE_SYSTEM_PROCESSOR avr)
    set (CMAKE_C_COMPILER ${AVR_CC})
    set (CMAKE_CXX_COMPILER ${AVR_CXX})

    set (elf_file ${Target_fw_atmega2560}.elf)
    set (eep_file ${Target_fw_atmega2560}.eep)
    set (hex_file ${Target_fw_atmega2560}.hex)

    file (
        GLOB FW_ATMEGA2560_SOURCES
        src/main.atmega2560.cc
        src/impl/atmega2560/*.cc
        src/impl/atmega2560/usart/*.c
    )
    add_executable (${elf_file} ${FW_ATMEGA2560_SOURCES} ${FW_COMMON_IF_SOURCES})
    set_target_properties (
        ${elf_file}
        PROPERTIES
            COMPILE_FLAGS "-mmcu=atmega2560 -Os -flto -D F_CPU=16000000L -Wall -D TARGET_AVR"
            LINK_FLAGS "-mmcu=atmega2560"
    )
    add_custom_command (
        OUTPUT ${eep_file}
        COMMAND
            ${AVR_OBJCOPY}
            -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load
            --no-change-warnings --change-section-lma .eeprom=0
            ${elf_file}
            ${eep_file}
        DEPENDS ${elf_file}
    )
    add_custom_command (
        OUTPUT ${hex_file}
        COMMAND
            ${AVR_OBJCOPY}
            -O ihex -R .eeprom
            ${elf_file}
            ${hex_file}
        DEPENDS ${elf_file}
    )
    add_custom_target (
        ${Target_fw_atmega2560}
        ALL
        DEPENDS
            ${elf_file}
            ${eep_file}
            ${hex_file}
    )
    add_custom_target (
        ${Target_fw_atmega2560}-size
        ALL ${AVR_SIZE_TOOL} -C --mcu=atmega2560 ${elf_file}
        DEPENDS ${elf_file}
    )
    if (DEFINED CLEF_PORT AND DEFINED CLEF_BAUD)
        add_custom_target(
            ${Target_fw_atmega2560}-upload-hex
            ALL
                ${AVRDUDE}
                -D
                -p atmega2560
                -c wiring
                -P ${CLEF_PORT}
                -b ${CLEF_BAUD}
                -U flash:w:${hex_file}:i
            DEPENDS ${Target_fw_atmega2560}
            COMMENT "Uploading ${hex_file}"
        )
        add_custom_target(
            ${Target_fw_atmega2560}-upload-eep
            ALL
                ${AVRDUDE}
                -D
                -p atmega2560
                -c wiring
                -P ${CLEF_PORT}
                -b ${CLEF_BAUD}
                -U eeprom:w:${eep_file}:i
            DEPENDS ${Target_fw_atmega2560} ${Target_fw_atmega2560}-upload-hex
            COMMENT "Uploading ${eep_file}"
        )
    endif ()

else ()
    # Install Google Test
    include (FetchContent)
    FetchContent_Declare (googletest URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip)
    FetchContent_MakeAvailable (googletest)
    enable_testing ()

    # Set up include directories
    include_directories (${googletest_SOURCE_DIR}/googletest/include)

    file (GLOB FW_EMULATOR_IF_SOURCES src/impl/emulator/*.cc)

    # Tests target
    file (GLOB TESTS_SOURCES tests/main.cc tests/util/*.cc tests/fw/*.cc tests/impl/emulator/*.cc)
    add_executable (${Target_tests} ${TESTS_SOURCES} ${FW_EMULATOR_IF_SOURCES} ${FW_COMMON_IF_SOURCES})
    target_link_libraries (${Target_tests} gtest_main)

    # Set up for Google Test
    include (GoogleTest)
    gtest_discover_tests (${Target_tests})

endif ()
