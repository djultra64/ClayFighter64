# Hidden native verification only; ClayFighter 64 by djultra64.
param([string]$Root='C:\Users\ryarcade\Documents\Codex\ClayTality64', [string]$CMake='C:\Users\ryarcade\Documents\Codex\Tetrisphere-M1\tools\cmake-3.31.8-windows-x86_64\bin\cmake.exe', [string]$Version='development')
$ErrorActionPreference='Stop'
Set-Location $Root
$ErrorActionPreference='Continue'
& $CMake.Replace('cmake.exe','ctest.exe') --test-dir build -C Release --output-on-failure *> tests.log
$testCode=$LASTEXITCODE
$ErrorActionPreference='Stop'
Get-Content tests.log
$p=Start-Process -FilePath "$Root\build\Release\async_resource_wait_test.exe" -ArgumentList original -NoNewWindow -PassThru -RedirectStandardOutput "$Root\wait-red.stdout.log" -RedirectStandardError "$Root\wait-red.stderr.log"
$redTimedOut=-not $p.WaitForExit(2000)
if($redTimedOut){Stop-Process -Id $p.Id -Force}
$result=@{project='ClayFighter 64';author='djultra64';version=$Version;ctest_exit=$testCode;framebuffer_original_timeout=$redTimedOut;framebuffer_timeout_seconds=2;portable_settings='local/settings';portable_cache='local/cache';completed_utc=[DateTime]::UtcNow.ToString('o')}
$result | ConvertTo-Json | Set-Content windows-tests.json
if($testCode -ne 0 -or -not $redTimedOut){exit 1}
