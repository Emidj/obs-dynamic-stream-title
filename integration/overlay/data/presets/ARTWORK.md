# Recursos visuales de Supporter Bar — 0.11.3

Referencia de diseño: la lámina del usuario `Imagen de ChatGPT 27 sept 2026,
17_58_33.png`, conservada en `docs/validation/phase-11/supporter-bar-original-reference.png`.
Se corrigieron los fondos genéricos de 0.11.1 tras el rechazo visual del usuario.
No reutilizar el antiguo generador procedural para sustituir estas ilustraciones.

Autoría: herramienta integrada Imagegen, edición a partir de referencia,
`transparent_background=true`. Los trece PNG se copiaron del resultado de la
herramienta sin modificar los originales. Son recreaciones, no extracciones
idénticas de la lámina. No contienen nombres, contadores ni títulos de campos.
La colocación, recorte y composición pertenecen al renderer genérico de Qt.

## Prompts y criterio de autoría

Instrucción común: producir un único marco horizontal ornamental para el estilo
indicado de la referencia; exterior transparente; centro continuo vacío para
tarjetas dinámicas; sin texto, cifras, iconos de campos, tarjetas internas,
divisores ni pestaña con el nombre del estilo. Usar detalles de la referencia,
no una banda plana de color. Conservar extremos compactos y centro utilizable.

| Recurso | Dirección visual suministrada a Imagegen |
| --- | --- |
| neon-dj-frame.png | Marco superior izquierdo: auriculares metálicos curvos, doble contorno magenta/cian, carriles oscuros biselados y reflejos eléctricos. |
| cyberpunk-frame.png | Marco superior derecho: metal negro desgastado, esquinas angulares, contornos amarillo/dorado y acentos electrónicos rojos/cian. |
| retro-crt-frame.png | Segunda fila izquierda: carcasa CRT de metal grueso, biseles, tornillos, indicadores ámbar/cian, scanlines y borde inferior violeta/cian. |
| space-frame.png | Segunda fila derecha: nebulosa azul/violeta, Saturno a la izquierda, Tierra a la derecha y pequeñas estrellas. |
| horror-frame.png | Tercera fila izquierda: piedra/metal oscuro agrietado, fisuras rojo sangre y una calavera en cada extremo. |
| love-frame.png | Tercera fila derecha: carriles rosa/rojo brillantes, corazones grandes en extremos, pequeños corazones y burbujas cercanas. |
| graffiti-frame.png | Cuarta fila izquierda: salpicaduras y goteos cian/magenta/amarillo/lima, coronas pintadas a ambos extremos. |
| minimal-frame.png | Cuarta fila derecha: placa oscura pulida y redondeada, biseles discretos, borde superior violeta e inferior cian. |
| urban-city-frame.png | Quinta fila izquierda: acero azul industrial, extremos técnicos con luces ámbar/cian y skyline bajo sobre el borde superior. |
| futuristic-hologram-frame.png | Quinta fila derecha: carriles cian/aqua luminosos, biseles y extremos ornamentales con circuitos, cristal y pequeñas líneas técnicas. |
| supporter-icons.png | Un atlas con seis iconos dimensionales brillantes, en celdas iguales: corazón magenta, estrella dorada, diamante violeta, regalo rosa, diamante cian, corona dorada. Sin fondo ni texto. |
| supporter-icons-cyan.png | Editar el atlas anterior conservando siluetas, orden y colocación; convertir todos los iconos y resplandores a cian/aqua con núcleos blancos y sombras teal. |

## Uso y límites

Trece archivos, 12.111.497 bytes comprimidos. Cada original es 2172×724 RGBA.
El AssetManager limita el archivo a 4 MiB, la entrada a 4096×1024/4 megapíxeles
y la imagen residente a 1024×1024 como máximo, preservando su proporción.
La caché LRU conserva como máximo ocho entradas/8 MiB, además de los handles
retenidos por frames activos. No se carga el catálogo completo al arrancar.

`backgroundCrop`/`iconCrop` indican cajas en milésimas del recurso completo;
`backgroundEdge` conserva los adornos laterales mientras el centro se estira.
Los seis símbolos se seleccionan por identidad del campo, no por su columna
visible: ocultar Follow no convierte el icono de Sub en un corazón.
El atlas multicolor se comparte entre ocho estilos; Hologram usa su variante
cian y Neon DJ el atlas afinado de 0.11.3. Títulos, valores, tarjetas y selección
de campos siguen dinámicos.

Galería: `docs/validation/phase-11/factory-presets-0.11.2.png`, compuesta por los
mismos frames que utiliza la fuente nativa. La presencia de la ilustración no
implica aprobación visual del usuario; comparar con la referencia al revisar.

## Afinado de Neon DJ en 0.11.3

Referencia adicional del usuario: `Imagen de ChatGPT 30 sept 2026, 20_52_37.png`,
con cinco estados de una alerta Follow. Se conserva una copia en
`docs/validation/phase-11/animation-original-reference.png`.
Modo: herramienta integrada Imagegen, edición de referencia con exterior
transparente; no se utilizó CLI ni otra API. Marco e iconos son recursos
estáticos. La animación se calcula en el programa GPU compartido del plugin,
no en un GIF o vídeo incorporado a OBS.

Prompts finales, normalizados como especificación de los dos recursos:

**neon-dj-frame.png** — Usar la lámina de cinco estados como referencia. Extraer
solo el marco exterior de la primera fila, ESTADO NORMAL: auriculares y doble
contorno eléctrico azul/magenta. Conservar fielmente las siluetas, los reflejos
y las proporciones del diseño. Eliminar las seis tarjetas internas, los iconos,
todos los títulos, nombres y contadores, y el panel explicativo de la izquierda.
Dejar una cavidad interior oscura continua para componer tarjetas dinámicas.
Un solo marco ancho centrado en un lienzo 3:1, exterior realmente transparente,
sin tablero de ajedrez, sin otros marcos y sin rediseño creativo.

**neon-dj-icons.png** — Usar los seis iconos de la primera fila normal como
referencia: corazón magenta, estrella dorada, diamante violeta, regalo magenta,
diamante cian y corona dorada, en ese orden. Atlas horizontal de seis celdas
iguales, una sola banda central en un lienzo 3:1. Conservar las siluetas suaves,
el volumen pulido y el glow del original, con suficiente separación para el
recorte por celda. Fondo realmente transparente; sin texto, tarjetas, marco,
tablero de ajedrez, objetos adicionales ni acabado de metal fotográfico o
glitter que sustituya la sencillez del diseño de referencia.

Salidas originales de Imagegen: `exec-1961aa7c-004a-44df-b3c3-587f7bb13303.png`
(marco) y `exec-b7fb7133-7adf-42f1-aa3c-22baaf2854e0.png` (atlas), copiadas sin
alterar los PNG. El marco reemplaza el anterior por petición de fidelidad del
usuario. Los recursos finales del proyecto están en:

- `C:/NEXUS-Dependencies/OBS Twitch alerts/OBS Twitch Overlays/data/presets/assets/neon-dj-frame.png`
- `C:/NEXUS-Dependencies/OBS Twitch alerts/OBS Twitch Overlays/data/presets/assets/neon-dj-icons.png`

La [galería de animaciones](../../docs/validation/phase-11/animations/index.html)
contiene capturas del renderer nativo. Las otras nueve ilustraciones mantienen
la dirección de autoría de 0.11.2; sus perfiles de movimiento se añaden como datos.

Minimal 0.12.24: shader de líneas plata/hielo, arcos y reflejos abiertos;
seis clips propios de contornos articulados generados con Generate-AlertIcons.py.
No fondo de escena, audio ni reproducción/decodificación por fotograma.

Urban City 0.12.25: bronce industrial, esquinas móviles, faroles ámbar,
rejillas y reflejos GPU. Seis clips propios de componentes articulados,
autoría Generate-AlertIcons.py. Sin fondo de escena, audio ni player nuevo.
