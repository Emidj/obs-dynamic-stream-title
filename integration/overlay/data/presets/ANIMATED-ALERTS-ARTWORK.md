# Animated Alerts · Neon DJ · 0.12.16

## Revisión de movimiento 03/10/2026

El plasma, el aro, filamentos y partículas ahora son geometría/procedimientos
del shader GPU compartido; los PNG de plasma/aro descritos abajo son arte
histórico y no se cargan en Animated Alerts. El retrato tiene centro fijo.
Petición final: reproducir la entrada al revés sin variar las formas.
Las capas invierten los gestos de entrada. Iconos y procedimientos GPU
retroceden desde el reloj de espera actual, escalados por la duración de
salida, para evitar un salto de fase. Brillo/escala enlazan el primer 12 %.
La duración de salida sigue escalando el gesto completo; no máscara de píxeles.
Se conserva el atlas antiguo como reserva de iconos para presets sin clips.

Tras la petición de GIF separados, las seis animaciones se dibujan como
geometría vectorial/3D en `scripts/Generate-AlertIcons.py`, horneadas una sola
vez en hojas RGBA 1024×768: 8×6 celdas de 128 px, 24 fps, 48 frames/2 segundos.
Corazón facetado con doble latido y corazones pequeños; corona con perspectiva
y joya; regalo con caras, tapa articulada y lazo; tres regalos desfasados;
figuras Raid con brazos independientes; octaedro giratorio con iluminación.
No se editan los PNG de Imagegen. No se deforman sus UV durante el playback.
Los seis clips nuevos son datos `iconAnimations` y comparten AssetManager,
VisualEngine y caché GPU existentes. Solo se carga el icono del evento activo.
Los GIF de revisión incluyen damero por la limitación de alpha de GIF; la
composición usa los mismos frames con alpha completo, sin fondo ni damero.

Generación reproducible:
`Generate-AlertIcons.py --assets data/presets/assets --preview docs/validation/phase-12/animated-icons-0.12.16`.
La placa tiene ancho automático 192–480 px y altura 86; siempre centrada,
nombre corto centrado, largo con ping-pong; título/cantidad
reducidos. Entrada/espera/salida siguen independientes y la fuente mide 1000².

## Procedencia del arte anterior

Primer checkpoint incremental de esta fuente; los otros estilos se incorporarán después.
Referencias del usuario: 27 septiembre 2026 16:14:17 (diseños), 16:35:46
(entrada/espera/salida) y 16:19:48 (seis variantes Neon DJ).

Arte generado con la herramienta integrada Imagegen, modo transparente,
referencia 16:19:48, seguida de dos ediciones para separar las capas. Archivo copiado intacto a `assets/neon-dj-alerts.png`.
El resultado nativo es un atlas cuadrado de 1254×1254; el gestor lo decodifica
una vez con residencia máxima 1024×1024. La fuente y todos los fotogramas
exportados por OBS son exactamente 1000×1000, transparentes y reescalables.
No se ha recortado, repintado ni redimensionado el PNG de producción con scripts.

Componentes: explosión de plasma, aro, corazón, corona, regalo individual,
pack, grupo Raid, diamante, estela y fragmentos. El texto y el retrato real
son capas dinámicas; no están pintados en el atlas. Dos selectores FONT nativos
con herencia independiente. Solo se prepara el evento activo.

## Prompt final (herramienta integrada)

Edit the supplied transparent sprite atlas. Keep the same spectacular neon artwork, same 2 large quadrants on top and eight small square cells below. CRITICAL: independently shrink EACH of the ten sprites to 70 percent of its current size around the centre OF ITS OWN GRID CELL. Preserve the full shape and glow, do not cut the glow. This leaves a LARGE EMPTY FULLY TRANSPARENT gutter around all four sides of every cell. A thin black-looking gap is not enough: require 15 percent of each cell width totally empty on every side, no particles or smoke there. Top-left burst in cell x0-50%, y0-50%; top-right ring in cell x50-100%, y0-50%. Lower cells each 25% width and25% height. Third row heart, crown, one gift, gift pack; fourth row raid group, diamond, speed streak, shards. Keep every component and its glow completely inside its own cell. Ring inner hole fully transparent. Do not introduce text, face, background or checkerboard. Output a square transparent PNG. The sprites can look substantially smaller in this atlas; our renderer will enlarge them later.

## Movimiento nativo

Entrada de aro/retrato, expansión de plasma, estelas contrapuestas y revelado
escalonado del texto. Durante la espera: respiración, microgiro y partículas
del evento (corazones, coronas, regalos, grupos o diamantes). Salida: texto
primero, dispersión y apagado exacto. Tres duraciones independientes.
La foto se obtiene por ID desde Get Users y se descarga sin credenciales
en el CDN de Twitch a través del transporte compartido. La animación espera
la respuesta y la preparación de la foto; fallo o anonimato usan reserva.
La caché de imágenes es única, limitada a ocho entradas/8 MiB.


## Capas aisladas finales

El aro y el plasma se generaron después como PNG individuales transparentes,
para evitar que el atlas mezclase brillos de componentes vecinos. Archivos
intactos: assets/neon-dj-alert-burst.png y assets/neon-dj-alert-ring.png.
Se cargan mediante datos burstImage/ringImage en el mismo AssetManager; no
crean otro renderer. Los prompts completos están en alerts-burst-prompt.txt
y alerts-ring-prompt.txt. El atlas final conserva iconos y partículas.

