# Compile against real ARM/FreeRTOS types without claiming a full flight link.
# The supplied kernel tree is a local test dependency, not a selected mission RTOS.
param([Parameter(Mandatory=$true)][string]$KernelPath)
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path "$PSScriptRoot/../../../..").Path
$taskKernel=(Resolve-Path $KernelPath).Path
$taskCompiler="$env:LOCALAPPDATA/Arduino15/packages/STMicroelectronics/tools/xpack-arm-none-eabi-gcc/14.2.1-1.1/bin/arm-none-eabi-gcc.exe"
Push-Location $taskRoot
try {
 New-Item -ItemType Directory -Force work | Out-Null
 $taskFlags=@('-std=c11','-Wall','-Wextra','-Werror','-mcpu=cortex-m4','-mthumb','-mfpu=fpv4-sp-d16','-mfloat-abi=hard',"-I$taskKernel/include","-I$taskKernel/portable/GCC/ARM_CM4F",'-IHAB1/onboard/stm32/tests/arm_config','-DHAB_ENABLE_SD_LOGGING=1')
 foreach($taskSource in @('flight_tasks','sd_logging')) {
  & $taskCompiler @taskFlags -c "HAB1/onboard/stm32/apps/flight/$taskSource.c" -o "work/$($taskSource)_arm.o"
  if($LASTEXITCODE -ne 0) {throw "Compile failed: $taskSource"}
 }
 $taskVendor='HAB1/onboard/stm32/modules/drivers/barometer/barometer'
 $taskFlags+=@('-DUSE_HAL_DRIVER','-DSTM32F411xE',"-I$taskVendor/Core/Inc","-I$taskVendor/Drivers/STM32F4xx_HAL_Driver/Inc","-I$taskVendor/Drivers/CMSIS/Include","-I$taskVendor/Drivers/CMSIS/Device/ST/STM32F4xx/Include",'-IHAB1/onboard/stm32/third_party/fatfs')
 & $taskCompiler @taskFlags -c HAB1/onboard/stm32/platform/board/board_sd.c -o work/board_sd_arm.o
 if($LASTEXITCODE -ne 0) {throw 'Board SD adapter compile failed'}
 Write-Output 'PASS: enabled flight tasks, queue and board adapter ARM compilation'
} finally {Pop-Location}
