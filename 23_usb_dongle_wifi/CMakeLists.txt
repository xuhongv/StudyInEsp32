# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

add_compile_options(-fdiagnostics-color=always)
add_definitions(-w)

set(EXTRA_COMPONENT_DIRS components/FreeRTOS-Plus-CLI
                         components/usb/tinyusb)
                            
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(aithinker_esp_12K_usb_dongle)
