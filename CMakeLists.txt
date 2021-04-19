cmake_minimum_required(VERSION 3.10)

# STM32WLE, STM32WLEx, STM32WLE5x, STM32WLE5CCUX

project(pendant2021-stm32wl C CXX ASM)

set(CC_FLAGS
    -mcpu=cortex-m4 
    -Wnull-dereference
    -mthumb
    -mfloat-abi=soft
    -fdata-sections
    -ffunction-sections
    -flto
    -Wall
    -Wno-strict-aliasing 
    -Wno-format
    -ffast-math)

set(CXX_FLAGS 
    -Wall 
    -Wextra
    -Wdouble-promotion
    -Wshadow 
    -Wfloat-conversion 
    -Wno-volatile
    -Wformat=2 
    -std=c++20 
    -fno-rtti 
    -fno-exceptions)

set(LD_FLAGS
    -T${CMAKE_SOURCE_DIR}/STM32CubeIDE/STM32WLE5CCUX_FLASH.ld
    -flto
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=soft
    -specs=nano.specs
    -u _printf_float
    -lc
    -lm
    -lnosys
    -Wl,-Map=${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections)

set(DEBUG_FLAGS
    -Og)

set(RELEASE_FLAGS   
    -Os)

set(DEFINITIONS
    USER_APP_BUILD
    USE_HAL_DRIVER 
    CORE_CM4 
    STM32WLE5xx)

set(BASE_ADDRESS 0x08000000)

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)
set(DUMP_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.S)

include_directories(.)
include_directories(STM32CubeIDE/Core/Inc)
include_directories(STM32CubeIDE/Drivers/CMSIS/Include)
include_directories(STM32CubeIDE/Drivers/CMSIS/Device/ST/STM32WLxx/Include)
include_directories(STM32CubeIDE/Drivers/STM32WLxx_HAL_Driver/Inc)
include_directories(STM32CubeIDE/LoRaWAN/App)
include_directories(STM32CubeIDE/LoRaWAN/Target)
include_directories(STM32CubeIDE/Utilities/sequencer)
include_directories(STM32CubeIDE/Utilities/lpm/tiny_lpm)
include_directories(STM32CubeIDE/Utilities/timer)
include_directories(STM32CubeIDE/Utilities/trace/adv_trace)
include_directories(STM32CubeIDE/Utilities/misc)
include_directories(STM32CubeIDE/Middlewares/Third_Party/SubGHz_Phy)
include_directories(STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/LmHandler)
include_directories(STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/LmHandler/Packages)
include_directories(STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/Crypto)
include_directories(STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/Utilities)
include_directories(STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/Mac)
include_directories(STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/Mac/Region)

file(GLOB_RECURSE STM32CUBEIDE_SRC ${PROJECT_SOURCE_DIR}/STM32CubeIDE/Core/Src/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${PROJECT_SOURCE_DIR}/STM32CubeIDE/LoRaWAN/Target/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${PROJECT_SOURCE_DIR}/STM32CubeIDE/Utilities/sequencer/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${PROJECT_SOURCE_DIR}/STM32CubeIDE/Utilities/lpm/tiny_lpm/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${PROJECT_SOURCE_DIR}/STM32CubeIDE/Utilities/timer/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${PROJECT_SOURCE_DIR}/STM32CubeIDE/Utilities/trace/adv_trace/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${PROJECT_SOURCE_DIR}/STM32CubeIDE/Utilities/misc/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${PROJECT_SOURCE_DIR}/STM32CubeIDE/Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${PROJECT_SOURCE_DIR}/STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/LmHandler/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${PROJECT_SOURCE_DIR}/STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/Crypto/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${PROJECT_SOURCE_DIR}/STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/Utilities/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${PROJECT_SOURCE_DIR}/STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/Mac/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${PROJECT_SOURCE_DIR}/STM32CubeIDE/Middlewares/Third_Party/LoRaWAN/Mac/Region/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${PROJECT_SOURCE_DIR}/STM32CubeIDE/Drivers/STM32WLxx_HAL_Driver/Src/*.c)
file(GLOB_RECURSE STM32CUBEIDE_SRC ${PROJECT_SOURCE_DIR}/STM32CubeIDE/*.c)

add_executable(${PROJECT_NAME}.elf 
    ${PROJECT_SOURCE_DIR}/user_app.cpp 
    ${PROJECT_SOURCE_DIR}/i2c.cpp 
    ${PROJECT_SOURCE_DIR}/murmurhash3.cpp 
    ${PROJECT_SOURCE_DIR}/STM32CubeIDE/Core/Startup/startup_stm32wle5ccux.s 
    ${STM32CUBEIDE_SRC})

# Generate version.h
find_package(Git)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} OUTPUT_VARIABLE GIT_SHORT_SHA OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-list HEAD --count WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} OUTPUT_VARIABLE GIT_REV_COUNT OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT_EXECUTABLE} show -s --format=%ad HEAD --date=iso-strict WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} OUTPUT_VARIABLE GIT_COMMIT_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    set(GIT_SHORT_SHA "unknown")
    set(GIT_REV_COUNT "unknown")
    set(GIT_COMMIT_DATE "unknown")
endif()

include_directories(${CMAKE_BINARY_DIR})
configure_file("${PROJECT_SOURCE_DIR}/version.h.in" "${CMAKE_BINARY_DIR}/version.h" @ONLY)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "Compiler must be GCC.")
endif(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

if(CMAKE_CXX_COMPILER_VERSION LESS "10.2")
    message(FATAL_ERROR "GCC must be version 10.2 or higher.")
endif(CMAKE_CXX_COMPILER_VERSION LESS "10.2")

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE ${DEFINITIONS})
target_compile_options(${PROJECT_NAME}.elf PRIVATE ${CC_FLAGS})
target_link_options(${PROJECT_NAME}.elf PRIVATE ${LD_FLAGS})
target_compile_options(${PROJECT_NAME}.elf PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CXX_FLAGS}>")
target_compile_options(${PROJECT_NAME}.elf PRIVATE "$<$<CONFIG:DEBUG>:${DEBUG_FLAGS}>")
target_compile_options(${PROJECT_NAME}.elf PRIVATE "$<$<CONFIG:RELEASE>:${RELEASE_FLAGS}>")

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -D $<TARGET_FILE:${PROJECT_NAME}.elf> > ${DUMP_FILE}
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
    COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom -R .fuse -R .lock -R .signature $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
    COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")
    
set(PROGRAM_CMD "openocd -f interface/stlink.cfg -f target/stm32wlx.cfg -c \"program ${PROJECT_NAME}.bin ${BASE_ADDRESS} verify reset exit\"")
install(CODE "execute_process(COMMAND ${PROGRAM_CMD})")
