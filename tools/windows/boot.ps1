# Hidden hardware boot test; ClayTality 64 by djultra64.
param([string]$Root='C:\Users\ryarcade\Documents\Codex\ClayTality64', [string]$Profile='original', [int]$Seconds=35, [string]$InputScript='')
$ErrorActionPreference='Stop'
Set-Location $Root
$env:CLAY_TRACE_GAME='1';$env:CLAY_TRACE_INPUT='1';$env:CLAY_TRACE_INTERPOLATION='1'
$args=@('--hidden','--rom','probe-output/base.z64','--graphics-profile',$Profile,'--seconds',"$Seconds")
if($InputScript){$args+=@('--test-input',$InputScript)}
$out="$Root\windows-boot-$Profile.stdout.log";$err="$Root\windows-boot-$Profile.stderr.log"
$start=New-Object System.Diagnostics.ProcessStartInfo
$start.FileName="$Root\build\Release\claytality64.exe";$start.Arguments=$args -join ' ';$start.WorkingDirectory=$Root
$start.UseShellExecute=$false;$start.CreateNoWindow=$true;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
$p=New-Object System.Diagnostics.Process;$p.StartInfo=$start
$outFile=[IO.File]::Open($out,[IO.FileMode]::Create,[IO.FileAccess]::Write,[IO.FileShare]::ReadWrite)
$errFile=[IO.File]::Open($err,[IO.FileMode]::Create,[IO.FileAccess]::Write,[IO.FileShare]::ReadWrite)
[void]$p.Start();$outCopy=$p.StandardOutput.BaseStream.CopyToAsync($outFile);$errCopy=$p.StandardError.BaseStream.CopyToAsync($errFile)
if(-not $p.WaitForExit(($Seconds+30)*1000)){Stop-Process -Id $p.Id -Force;throw 'Native hidden boot exceeded its bounded deadline'}
$p.WaitForExit();$exitCode=$p.ExitCode;$outCopy.Wait();$errCopy.Wait();$outFile.Dispose();$errFile.Dispose();$p.Dispose()
Get-Content $err -Tail 25
@{profile=$Profile;seconds=$Seconds;exit_code=$exitCode;hidden=$true} | ConvertTo-Json | Set-Content "$Root\windows-boot-$Profile.json"
if($null -eq $exitCode){throw "Unknown process exit code"}
if($exitCode -ne 0){exit $exitCode}
