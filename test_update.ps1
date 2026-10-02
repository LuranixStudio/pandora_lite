$ErrorActionPreference = 'Stop'
$testDir = Join-Path $env:RUNNER_TEMP 'pandora-update-test'
New-Item -ItemType Directory -Force $testDir | Out-Null
$destination = Join-Path $testDir 'installed.exe'
$stage = Join-Path $testDir 'staged.exe'
Copy-Item .\build\Release\pandora_lite.exe $destination
Copy-Item .\build\Release\pandora_lite.exe $stage
$hash = (Get-FileHash $stage -Algorithm SHA256).Hash.ToLowerInvariant()
$old = Start-Process $destination -PassThru
try {
  $argsLine = '--apply-update {0} "{1}" {2}' -f $old.Id, $destination, $hash
  $updater = Start-Process $stage -ArgumentList $argsLine -PassThru
  Start-Sleep -Seconds 1
  $updater.Refresh()
  if ($updater.HasExited) { throw 'Updater did not wait for old process' }
  Stop-Process -Id $old.Id
  if (!$updater.WaitForExit(20000)) { throw 'Updater timed out' }
  if ($updater.ExitCode -ne 0) { throw "Updater failed: $($updater.ExitCode)" }
  if (!(Test-Path "$destination.bak")) { throw 'Backup missing' }
  if ((Get-FileHash $destination).Hash.ToLowerInvariant() -ne $hash) { throw 'Installed hash mismatch' }
  Start-Sleep -Seconds 2
  $restart = @(Get-Process installed -ErrorAction SilentlyContinue)
  if (!$restart.Count) { throw 'Updated app did not restart' }
  $restart | Stop-Process
  $bad = Start-Process $stage -ArgumentList ('--apply-update 999999 "{0}" {1}' -f $destination, ('0' * 64)) -Wait -PassThru
  if ($bad.ExitCode -ne 2) { throw 'Tampered update was not rejected' }
  if ((Get-FileHash $destination).Hash.ToLowerInvariant() -ne $hash) { throw 'Tampered update modified installed app' }
  Write-Output 'Update wait, SHA256, replacement, backup, restart and tamper rejection passed'
} finally {
  Get-Process installed,staged -ErrorAction SilentlyContinue | Stop-Process -ErrorAction SilentlyContinue
}
