# Isolated MSVC native build; ClayFighter 64 by djultra64.
param([string]$Root='C:\Users\ryarcade\Documents\Codex\ClayTality64', [string]$CMake='C:\Users\ryarcade\Documents\Codex\Tetrisphere-M1\tools\cmake-3.31.8-windows-x86_64\bin\cmake.exe', [string]$Runtime, [string]$RT64, [string]$RT64Build, [string]$SDL)
$ErrorActionPreference='Stop'
Set-Location $Root
if(Test-Path 'host\training_guest.cpp') {
 if(-not (Test-Path 'generated\windows_training_guest.cpp')) {
  throw 'Missing private Training fixture: run tools/windows/prepare_training.py on the preparation host and transfer it with current host/ and tests/.'
 }
}
& $CMake -S tools/windows -B build -G 'Visual Studio 17 2022' -A x64 "-DCLAY_RUNTIME=$Runtime" "-DCLAY_RT64=$RT64" "-DCLAY_RT64_BUILD=$RT64Build" "-DCLAY_SDL=$SDL"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $CMake --build build --config Release --parallel 6
exit $LASTEXITCODE
