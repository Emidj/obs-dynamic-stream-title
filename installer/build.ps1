$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $project
$release=Join-Path $project 'build/RelWithDebInfo'
$destination=Join-Path $project 'dist'
New-Item -ItemType Directory -Force $destination | Out-Null
$pluginHash=(Get-FileHash "$release/obs-dynamic-stream-title.dll").Hash.ToLowerInvariant()
$coreHash=(Get-FileHash "$release/nexus-twitch-core.dll").Hash.ToLowerInvariant()
$overlayHash=(Get-FileHash 'build/integration/overlay/RelWithDebInfo/obs-twitch-overlays.dll').Hash.ToLowerInvariant()
$version='1.0.0'
$coreContract=Get-Content shared/twitch-core-version.json -Raw | ConvertFrom-Json
if((Get-Item "$release/nexus-twitch-core.dll").VersionInfo.ProductVersion -ne $coreContract.Version){throw 'Recompila Twitch Core: su versión no coincide con el contrato.'}
$coreVersion=$coreContract.Version
$coreAbi=$coreContract.Abi
$guideHash=(Get-FileHash 'output/pdf/Dynamic-Stream-Title-User-Guide-ES-EN-IT-FR-DE.pdf').Hash.ToLowerInvariant()
$brandingHash=(Get-FileHash 'data/branding/installer-nexus.png').Hash.ToLowerInvariant()
$metadata="internal static class Payload { public const string Version=`"$version`",PluginHash=`"$pluginHash`",CoreHash=`"$coreHash`",OverlayHash=`"$overlayHash`",GuideHash=`"$guideHash`",BrandingHash=`"$brandingHash`",CoreVersion=`"$coreVersion`"; public const int CoreAbi=$coreAbi; }"
$metadata | Set-Content build/Payload.cs -Encoding utf8
$readme=@'
Dynamic Stream Title for Twitch — NEXUS 1.0.0
OBS 32.x, Windows x64. Cierra OBS y selecciona su carpeta raíz en el instalador.
Puedes usar el mismo EXE para instalar, actualizar o desinstalar.

Apply publica el borrador manual. Raiders y Hype solo cambian el título aplicado.
El mismo raider aparece una vez aunque envíe varios raids.
Sin casilla de bloqueo: las automatizaciones continúan al recuperar la sesión.
Los eventos solo publican si Twitch confirma que el canal está en directo.
Un conflicto pausa la operación afectada; Recargar canal reconcilia sin enviar el borrador.

Twitch Core reutiliza el motor de OBS Twitch Overlay. El instalador incluye el
Core y reutiliza una copia idéntica existente. Nunca reemplaza un Core desconocido
o utilizado por consumidores incompatibles. Overlay 0.14.5 reconocido se actualiza a la versión compartida con backup de su DLL.
No se modifican sus presets ni configuración. Otras compilaciones se conservan.
La desinstalación preserva el Core compartido, configuración, credenciales y backups.

Pruebas reales de Raiders/Hype y directos prolongados: pendientes hasta un stream.
La comprobación previa a publicar detecta cambios externos, pero Twitch no ofrece
comparación y actualización atómicas: otra edición puede coincidir entre ambas.
Limitación conocida: OBS informó 1 asignación residual al cerrar con ambos plugins; en revisión.
Instalador sin firma digital. Código GPL-2.0-or-later; fuente incluida por separado.
'@
$readme | Set-Content dist/LEEME.txt -Encoding utf8
$exe=Join-Path $destination "OBS-Dynamic-Stream-Title-$version-Windows-x64-Setup.exe"
& 'C:/Windows/Microsoft.NET/Framework64/v4.0.30319/csc.exe' /nologo /target:winexe /win32icon:data/branding/nexus.ico /win32manifest:installer/app.manifest /platform:x64 /optimize+ "/out:$exe" /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.Web.Extensions.dll /reference:System.Core.dll "/resource:$release/obs-dynamic-stream-title.dll,plugin" "/resource:$release/nexus-twitch-core.dll,core" /resource:dist/LEEME.txt,readme /resource:LICENSE,license "/resource:build/integration/overlay/RelWithDebInfo/obs-twitch-overlays.dll,overlay" /resource:data/branding/installer-nexus.png,branding /resource:data/locale/strings.json,translations /resource:shared/twitch-core-version.json,core-contract /resource:shared/contracts/title.json,title-contract /resource:shared/contracts/overlay.json,overlay-contract /resource:output/pdf/Dynamic-Stream-Title-User-Guide-ES-EN-IT-FR-DE.pdf,guide installer\Setup.cs build\Payload.cs
if($LASTEXITCODE -ne 0){throw 'Error al compilar el instalador.'}
Get-FileHash $exe -Algorithm SHA256 | ForEach-Object {$_.Hash.ToLowerInvariant()+'  '+(Split-Path $_.Path -Leaf)} | Set-Content dist/SHA256SUMS.txt -Encoding ascii
@{Name='NEXUS Twitch Core';Version=$coreVersion;Abi=$coreAbi;File='nexus-twitch-core.dll';SHA256=$coreHash;Consumers=@((Get-Content shared/contracts/title.json -Raw | ConvertFrom-Json),(Get-Content shared/contracts/overlay.json -Raw | ConvertFrom-Json))} | ConvertTo-Json -Depth 5 | Set-Content "dist/Twitch-Core-$coreVersion.json" -Encoding utf8
Write-Output $exe
