# Dynamic Stream Title 1.0.0 — Official Release

An OBS Studio dock for managing Twitch titles, categories and tags, with optional raid and Hype Train automation.

- Explicit Apply to Twitch publishing; automatic events preserve the pending draft.
- Thanks Raiders with channel deduplication and independent Hype Train prefix.
- Live-state checks, conflict handling and session recovery without silent retries.
- English, Spanish, Italian, French and German UI, installer and illustrated guide.
- Includes Twitch Core 1.1.0, ABI 6; Windows x64, OBS 32.x.

## Downloads

- **Windows-x64-Setup.exe:** recommended installation. Close OBS first. The installer is unsigned.
- **Source.zip:** reviewed source code, build and installer scripts, licenses and documentation.
- **Package.zip:** installer, sources, guide, licenses and Core version information together.
- **User-Guide-ES-EN-IT-FR-DE.pdf:** illustrated multilingual guide.
- **SHA256SUMS.txt:** integrity checks for all downloadable files.

## Validation and known limits

The development report records 30 passing local suites, isolated installer tests and a portable OBS 32.2.2 UI review. Current-revision real raid/Hype events and extended live-stream testing remain pending. One residual allocation appeared on shutdown with both plugins; its origin is unconfirmed. These checks do not certify leak-free or extended live operation.

Source: https://github.com/Emidj/obs-dynamic-stream-title
