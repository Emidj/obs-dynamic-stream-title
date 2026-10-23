$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $project
$version='1.0.0'
$coreVersion=(Get-Content shared/twitch-core-version.json -Raw | ConvertFrom-Json).Version
$source=Join-Path $project ('build/source-'+[Guid]::NewGuid().ToString('N'))
$sourceRoot=Join-Path $source 'OBS-Dynamic-Stream-Title'
New-Item -ItemType Directory -Force $sourceRoot | Out-Null
foreach($file in @('CMakeLists.txt','LICENSE','README.md','BUILD.md')){Copy-Item -LiteralPath $file -Destination $sourceRoot}
foreach($folder in @('src','shared','scripts','data','tests','installer','integration')){Copy-Item -LiteralPath $folder -Destination $sourceRoot -Recurse}
$captureTarget=Join-Path $sourceRoot 'docs/validation/localization'
New-Item -ItemType Directory -Force $captureTarget | Out-Null
Copy-Item -LiteralPath docs/validation/localization/captures -Destination $captureTarget -Recurse
Copy-Item -LiteralPath docs/Twitch-Core-Versioning.md -Destination (Join-Path $sourceRoot 'docs')
Copy-Item -LiteralPath docs/roadmap.md -Destination (Join-Path $sourceRoot 'docs')
$sourceZip=Join-Path $project "dist/OBS-Dynamic-Stream-Title-$version-Source.zip"
Compress-Archive -LiteralPath $sourceRoot -DestinationPath $sourceZip -Force
$package=Join-Path $project ('build/package-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force $package | Out-Null
foreach($file in @("dist/OBS-Dynamic-Stream-Title-$version-Windows-x64-Setup.exe",$sourceZip,'dist/LEEME.txt','LICENSE',"dist/Twitch-Core-$coreVersion.json",'docs/Twitch-Core-Versioning.md','output/pdf/Dynamic-Stream-Title-User-Guide-ES-EN-IT-FR-DE.pdf')){Copy-Item -LiteralPath $file -Destination $package}
$packageZip=Join-Path $project "dist/OBS-Dynamic-Stream-Title-$version-Package.zip"
Compress-Archive -Path (Join-Path $package '*') -DestinationPath $packageZip -Force
Get-FileHash "dist/OBS-Dynamic-Stream-Title-$version-Windows-x64-Setup.exe",$sourceZip,$packageZip,"dist/Twitch-Core-$coreVersion.json",'output/pdf/Dynamic-Stream-Title-User-Guide-ES-EN-IT-FR-DE.pdf' | ForEach-Object {$_.Hash.ToLowerInvariant()+'  '+(Split-Path $_.Path -Leaf)} | Set-Content dist/SHA256SUMS.txt -Encoding ascii
Write-Output $packageZip
