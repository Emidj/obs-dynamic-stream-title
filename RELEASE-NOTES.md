# Dynamic Stream Title 1.0.0 — Official Release

## Updated build — 2026-11-03

Version remains **1.0.0**. The current installer and source assets are the updated OBS 32/33-compatible build. Earlier downloads are retained with `-initial-20261023` in their filenames. The original v1.0.0 tag remains the original release snapshot; the current corresponding source is the **Source.zip asset** and the repository update linked below.

- Plugin and installer now accept OBS 32.x and 33.x; range tests reject 31 and 34.
- Real isolated OBS 33.0.1 / Qt 6.11.1 loaded the plugin, registered/opened the dock and displayed the Unicode emoji picker correctly.
- Keeps the Qt 6.8.3 build baseline for OBS 32 binary compatibility.
- Dock header receives a minimum height so its name can wrap in a narrow panel.
- Isolated installation preserved an existing Twitch Core 1.3.1 ABI 6 exactly; the package still bundles Core 1.1.0 and retains newer compatible versions.

Development validation recorded 27/30 initial suites passing; three local-network suites timed out in isolation and passed outside it. Final UI, language and version-range checks passed 3/3. No Twitch publication or real OBS 33 raid/Hype event was performed. Extended live-stream testing and the earlier unattributed residual shutdown allocation remain unresolved boundaries.

## Features

An OBS Studio dock for Twitch titles, categories and tags: explicit Apply to Twitch, Thanks Raiders with channel deduplication, independent Hype Train prefix, live-state checks, conflict handling and session recovery. English, Spanish, Italian, French and German UI, installer and illustrated guide.

## Downloads

- **Windows-x64-Setup.exe:** current recommended installer. Close OBS first. Installer unsigned.
- **Source.zip:** current corresponding source, build scripts, notices and documentation.
- **Package.zip:** current installer, sources, guide, licenses and Core information together.
- **User-Guide-ES-EN-IT-FR-DE.pdf:** unchanged illustrated multilingual guide.
- **SHA256SUMS.txt:** checksums for current downloads. Files marked `initial-20261023` are retained earlier artifacts, not the current build.

GPL-2.0-or-later; retained third-party notices apply.
