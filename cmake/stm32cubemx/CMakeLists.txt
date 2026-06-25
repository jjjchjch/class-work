cmake_minimum_required(VERSION 3.22)
# Enable CMake support for ASM and C languages
enable_language(C ASM)
# STM32CubeMX generated symbols (macros)
set(MX_Defines_Syms 
	USE_HAL_DRIVER 
	STM32F407xx
    $<$<CONFIG:Debug>:DEBUG>
    ARM_MATH_CM4
    ARM_DSP_CONFIG_TABLES
    ARM_FAST_ALLOW_TABLES
    ARM_TABLE_SIN_F32
    ARM_FFT_ALLOW_TABLES
    ARM_TABLE_TWIDDLECOEF_F32_128
    ARM_TABLE_BITREVIDX_FLT_128
    ARM_TABLE_TWIDDLECOEF_F32_256
    ARM_TABLE_BITREVIDX_FLT_256
    ARM_TABLE_TWIDDLECOEF_RFFT_F32_256
    ARM_TABLE_TWIDDLECOEF_F32_512
    ARM_TABLE_BITREVIDX_FLT_512
    ARM_TABLE_TWIDDLECOEF_RFFT_F32_1024
)

# STM32CubeMX generated include paths
set(MX_Include_Dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/LCD
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/PrivateInclude
)

# STM32CubeMX generated application sources
set(MX_Application_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/timer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/key.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/dac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/dsp_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32f4xx_it.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32f4xx_hal_msp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/sysmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/syscalls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/LCD/bsp_LCD_ILI9341.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/LCD/bsp_W25Q128.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/mooncat_anim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/myiic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/bmp280.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/esp8266.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/bsp_AD9833.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../startup_stm32f407xx.s
)

# STM32 HAL/LL Drivers
set(STM32_Drivers_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/system_stm32f4xx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dac_ex.c
)

# Drivers Midllewares

# CMSIS-DSP Library (ÕÍ’˚“∆÷≤)
set(CMSIS_DSP_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/Source/BasicMathFunctions/BasicMathFunctions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/Source/FastMathFunctions/FastMathFunctions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/Source/StatisticsFunctions/StatisticsFunctions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/Source/ComplexMathFunctions/ComplexMathFunctions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/Source/ControllerFunctions/ControllerFunctions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/Source/FilteringFunctions/FilteringFunctions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/Source/MatrixFunctions/MatrixFunctions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/Source/TransformFunctions/TransformFunctions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/Source/SupportFunctions/SupportFunctions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/Source/SVMFunctions/SVMFunctions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/Source/BayesFunctions/BayesFunctions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/Source/DistanceFunctions/DistanceFunctions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/Source/InterpolationFunctions/InterpolationFunctions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/Source/QuaternionMathFunctions/QuaternionMathFunctions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/DSP/Source/CommonTables/CommonTables.c
)

# Add CMSIS-DSP sources to the project
add_library(CMSIS_DSP OBJECT ${CMSIS_DSP_SRC})
target_include_directories(CMSIS_DSP PRIVATE ${MX_Include_Dirs})
target_link_libraries(CMSIS_DSP PUBLIC stm32cubemx)


# Link directories setup
set(MX_LINK_DIRS

)
# Project static libraries
set(MX_LINK_LIBS 
    STM32_Drivers
    CMSIS_DSP
    ${TOOLCHAIN_LINK_LIBRARIES}
    
)
# Interface library for includes and symbols
add_library(stm32cubemx INTERFACE)
target_include_directories(stm32cubemx INTERFACE ${MX_Include_Dirs})
target_compile_definitions(stm32cubemx INTERFACE ${MX_Defines_Syms})

# Create STM32_Drivers static library
add_library(STM32_Drivers OBJECT)
target_sources(STM32_Drivers PRIVATE ${STM32_Drivers_Src})
target_link_libraries(STM32_Drivers PUBLIC stm32cubemx)


# Add STM32CubeMX generated application sources to the project
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${MX_Application_Src})

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${MX_LINK_DIRS})

# Add libraries to the project
target_link_libraries(${CMAKE_PROJECT_NAME} ${MX_LINK_LIBS})

# Add the map file to the list of files to be removed with 'clean' target
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_PROJECT_NAME}.map)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()
