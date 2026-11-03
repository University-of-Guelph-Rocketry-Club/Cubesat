# C-only bench build: reuse the repository's vendor HAL/startup, then link our logger stack.
# This script compiles firmware but never flashes a board or writes an SD card.
$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path "$PSScriptRoot/../../../../../../..").Path
$taskStm = "$taskRoot/HAB1/onboard/stm32"
$taskVendor = "$taskStm/modules/drivers/barometer/barometer"
$taskTool = "$env:LOCALAPPDATA/Arduino15/packages/STMicroelectronics/tools/xpack-arm-none-eabi-gcc/14.2.1-1.1/bin"
if (-not (Test-Path "$taskTool/arm-none-eabi-gcc.exe")) { throw 'Set taskTool to an installed GNU Arm compiler' }
$taskBuild = "$PSScriptRoot/build"
New-Item -ItemType Directory -Force $taskBuild | Out-Null
$taskFlags = @('-std=c11','-Os','-g3','-Wall','-Wextra','-mcpu=cortex-m4','-mthumb',
 '-mfpu=fpv4-sp-d16','-mfloat-abi=hard','-ffunction-sections','-fdata-sections',
 '-DUSE_HAL_DRIVER','-DSTM32F411xE',"-I$taskVendor/Core/Inc",
 "-I$taskVendor/Drivers/STM32F4xx_HAL_Driver/Inc", "-I$taskVendor/Drivers/CMSIS/Include",
 "-I$taskVendor/Drivers/CMSIS/Device/ST/STM32F4xx/Include",
 "-I$taskStm/third_party/fatfs", "-I$taskStm/modules/sd_logger", "-I$taskStm/modules/drivers/sd_card")
$taskSources = @("$PSScriptRoot/main.c", "$taskStm/modules/sd_logger/sd_logger.c",
 "$taskStm/modules/sd_logger/sd_fatfs.c", "$taskStm/modules/drivers/sd_card/sd_spi.c",
 "$taskStm/modules/drivers/sd_card/sd_spi_hal.c", "$taskStm/third_party/fatfs/ff.c",
 "$taskVendor/Core/Src/system_stm32f4xx.c", "$taskVendor/Core/Src/sysmem.c",
 "$taskVendor/startup_stm32f411xe.s")
foreach ($taskDriver in @('hal','hal_cortex','hal_gpio','hal_spi','hal_uart','hal_rcc','hal_rcc_ex','hal_flash','hal_flash_ex','hal_pwr','hal_pwr_ex','hal_dma')) {
 $taskSources += "$taskVendor/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_$taskDriver.c"
}
$taskObjects = @()
# Keep object files separate so compile failures identify the source immediately.
foreach ($taskSource in $taskSources) {
 $taskObject = "$taskBuild/$([IO.Path]::GetFileName($taskSource)).o"
 & "$taskTool/arm-none-eabi-gcc.exe" @taskFlags -c $taskSource -o $taskObject
 if ($LASTEXITCODE -ne 0) { throw "Compile failed: $taskSource" }
 $taskObjects += $taskObject
}
& "$taskTool/arm-none-eabi-gcc.exe" @taskFlags @taskObjects "-T$taskVendor/STM32F411xx_FLASH.ld" --specs=nano.specs --specs=nosys.specs '-Wl,--gc-sections' '-Wl,-u,_printf_float' "-Wl,-Map=$taskBuild/logger.map" '-Wl,--defsym=_Min_Stack_Size=0x2000' -lc -lm -o "$taskBuild/logger.elf"
if ($LASTEXITCODE -ne 0) { throw 'Link failed' }
& "$taskTool/arm-none-eabi-objcopy.exe" -O binary "$taskBuild/logger.elf" "$taskBuild/logger.bin"
& "$taskTool/arm-none-eabi-size.exe" "$taskBuild/logger.elf"
