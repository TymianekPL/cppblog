cmake_minimum_required(VERSION 3.18)

set(AVR_MCU "atmega328p" CACHE STRING "AVR Microcontroller model")
set(AVR_PROGRAMMER "arduino" CACHE STRING "AVRDUDE programmer type")
set(AVR_PORT "COM3" CACHE STRING "Serial port for upload")
set(AVR_BAUD "115200" CACHE STRING "Baud rate for upload")
set(AVR_SERIAL_BAUD "9600" CACHE STRING "Baud rate for serial")
set(AVR_F_CPU "16000000L" CACHE STRING "Clock speed of the MCU")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

project(HiArduino)

add_compile_options("-mmcu=${AVR_MCU}")
add_link_options("-mmcu=${AVR_MCU}")

add_compile_options(
    -Wall
    -Wextra
    -pedantic-errors
    -Werror
    $<$<CONFIG:Debug>:-Og>
    $<$<CONFIG:RelWithDebInfo>:-Oz>

    $<$<CONFIG:Release>:-Oz>
    $<$<CONFIG:Release>:-flto>
    $<$<CONFIG:Release>:-fno-exceptions>
    $<$<CONFIG:Release>:-fno-rtti>
    $<$<CONFIG:Release>:-fno-threadsafe-statics>
)

add_compile_options("-ffunction-sections" "-fdata-sections")
add_link_options(
    -Wl,--gc-sections
    $<$<CONFIG:Release>:-flto>
)

add_executable(${PROJECT_NAME} src/main.cpp)
# add_executable(${PROJECT_NAME} src/main.cpp src/sleep.cpp)
# target_sources(${PROJECT_NAME} PRIVATE FILE_SET CXX_MODULES FILES src/hw.ixx src/sleep.ixx)

target_compile_definitions(${PROJECT_NAME} PRIVATE F_CPU=${AVR_F_CPU}
                           SERIAL_BAUD=${AVR_SERIAL_BAUD})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND avr-objcopy -O ihex -R .eeprom ${PROJECT_NAME} ${PROJECT_NAME}.hex
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    BYPRODUCTS ${PROJECT_NAME}.hex
    COMMENT "Generating ${PROJECT_NAME}.hex for ${AVR_MCU}"
)

add_custom_target(upload COMMAND avrdude -c ${AVR_PROGRAMMER} -p ${AVR_MCU} -P
                  ${AVR_PORT} -b ${AVR_BAUD} -U flash:w:${PROJECT_NAME}.hex:i
                  DEPENDS ${PROJECT_NAME} WORKING_DIRECTORY
                  ${CMAKE_CURRENT_BINARY_DIR} COMMENT
                  "Uploading to ${AVR_MCU} on ${AVR_PORT}...")

add_custom_target(serial COMMAND pwsh-preview -NoExit -File
                  "${CMAKE_SOURCE_DIR}/SerialMonitor.ps1" -Port ${AVR_PORT}
                  -Baud ${AVR_SERIAL_BAUD} COMMENT
                  "Opening interactive serial monitor on ${AVR_PORT}...")
