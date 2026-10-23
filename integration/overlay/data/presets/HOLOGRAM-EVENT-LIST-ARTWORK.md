# Futuristic Hologram — recurso Event List 0.12.14

Modo: Imagegen integrado, generación con referencia y edición background-extraction;
transparent_background=true. Solo panel 10 de la referencia, sin el fondo.
Recurso final: [futuristic-hologram-event-list.png](assets/futuristic-hologram-event-list.png), copiado intacto del resultado.
1536×1024 RGBA, 1.809.959 bytes. SHA256 BF9E1AEB47F7D516FB06830148F801C4631FBBAB25C482A5222093F9249E96B2.
Residente 1024×683, 2.797.568 bytes, AssetManager compartido LRU ocho/ocho MiB.
Cinco tarjetas independientes, cristal cian/violeta/rosa y bordes digitales finos;
corazón, corona, diamante, regalo y grupo específicos. Sin texto precocinado.
La imagen mostrada por el generador puede enseñar RGB azul bajo alfa cero;
los píxeles de los cuatro gutters centrales tienen alfa cero. Se verifica el
alfa del archivo y el resultado nativo, no su representación en el chat.
No es una copia idéntica píxel a píxel de la referencia. Perfil compartido 5,
partículas cuadradas (forma 1), diez como máximo; no añade otro renderer.
Opacidad del interior oscuro mediante backgroundBox, sin modificar el atlas.

## Prompt de generación

Use case: style-transfer
Asset type: transparent production sprite atlas for native OBS Event List.
Input image: STYLE REFERENCE ONLY. Study ONLY panel 10 FUTURISTIC HOLOGRAM (bottom row, last column). Ignore all other styles and ALL scene backgrounds.
Primary request: Recreate ONLY the compact individual event cards of panel 10: thin translucent holographic glass, pale electric blue/cyan and violet/magenta beveled luminous rails, elegant rounded/chamfered corners, subtle double outlines, very faint segmented digital edge markings. Close to the reference proportions and material, no industrial metal or bolts, no generic neon tubing capsule.
Composition: landscape 1536 x 1024 transparent sprite sheet. Exactly FIVE independent horizontal cards aligned x=40..1496, with equal total height ~155px, row centers y=105,305,505,705,905. Generous totally transparent gutters and outer margins, no connected backdrop. Each card has clear straight dark interior until x=1460 and a THIN right endcap (under 20px) so right-aligned amounts fit.
Each card: recognizable 3D holographic icon centered at x=110,y=rowcenter, about100px tall. Small empty capsule badge at x=185..400, vertically centered, height60px. Keep blank name region x=415..1090 and blank right amount region x=1100..1455; recessed almost-black midnight-blue glass there, mostly uniform and fully readable, not bright holographic streaks behind text. Blue faint reflections confined to edges. No text anywhere.
Five rows top to bottom:
1 BITS: faceted violet/blue crystalline diamond with bright cyan rim, blue-violet rails and empty blue-violet badge.
2 FOLLOW: cyan projected glass heart, cyan/blue rails and empty cyan badge.
3 SUB: cyan glass crown with luminous points, blue/cyan rails and empty cyan badge.
4 GIFTED: dimensional pink holographic gift box with luminous bow, violet/magenta rails and empty bright pink badge.
5 RAID: three blue-violet holographic people silhouettes, blue/violet rails and empty violet badge.
Materials: transparent glass edges, precise holographic projection, luminous polished cyan facets, subtle scanline detail limited to icon/frame. Stylish and intricate like reference. Same card size and geometry, no duplicate icons, no pre-rendered names/counts/time, no title or numbers, no stars, particles outside modest rim glow, cosmic backdrop, buildings, walls, stage, checkerboard, shadows connecting rows or watermark. Real alpha transparency outside individual cards. Keep dark interiors suitable for separately adjustable background opacity.

## Prompt de extracción del fondo

Use case: background-extraction
Asset type: production transparent five-row OBS sprite atlas.
Change ONLY the background. Remove ALL blue/purple gradient, haze, fog and backdrop outside the five individual event cards, including every gutter between rows and outer margins. Those pixels must be fully alpha=0, actual transparent PNG. Keep the existing five cards, dark readable interior, five icons, colors, tiny edge glow, empty badges and their positions unchanged. Keep 1536x1024 canvas. No checkerboard baked in, no new text, no new elements. Restrict glow to a narrow 3-pixel rim of each card; absolutely no blue haze connecting cards. Preserve opaque dark interiors inside cards and preserve bright crystalline icons.
