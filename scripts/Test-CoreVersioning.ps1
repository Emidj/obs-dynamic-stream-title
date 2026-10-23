param([string]$Installer,[string]$ObsExe,[string]$Target)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force $Target | Out-Null
$a=[Reflection.Assembly]::LoadFile((Resolve-Path $Installer).Path);$t=$a.GetType('Setup');$flags=[Reflection.BindingFlags]'Static,NonPublic'
$t.GetField('FixtureMode',$flags).SetValue($null,$true)
function Call([string]$name,[object[]]$values){$t.GetMethod($name,$flags).Invoke($null,$values)}
function Install([string]$root){Call 'Install' ([object[]]@([string]$root))}
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class ContractFixture {
 [DllImport("kernel32",CharSet=CharSet.Unicode,SetLastError=true)] static extern IntPtr BeginUpdateResource(string path,bool delete);
 [DllImport("kernel32",CharSet=CharSet.Unicode,SetLastError=true)] static extern bool UpdateResource(IntPtr h,IntPtr type,string name,ushort language,byte[] data,uint count);
 [DllImport("kernel32",SetLastError=true)] static extern bool EndUpdateResource(IntPtr h,bool discard);
 public static void Write(string path,byte[] data){
  var h=BeginUpdateResource(path,false);if(h==IntPtr.Zero)throw new Exception("BeginUpdateResource failed");
  if(!UpdateResource(h,new IntPtr(10),"NEXUS_TWITCH_CONTRACT",0x409,data,(uint)data.Length)){EndUpdateResource(h,true);throw new Exception("UpdateResource failed");}
  if(!EndUpdateResource(h,false))throw new Exception("EndUpdateResource failed");
 }
}
"@
$root=Join-Path $Target 'OBS';New-Item -ItemType Directory -Force "$root/bin/64bit" | Out-Null
Copy-Item $ObsExe "$root/bin/64bit/obs64.exe";Set-Content "$root/portable_mode.txt" ''
Install $root
$core="$root/bin/64bit/nexus-twitch-core.dll";$record="$root/data/nexus-twitch-core/runtime.json"
$meta=Get-Content $record -Raw | ConvertFrom-Json
if($meta.Version -ne '1.1.0' -or $meta.Abi -ne 6){throw 'Core version follows plugin version'}
$contract=Call 'DllContract' ([object[]]@([string]$core))
if($contract.Version -ne '1.1.0'){throw 'Embedded resource missing'}
$pluginContract=Call 'DllContract' ([object[]]@([string]"$root/obs-plugins/64bit/obs-dynamic-stream-title.dll"))
if($pluginContract.MinVersion -ne '1.1.0' -or $pluginContract.MaxVersionExclusive -ne '2.0.0'){throw 'Consumer contract missing'}
# Synthetic inert PE resource fixtures: never executed.
[ContractFixture]::Write($core,[Text.Encoding]::UTF8.GetBytes('{"Schema":1,"Kind":"core","Version":"1.2.0","Abi":6}'))
$meta.Version='1.2.0';$meta.Hash=(Get-FileHash $core).Hash.ToLowerInvariant();$meta | ConvertTo-Json -Depth 8 | Set-Content $record -Encoding UTF8
$newHash=(Get-FileHash $core).Hash
Install $root
$meta=Get-Content $record -Raw | ConvertFrom-Json
if((Get-FileHash $core).Hash -ne $newHash -or $meta.Version -ne '1.2.0'){throw 'Newer compatible Core downgraded or mislabeled'}
# Existing compatible Overlay accepts a different binary hash and independent consumer range.
$overlay="$root/obs-plugins/64bit/obs-twitch-overlays.dll"
Copy-Item 'build/integration/overlay/RelWithDebInfo/obs-twitch-overlays.dll' $overlay
[ContractFixture]::Write($overlay,[Text.Encoding]::UTF8.GetBytes('{"Schema":1,"Kind":"consumer","Consumer":"obs-twitch-overlays","Abi":6,"MinVersion":"1.0.1","MaxVersionExclusive":"2.0.0"}'))
Install $root
# A declared incompatible consumer must block all changes and identify itself.
[ContractFixture]::Write($overlay,[Text.Encoding]::UTF8.GetBytes('{"Schema":1,"Kind":"consumer","Consumer":"obs-twitch-overlays","Abi":7,"MinVersion":"1.1.0","MaxVersionExclusive":"2.0.0"}'))
$before=(Get-FileHash $record).Hash;$failed=$false
try{Install $root}catch{$failed=$_.Exception.ToString().Contains('obs-twitch-overlays')}
if(!$failed -or (Get-FileHash $core).Hash -ne $newHash -or (Get-FileHash $record).Hash -ne $before){throw 'Incompatible consumer not blocked atomically'}
# Malformed range cannot become implicit compatibility.
[ContractFixture]::Write($overlay,[Text.Encoding]::UTF8.GetBytes('{"Schema":1,"Kind":"consumer","Consumer":"obs-twitch-overlays","Abi":6,"MinVersion":"bad","MaxVersionExclusive":"2.0.0"}'))
$failed=$false;try{Install $root}catch{$failed=$true};if(!$failed){throw 'Malformed contract accepted'}
# A newer incompatible major is rejected, never downgraded.
Copy-Item 'build/integration/overlay/RelWithDebInfo/obs-twitch-overlays.dll' $overlay -Force
[ContractFixture]::Write($core,[Text.Encoding]::UTF8.GetBytes('{"Schema":1,"Kind":"core","Version":"2.0.0","Abi":7}'))
$meta=Get-Content $record -Raw | ConvertFrom-Json;$meta.Version='2.0.0';$meta.Hash=(Get-FileHash $core).Hash.ToLowerInvariant();$meta | ConvertTo-Json -Depth 8 | Set-Content $record -Encoding UTF8
$majorHash=(Get-FileHash $core).Hash;$failed=$false;try{Install $root}catch{$failed=$true}
if(!$failed -or (Get-FileHash $core).Hash -ne $majorHash){throw 'Incompatible newer major downgraded'}
# An older owned Core is upgraded only after all consumer ranges are checked.
[ContractFixture]::Write($core,[Text.Encoding]::UTF8.GetBytes('{"Schema":1,"Kind":"core","Version":"0.9.0","Abi":6}'))
$meta.Version='0.9.0';$meta.Hash=(Get-FileHash $core).Hash.ToLowerInvariant();$meta | ConvertTo-Json -Depth 8 | Set-Content $record -Encoding UTF8
Install $root
$meta=Get-Content $record -Raw | ConvertFrom-Json
if($meta.Version -ne '1.1.0' -or $meta.Abi -ne 6){throw 'Compatible upgrade failed'}
# Same version with different content is an integrity error, not a compatibility claim.
[ContractFixture]::Write($core,[Text.Encoding]::UTF8.GetBytes('{"Schema":1,"Kind":"core","Version":"1.1.0","Abi":6,"Fixture":true}'))
$meta.Hash=(Get-FileHash $core).Hash.ToLowerInvariant();$meta | ConvertTo-Json -Depth 8 | Set-Content $record -Encoding UTF8
$failed=$false;try{Install $root}catch{$failed=$true};if(!$failed){throw 'Conflicting same-version build accepted'}
'PASS: independent version and embedded contracts; newer compatible Core retained; independent Overlay hash accepted; incompatible ABI/major, malformed range and same-version conflict rejected; older owned Core upgraded; incompatible operations preserve files.'
