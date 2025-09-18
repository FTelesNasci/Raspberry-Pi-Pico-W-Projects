# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(MPU6050_Servo C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(MPU6050_Servo 
            MPU6050_Servo.c 
            lib/servo/servo_sim.c
            lib/flash/flash_storage.c
            lib/ssd1306/ssd1306.c
            lib/mpu6050/mpu6050_i2c.c
            )

pico_set_program_name(MPU6050_Servo "MPU6050_Servo")
pico_set_program_version(MPU6050_Servo "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(MPU6050_Servo 0)
pico_enable_stdio_usb(MPU6050_Servo 1)

# Add the standard library to the build
target_link_libraries(MPU6050_Servo
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(MPU6050_Servo PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lib
        ${CMAKE_CURRENT_LIST_DIR}/lib/servo
        ${CMAKE_CURRENT_LIST_DIR}/lib/flash
        ${CMAKE_CURRENT_LIST_DIR}/lib/ssd1306
        ${CMAKE_CURRENT_LIST_DIR}/lib/mpu6050
)

# Add any user requested libraries
target_link_libraries(MPU6050_Servo 
        hardware_pwm
        hardware_i2c
        hardware_flash
        hardware_sync
        )

pico_add_extra_outputs(MPU6050_Servo)

