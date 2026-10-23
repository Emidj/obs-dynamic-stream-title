param([string]$Installer,[string]$ObsExe,[string]$LegacyOverlay,[string]$Target)
$ErrorActionPreference='Stop'
$assembly=[Reflection.Assembly]::LoadFile((Resolve-Path $Installer).Path)
$type=$assembly.GetType('Setup')
$flags=[Reflection.BindingFlags]'Static,NonPublic'
$type.GetField('FixtureMode',$flags).SetValue($null,$true)
function Invoke-Setup([string]$Name,[object[]]$Arguments){$type.GetMethod($Name,$flags).Invoke($null,[object[]][string[]]$Arguments)}
$fixture=Join-Path $Target ('selftest-'+[guid]::NewGuid().ToString('N'))
Invoke-Setup 'SelfTest' @($fixture,(Resolve-Path $ObsExe).Path,(Join-Path $Target 'selftest.log'))
$root=Join-Path $Target 'migration'
New-Item -ItemType Directory -Force "$root/bin/64bit","$root/obs-plugins/64bit","$root/data/obs-plugins/obs-twitch-overlays/presets","$root/config/obs-studio/plugin_config/obs-twitch-overlays" | Out-Null
Copy-Item $ObsExe "$root/bin/64bit/obs64.exe"
$overlay="$root/obs-plugins/64bit/obs-twitch-overlays.dll"
Copy-Item $LegacyOverlay $overlay
Set-Content "$root/portable_mode.txt" ''
Set-Content "$root/data/obs-plugins/obs-twitch-overlays/presets/custom.json" 'custom-preset'
Set-Content "$root/config/obs-studio/plugin_config/obs-twitch-overlays/settings.json" 'user-settings'
$before=(Get-FileHash $overlay).Hash
Invoke-Setup 'Install' @($root)
if((Get-FileHash $overlay).Hash -eq $before){throw 'Overlay not upgraded'}
if(!(Get-ChildItem "$overlay.dst-backup-*") -or (Get-FileHash (Get-ChildItem "$overlay.dst-backup-*" | Select-Object -First 1).FullName).Hash -ne $before){throw 'Original backup missing'}
Invoke-Setup 'Install' @($root)
# A failed later write must roll back the earlier Overlay replacement.
Copy-Item $LegacyOverlay $overlay -Force
$readme="$root/data/obs-plugins/obs-dynamic-stream-title/README.txt"
$handle=[IO.File]::Open($readme,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read)
$failed=$false
try{Invoke-Setup 'Install' @($root)}catch{$failed=$true}finally{$handle.Dispose()}
if(!$failed -or (Get-FileHash $overlay).Hash -ne $before){throw 'Overlay rollback failed'}
Invoke-Setup 'Install' @($root)
# Plan migration of a standard global plugin layout and update its ownership hash.
$globalRoot=Join-Path $Target 'global-plugin'
New-Item -ItemType Directory -Force "$globalRoot/bin/64bit","$globalRoot/setup" | Out-Null
$globalDll="$globalRoot/bin/64bit/obs-twitch-overlays.dll";Copy-Item $LegacyOverlay $globalDll
$manifest=[IO.Path]::GetFullPath("$globalRoot/setup/installed.tsv")
[IO.File]::WriteAllText($manifest,"# OBS Twitch Overlays NEXUS 0.14.5`n$before`tbin/64bit/obs-twitch-overlays.dll`nKEEP`tdata/presets/custom.json`n")
$writes=New-Object 'System.Collections.Generic.Dictionary[string,byte[]]'
$type.GetMethod('PlanOverlayUpgrade',$flags).Invoke($null,[object[]]@([string]$globalDll,$writes.PSObject.BaseObject))
if(!$writes.ContainsKey($manifest) -or [Text.Encoding]::UTF8.GetString($writes[$manifest]) -notmatch 'KEEP'){throw 'Global manifest plan failed'}
if((Get-FileHash $globalDll).Hash -ne $before){throw 'Plan modified installed file'}
Invoke-Setup 'Uninstall' @($root)
if(!(Test-Path "$root/bin/64bit/nexus-twitch-core.dll") -or !(Test-Path $overlay)){throw 'Shared runtime or Overlay removed'}
if((Get-Content "$root/data/obs-plugins/obs-twitch-overlays/presets/custom.json") -ne 'custom-preset' -or (Get-Content "$root/config/obs-studio/plugin_config/obs-twitch-overlays/settings.json") -ne 'user-settings'){throw 'User data changed'}
'PASS: known Overlay migration, original DLL backup, reinstall, Title removal preserves Overlay/Core, presets/settings preserved, rollback on write failure, global ownership manifest updated.'
