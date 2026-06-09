cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME Hemoco_CEC_V1)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM, C and C++ languages
enable_language(C CXX ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Application, TouchGFX and generated UI sources that are not emitted by the
# CubeMX CMake exporter. Keep these globs scoped to firmware/UI directories so
# simulator/build artifacts are not pulled into the embedded target.
file(GLOB CORE_APP_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/*.c
)
list(REMOVE_ITEM CORE_APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/system_stm32h7xx.c
)

file(GLOB TOUCHGFX_TARGET_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/target/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/target/generated/*.cpp
)

file(GLOB_RECURSE TOUCHGFX_GUI_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/gui/src/*.cpp
)

file(GLOB_RECURSE TOUCHGFX_GENERATED_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/generated/fonts/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/generated/gui_generated/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/generated/images/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/generated/texts/src/*.cpp
)

set(TOUCHGFX_CORE_LIB
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/touchgfx/lib/core/cortex_m7/gcc/libtouchgfx-float-abi-hard.a
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CORE_APP_SOURCES}
    ${TOUCHGFX_TARGET_SOURCES}
    ${TOUCHGFX_GUI_SOURCES}
    ${TOUCHGFX_GENERATED_SOURCES}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/App
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/target/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/target
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/touchgfx/framework/include
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/generated/fonts/include
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/generated/gui_generated/include
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/generated/images/include
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/generated/texts/include
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/generated/videos/include
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchGFX/gui/include
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/Target
    ${CMAKE_CURRENT_SOURCE_DIR}/FATFS/Target
    ${CMAKE_CURRENT_SOURCE_DIR}/FATFS/App
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/FatFs/src
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    -u _printf_float
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    ${TOUCHGFX_CORE_LIB}

    # Add user defined libraries
)
