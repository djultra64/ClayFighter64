# Actual logical-button hidden donor combat verification; djultra64.
param([string]$Root='C:\Users\ryarcade\Documents\Codex\ClayTality64', [int[]]$Rows=@(0,1,2,3), [int]$Seconds=70, [switch]$Extended)
$ErrorActionPreference='Stop';$ProgressPreference='SilentlyContinue';Set-Location $Root
$env:CLAY_TRACE_GAME='1';$env:CLAY_TRACE_INPUT='1';$env:CLAY_TRACE_INTERPOLATION='1'
$reports=@()
foreach($row in $Rows){
 $id=@(6,11,12,16)[$row];$events=@(@{at_ms=1500;button='start'},@{at_ms=3000;button='start'},@{at_ms=4500;button='south'},@{at_ms=6500;button='south'},@{at_ms=10000;button='right'},@{at_ms=12000;button='right'},@{at_ms=14000;button='right'})
 for($down=0;$down -lt $row;$down++){$events+=@{at_ms=16000+2000*$down;button='down'}}
 $events+=@{at_ms=16000+2000*$row;button='south'}
 $events+=@(@{at_ms=34000;button='right';duration_ms=400},@{at_ms=35000;button='south'},@{at_ms=35500;button='west'},@{at_ms=36000;button='north'},@{at_ms=36500;button='east'},@{at_ms=40000;button='start'},@{at_ms=42000;button='south'})
 for($n=0;$n -lt 40;$n++){$events+=@{at_ms=45000+$n*500;button=@('south','east','north','west')[$n%4]}}
 if($Extended){
  $events=@($events|Where-Object {-not ($_.at_ms -eq 34000 -and $_.button -eq 'right')})
  for($at=34000;$at -lt ($Seconds*1000-5000);$at+=10000){$events+=@{at_ms=$at;button='right';duration_ms=4000};$events+=@{at_ms=$at+4500;button='left';duration_ms=4000}}
  for($at=66000;$at -lt ($Seconds*1000-5000);$at+=350){$events+=@{at_ms=$at;button=@('south','east','west','north')[[int](($at-66000)/350)%4]}}
 }
 $script="$Root\windows-donor-$id-input.json";$events|ConvertTo-Json|Set-Content $script
 $out="$Root\windows-donor-$id.stdout.log";$err="$Root\windows-donor-$id.stderr.log"
 $args=@('--hidden','--rom','probe-output/base.z64','--sc-assets','local/sc_roster/asset-image-roster.z64','--graphics-profile','4k120','--seconds',"$Seconds",'--user-data',"local/windows-test-users/donor-$id",'--test-input',"windows-donor-$id-input.json")
 $start=New-Object System.Diagnostics.ProcessStartInfo
 $start.FileName="$Root\build\Release\claytality64.exe";$start.Arguments=$args -join ' ';$start.WorkingDirectory=$Root
 $start.UseShellExecute=$false;$start.CreateNoWindow=$true;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
 $p=New-Object System.Diagnostics.Process;$p.StartInfo=$start
 $outFile=[IO.File]::Open($out,[IO.FileMode]::Create,[IO.FileAccess]::Write,[IO.FileShare]::ReadWrite)
 $errFile=[IO.File]::Open($err,[IO.FileMode]::Create,[IO.FileAccess]::Write,[IO.FileShare]::ReadWrite)
 [void]$p.Start();$outCopy=$p.StandardOutput.BaseStream.CopyToAsync($outFile);$errCopy=$p.StandardError.BaseStream.CopyToAsync($errFile)
 $timeout=-not $p.WaitForExit(($Seconds+30)*1000);if($timeout){Stop-Process -Id $p.Id -Force}
 $p.WaitForExit();$exitCode=$p.ExitCode;$outCopy.Wait();$errCopy.Wait();$outFile.Dispose();$errFile.Dispose();$p.Dispose();if($null -eq $exitCode){throw "Process exit code unavailable; no success claim"}
 $loaded=@();$modes=@();$health=@();$finished=@();$graphics=@();$progress=@();$inputs=0;$pieces=0;$fractional=0;$velocities=0
 foreach($line in Get-Content $err){try{$event=$line|ConvertFrom-Json}catch{continue};switch($event.event){
  'combat_loaded' {$loaded+=$event}
  'combat_health' {$health+=$event}
  'combat_finished' {$finished+=$event}
  'combat_input_mode' {$modes+=$event}
  'graphics_effective' {$graphics+=$event}
  'progress' {$progress+=$event}
  'guest_input' {if($event.buttons -ne 0){$inputs++}}
  'actor_interpolation_piece' {$pieces++}
  'rt64_interpolation_observed' {if($event.weight -gt 0 -and $event.weight -lt 1){$fractional++};if($event.nonzero_vertex_velocities -gt 0){$velocities++}}
 }}
 $selected=@($loaded|Where-Object {$_.p1 -eq $id -and -not $_.attract}).Count -gt 0
 $recent=@($progress|Select-Object -Last 3);$advancing=$recent.Count -ge 2 -and $recent[-1].gfx -gt $recent[-2].gfx -and $recent[-1].polls -gt $recent[-2].polls
 $report=@{combat_health=$health;combat_finished=$finished;progress_advancing=$advancing;recent_progress=$recent;requested_id=$id;exit_code=$exitCode;timeout=$timeout;actual_nonattract_selection=$selected;combat_loaded=$loaded;mode_transitions=$modes;guest_nonzero_input_events=$inputs;actor_piece_trace_events=$pieces;fractional_observer_events=$fractional;vertex_velocity_events=$velocities;final_progress=($progress|Select-Object -Last 1);final_graphics=($graphics|Select-Object -Last 1);hidden=$true;seconds=$Seconds}
 $report|ConvertTo-Json -Depth 8|Set-Content "$Root\windows-donor-$id.json";$reports+=$report
 $report|ConvertTo-Json -Depth 8
 if($timeout -or $exitCode -ne 0 -or -not $selected -or -not $advancing){break}
}
$reports|ConvertTo-Json -Depth 8|Set-Content "$Root\windows-donor-combat-results.json"
if($reports.Count -ne $Rows.Count -or @($reports|Where-Object {$_.timeout -or $_.exit_code -ne 0 -or -not $_.actual_nonattract_selection -or -not $_.progress_advancing}).Count){exit 1}
