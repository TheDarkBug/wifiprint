cmake_minimum_required(VERSION 3.13)

execute_process(COMMAND "pwd" OUTPUT_VARIABLE CURRENT_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_PLATFORM rp2040)
set(PICO_BOARD pico_w)
if (DEFINED ENV{PICO_SDK_PATH})
	set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
else()
	set(PICO_SDK_PATH "/usr/share/pico-sdk")
endif()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include(${PICO_SDK_PATH}/lib/tinyusb/hw/bsp/rp2040/pico_sdk_import.cmake)
project(wifiprint C CXX ASM)

pico_sdk_init()

add_executable(wifiprint main.c external/dhcpserver.c external/dnsserver.c)

set(FRONTEND_DIR generated)
file(MAKE_DIRECTORY ${FRONTEND_DIR})
execute_process(COMMAND bash "-c" "xxd -i -C frontend/index.html | sed 's/};/,0};/g;s/unsigned int .*//g' > ${FRONTEND_DIR}/index.html.h")
execute_process(COMMAND bash "-c" "xxd -i -C frontend/main.js | sed 's/};/,0};/g;s/unsigned int .*//g' > ${FRONTEND_DIR}/main.js.h")
execute_process(COMMAND bash "-c" "xxd -i -C frontend/pdf.js | sed 's/};/,0};/g;s/unsigned int .*//g' > ${FRONTEND_DIR}/pdf.js.h")
execute_process(COMMAND bash "-c" "xxd -i -C frontend/style.css | sed 's/};/,0};/g;s/unsigned int .*//g' > ${FRONTEND_DIR}/style.css.h")


target_include_directories(wifiprint PRIVATE ${CMAKE_CURRENT_LIST_DIR})

pico_enable_stdio_usb(wifiprint 1)
pico_enable_stdio_uart(wifiprint 0)

target_link_libraries(wifiprint
	pico_cyw43_arch_lwip_threadsafe_background
	pico_stdlib
)

pico_add_extra_outputs(wifiprint)
