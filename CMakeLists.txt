cmake_minimum_required(VERSION 3.13)

execute_process(COMMAND "pwd" OUTPUT_VARIABLE CURRENT_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_PLATFORM rp2040)
set(PICO_BOARD pico_w)
set(PICO_SDK_PATH "/usr/share/pico-sdk")
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include(${PICO_SDK_PATH}/lib/tinyusb/hw/bsp/rp2040/pico_sdk_import.cmake)
project(tabprint C CXX ASM)

pico_sdk_init()

add_executable(tabprint main.c external/dhcpserver.c external/dnsserver.c)

set(FRONTEND_DIR generated)
file(MAKE_DIRECTORY ${FRONTEND_DIR})
execute_process(COMMAND bash "-c" "xxd -i -C frontend/index.html | sed 's/};/,0};/g;s/unsigned int .*//g' > ${FRONTEND_DIR}/index.html.h")
execute_process(COMMAND bash "-c" "xxd -i -C frontend/main.js | sed 's/};/,0};/g;s/unsigned int .*//g' > ${FRONTEND_DIR}/main.js.h")
execute_process(COMMAND bash "-c" "xxd -i -C frontend/pdf.js | sed 's/};/,0};/g;s/unsigned int .*//g' > ${FRONTEND_DIR}/pdf.js.h")
execute_process(COMMAND bash "-c" "xxd -i -C frontend/style.css | sed 's/};/,0};/g;s/unsigned int .*//g' > ${FRONTEND_DIR}/style.css.h")


target_include_directories(tabprint PRIVATE ${CMAKE_CURRENT_LIST_DIR})

pico_enable_stdio_usb(tabprint 1)
pico_enable_stdio_uart(tabprint 0)

target_link_libraries(tabprint
	pico_cyw43_arch_lwip_threadsafe_background
	pico_stdlib
)

pico_add_extra_outputs(tabprint)
