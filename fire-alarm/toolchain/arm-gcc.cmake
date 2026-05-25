set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(ARM_TOOLCHAIN_PREFIX "arm-none-eabi-")

set(CMAKE_C_COMPILER    ${ARM_TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER  ${ARM_TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER  ${ARM_TOOLCHAIN_PREFIX}gcc)
set(CMAKE_AR            ${ARM_TOOLCHAIN_PREFIX}ar)
set(CMAKE_OBJCOPY       ${ARM_TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_OBJDUMP       ${ARM_TOOLCHAIN_PREFIX}objdump)
set(CMAKE_SIZE          ${ARM_TOOLCHAIN_PREFIX}size)

set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m0plus -mthumb -Wall -Wextra -Os -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-mcpu=cortex-m0plus -mthumb -Wl,--gc-sections -Wl,-Map=fire-alarm.map --specs=nano.specs --specs=nosys.specs")
