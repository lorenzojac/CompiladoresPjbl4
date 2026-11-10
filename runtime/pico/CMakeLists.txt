# Raspberry Pi Pico 2 W (D48): liga o programa do compilador a biblioteca
# de execucao e ao Pico SDK, gerando programa.elf e programa.uf2.
#
# ZL_PROGRAMA_OBJ e o objeto ARM do programa, gerado pelo llc a partir do
# LLVM IR (scripts/construir.sh). O CMake e configurado uma vez por pasta de
# construcao; para outro programa basta trocar o objeto e religar.
#
#   cmake -S runtime/pico -B build/pico -G Ninja -DPICO_SDK_PATH=... \
#         -DZL_PROGRAMA_OBJ=build/pico/programa.o
#   cmake --build build/pico
cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2_w CACHE STRING "Placa do Pico SDK")
set(PICO_PLATFORM rp2350-arm-s CACHE STRING "RP2350 com nucleos ARM")

include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)

project(macaronica_pico C CXX ASM)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

set(ZL_PROGRAMA_OBJ "" CACHE FILEPATH "Objeto ARM do programa (llc)")
option(ZL_SENTINELA "Marca o fim do programa com EOT (testes)" OFF)

if(NOT ZL_PROGRAMA_OBJ)
    message(FATAL_ERROR "defina -DZL_PROGRAMA_OBJ=caminho/programa.o")
endif()

set(ZL_RUNTIME ${CMAKE_CURRENT_LIST_DIR}/..)

add_executable(programa ${ZL_RUNTIME}/zl_runtime_pico.c ${ZL_PROGRAMA_OBJ})
set_source_files_properties(${ZL_PROGRAMA_OBJ}
    PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)

target_include_directories(programa PRIVATE ${ZL_RUNTIME})
target_compile_options(programa PRIVATE -Wall -Wextra)
# A newlib pre-compilada nao marca a pilha como nao executavel (aviso inocuo).
target_link_options(programa PRIVATE -Wl,--no-warn-execstack)
if(ZL_SENTINELA)
    target_compile_definitions(programa PRIVATE ZL_SENTINELA)
endif()

# stdio pela USB (porta COM virtual), sem UART; LED pelo chip CYW43439.
target_link_libraries(programa
    pico_stdlib
    pico_cyw43_arch_none
    hardware_watchdog
)
pico_enable_stdio_usb(programa 1)
pico_enable_stdio_uart(programa 0)

# Windows: o GNU ld 2.47 do MSYS2 recusa mais de um arquivo de script de
# linker (inclusive INCLUDE), e o SDK usa ~20. Geramos um script unico com o
# mesmo texto e tiramos o --script=default_locations.ld do SDK (D48).
option(ZL_LINKER_UNICO "Script de linker em um arquivo so" ${CMAKE_HOST_WIN32})
if(ZL_LINKER_UNICO)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(ZL_LD ${CMAKE_BINARY_DIR}/memmap_macaronica.ld)
    set(ZL_LD_PASTAS "")
    foreach(PASTA IN LISTS PICO_LINKER_SCRIPT_INCLUDE_DIRS)
        list(APPEND ZL_LD_PASTAS -L ${PASTA})
    endforeach()
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/achata_ld.py
            ${ZL_LD} ${PICO_LINKER_SCRIPT_PATH}/memmap_default.ld
            --antes ${PICO_LINKER_DEFAULT_LOCATIONS_PATH} ${ZL_LD_PASTAS}
        RESULT_VARIABLE ZL_LD_RESULTADO
    )
    if(NOT ZL_LD_RESULTADO EQUAL 0)
        message(FATAL_ERROR "achata_ld.py falhou")
    endif()
    pico_set_linker_script(programa ${ZL_LD})
    get_target_property(ZL_OPCOES pico_standard_link INTERFACE_LINK_OPTIONS)
    list(FILTER ZL_OPCOES EXCLUDE REGEX "--script=")
    set_target_properties(pico_standard_link
        PROPERTIES INTERFACE_LINK_OPTIONS "${ZL_OPCOES}")
endif()

pico_set_program_name(programa "macaronica")
pico_set_program_description(programa "Programa Macaronica (PjBL 4)")

# programa.uf2: arrastar para a unidade RP2350 (modo BOOTSEL).
pico_add_uf2_output(programa)
