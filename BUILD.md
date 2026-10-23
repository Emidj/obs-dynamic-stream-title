# Compilar y empaquetar

Requiere Windows x64, Visual Studio 2022 C++ Build Tools, CMake 3.28+, Qt 6.8.3, SDK libobs/obs-frontend-api y CURL con paquetes CMake compatibles. El motor reutilizado se compila desde la copia incluida de Overlay; no necesita el repositorio original como fuente. Se utiliza el SDK OBS 31.1.1 para conservar una base de símbolos compatible; el módulo comprueba OBS 32.x al cargar.

Configurar `CMAKE_PREFIX_PATH`, `libobs_DIR` y `obs-frontend-api_DIR` con las dependencias locales. No se incluyen SDK ni credenciales en las fuentes.

```powershell
./scripts/Invoke-CMake.ps1 -S . -B build -DDST_BUILD_OVERLAY_INTEGRATION=ON
./scripts/Invoke-CMake.ps1 --build build --config RelWithDebInfo
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

`DST_BUILD_OVERLAY_INTEGRATION=OFF` omite el módulo Overlay de la construcción predeterminada; el Core sigue siendo necesario. Para empaquetar se necesita el hash del Overlay adaptado, por lo que se usa ON.

Interfaz: editar `data/locale/translations.txt` y ejecutar `scripts/Build-Translations.py` con Python. Emojis: `scripts/Build-EmojiCatalog.ps1`; usa fuentes Unicode/CLDR incluidas y su licencia. La traducción no altera datos del usuario ni bloques publicados.

Guía: `scripts/build-user-guide.py`, con Python, reportlab, pypdf y pypdfium2. Requiere las capturas en `docs/validation/localization/captures` y las fuentes Arial de Windows. Genera un PDF de quince páginas y sus previsualizaciones. No genera el PDF descartado de fases.

Instalador: `installer/build.ps1`, con C# de .NET Framework 4.x. Incrusta DLL, Core, traducciones, imagen NEXUS, guía y licencias. Autoprueba sin modificar una instalación habitual:

```powershell
./dist/OBS-Dynamic-Stream-Title-1.0.0-Windows-x64-Setup.exe /selftest C:\carpeta-nueva-de-pruebas C:\OBS\bin\64bit\obs64.exe C:\informe.log
```

Elegir una carpeta de pruebas nueva. También admite `/install "raíz OBS"` y `/uninstall "raíz OBS"`; para uso normal, abrir su interfaz. Empaquetado con fuentes: `scripts/package.ps1`. Estas comprobaciones no sustituyen los directos pendientes.

Twitch Core 1.1.0 ABI 6 incorpora require_live opcional en guarded_update; Title requiere >=1.1.0. Los contratos de versión están en shared/. Después de modificar UI, renovar capturas y guía antes de compilar el instalador final.
