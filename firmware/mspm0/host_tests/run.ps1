param(
    [string]$Gcc = "gcc"
)

$ErrorActionPreference = "Stop"
$HostTests = Split-Path -Parent $MyInvocation.MyCommand.Path
$FirmwareRoot = Split-Path -Parent $HostTests
$Project = Join-Path $FirmwareRoot "3507test1"
$Output = Join-Path ([IO.Path]::GetTempPath()) "diansai_host_module_tests.exe"

$TestSources = Get-ChildItem $HostTests -Filter "test_*.c" |
    Sort-Object Name |
    ForEach-Object { $_.FullName }

$ModuleSources = @(
    "$Project\ModuleTests\01_power_monitor\test_power_monitor.c",
    "$Project\ModuleTests\02_button_estop\test_button_estop.c",
    "$Project\ModuleTests\03_motor_tb6612\test_motor_tb6612.c",
    "$Project\ModuleTests\04_encoder\test_encoder.c",
    "$Project\ModuleTests\05_line_sensor\test_line_sensor.c",
    "$Project\ModuleTests\06_tof_stp23l\test_tof_stp23l.c",
    "$Project\ModuleTests\07_vision_uart\test_vision_uart.c",
    "$Project\ModuleTests\08_gimbal\test_gimbal.c"
)

$Arguments = @(
    "-std=c11",
    "-Wall",
    "-Wextra",
    "-Werror",
    "-I$Project",
    (Join-Path $HostTests "main.c")
) + $TestSources + $ModuleSources + @("-o", $Output)

& $Gcc @Arguments
if ($LASTEXITCODE -ne 0) {
    throw "Host test compilation failed"
}

& $Output
if ($LASTEXITCODE -ne 0) {
    throw "Host module tests failed"
}