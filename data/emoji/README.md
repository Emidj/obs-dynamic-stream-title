# Catálogo Unicode local

Unicode Emoji 17.0: 3.953 entradas únicas, incluidas las secuencias fully-qualified y los componentes. Los datos incluyen tonos de piel, secuencias combinadas y banderas; su representación depende de las fuentes de Windows.

Fuentes:
- https://www.unicode.org/Public/17.0.0/emoji/emoji-test.txt
- https://raw.githubusercontent.com/unicode-org/cldr/release-48/common/annotations/es.xml
- https://raw.githubusercontent.com/unicode-org/cldr/release-48/common/annotationsDerived/es.xml
- https://www.unicode.org/license.txt

Se conservan los archivos de origen y LICENSE.txt. `scripts/Build-EmojiCatalog.ps1` genera catalog.json con nombres oficiales en inglés y anotaciones CLDR en español para búsquedas sin distinción de tildes. Los recursos se incorporan a la DLL: el selector no consulta Internet.

Las categorías siguen los diez grupos del archivo Unicode. Recientes guarda hasta 32 selecciones únicas durante la sesión del Dock, en memoria; no persiste al reiniciar OBS.

Anotaciones CLDR release-48 para it, fr y de (common/annotations y common/annotationsDerived, unicode-org/cldr). Nombres y búsqueda en cinco idiomas, con fuentes y licencia Unicode conservadas.

