# Third-party components and source provenance

Dynamic Stream Title and the included NEXUS Twitch Core / adapted OBS Twitch Overlays sources are distributed under GPL-2.0-or-later; retain both LICENSE files and source headers. The integration history is recorded in integration/overlay/SOURCE.md. This is a source integration copy, not a separate update to an installed Overlay.

The Unicode / CLDR-derived emoji catalogue retains the Unicode License V3 in data/emoji/LICENSE.txt. Artwork provenance and generation notes are retained in integration/overlay/data/presets/*ARTWORK.md. These notices do not transfer rights to third-party trademarks.

The Windows build links against OBS libobs / frontend APIs (GPL), Qt 6.8.3 Widgets / Network (Qt open-source licensing), and libcurl (curl license). These are external build dependencies; their SDKs and runtime DLLs are not redistributed in this source archive. The installer uses the compatible OBS runtime already installed on the target machine. Obtain dependencies from their official projects and preserve their applicable notices when redistributing them. Windows and the .NET Framework are system components.

The published installer is the unchanged official 1.0.0 artifact. The publication copy changes documentation and removes personal development paths from five provenance/prompt documents; no C++, C#, scripts, translations or plugin behavior were changed during publication.
