# Build isolated host executables; any compiler warning or failed assertion stops the run.
# work/ is ignored and none of these tests accesses physical storage.
$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path "$PSScriptRoot/../../../..").Path
Push-Location $taskRoot
try {
 New-Item -ItemType Directory -Force work | Out-Null
 $taskBase = 'HAB1/onboard/stm32'
 $taskLogger = "$taskBase/modules/sd_logger/sd_logger.c"
 $taskFat = "$taskBase/third_party/fatfs"
 $taskFlags = @('-std=c11','-Wall','-Wextra','-Werror','-pedantic')
 $taskSuites = @(
  @{Name='sd_logger';Sources=@($taskLogger,"$taskBase/tests/sd_logger_test.c");Flags=@()},
  @{Name='sd_fatfs';Sources=@($taskLogger,"$taskBase/modules/sd_logger/sd_fatfs.c","$taskFat/ff.c","$taskBase/tests/sd_fatfs_test.c");Flags=@("-I$taskFat",'-funsigned-char')},
  @{Name='sd_spi';Sources=@("$taskBase/modules/drivers/sd_card/sd_spi.c","$taskBase/tests/sd_spi_test.c");Flags=@("-I$taskFat")},
  @{Name='sd_logging_queue';Sources=@($taskLogger,"$taskBase/apps/flight/sd_logging.c","$taskBase/tests/sd_logging_queue_test.c");Flags=@("-I$taskBase/tests/rtos_stub")}
 )
 foreach($taskSuite in $taskSuites) {
  $taskExtra = $taskSuite.Flags; $taskSources = $taskSuite.Sources
  $taskExe = "work/$($taskSuite.Name)_test.exe"
  & gcc @taskFlags @taskExtra @taskSources -o $taskExe
  if($LASTEXITCODE -ne 0) { throw "Compile failed: $($taskSuite.Name)" }
  & "./$taskExe"
  if($LASTEXITCODE -ne 0) { throw "Test failed: $($taskSuite.Name)" }
 }
} finally { Pop-Location }
