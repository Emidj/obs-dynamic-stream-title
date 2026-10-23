# Event List — recursos transparentes

## Futuristic Hologram — 0.12.14

Imagegen integrado, atlas transparente de cristal cian/violeta/rosa siguiendo
solo columna 10. [Recurso, metadatos y prompts exactos](HOLOGRAM-EVENT-LIST-ARTWORK.md).
Perfil holográfico 5/forma 1/diez partículas del motor compartido.

## Urban City — 0.12.13

Imagegen integrado: generación con referencia y edición del remate derecho,
transparent_background=true. Solo tarjetas de columna 09, sin fondo.
Archivo final assets/urban-city-event-list.png, copia intacta del resultado:
1536×1024 RGBA / 2.077.046 bytes. SHA256
34E242EBE30D4A6D639D95FC953958CC16BD811C2A6D612DD530E7CA33FEFC4C.
Residente 1024×683 / 2.797.568 bytes, AssetManager compartido LRU ocho/ocho MiB.
Cinco marcos de metal oscuro desgastado, neón ámbar/azul/rojo, iconos montados
sobre metal. Etiquetas luminosas dinámicas sin pastilla, cantidades dentro del
panel oscuro. Atlas intacto; la opacidad se aplica al preparar la fila cacheada,
con máscara interior como datos del preset. Perfil urbano existente 7/forma 1,
diez chispas como máximo. No es extracción idéntica píxel a píxel.

### Prompt de generación Urban City

Use case: style-transfer
Asset type: transparent production sprite atlas for native OBS Event List.
Input image: STYLE REFERENCE ONLY. Study ONLY panel 09 URBAN CITY (bottom row, fourth column). Ignore all other styles and all scene backgrounds.
Primary request: Recreate the five compact dark industrial event cards from panel 09 with closely matching proportions, worn charcoal/black metal rail frames, small bolts, squared/chamfered corners, thin amber edge lamps and recessed nearly black interiors. Not a generic sci-fi neon capsule. Refined gritty urban metal, like the actual reference.
Composition: landscape sheet 1536 x 1024. Exactly FIVE independent horizontal cards stacked from top to bottom, each extends x=40 to x=1496 (95% sheet width), total heights around 170px. Row centers y=105,305,505,705,905. Completely transparent generous gutters (~30px) and outer margins. All cards identical dimensions and aligned; no connecting structures, panel background, street, walls, cables, lantern above or title.
Each row: large detailed luminous industrial icon at x=115 centered vertically. A small unobtrusive empty label area at x=190..400 (NO badge pill); empty dark username region x=410..1100; empty dark amount region x=1100..1460 all the way near the inner right edge. No text anywhere. The thin rectangular worn metal frame encloses icon and all dark regions. Ensure quantities will sit inside the straight dark interior rather than on the end cap. Black/dark face readable throughout.
Five rows:
1 BITS: small precision faceted amber diamond with thin lit edges and metal supports, orange rim/frame glints.
2 FOLLOW: amber neon-tube heart inside dark industrial mounting, warm orange edge glints.
3 SUB: icy blue electric tubular crown in a dark metal mount, small cool blue reflections on the frame.
4 GIFTED: warm amber neon gift box with structured metallic bow, amber edge lamps.
5 RAID: three red-orange illuminated stylized people silhouettes in metal mounting, ember-red accents.
Materials and finish: crisp modest 3D realism, worn black gunmetal rails, screws/rivets and small chipped patches, highlights from the icons. Detailed icon silhouettes matching reference 09, no uniform flat generic symbols. Slim border—not thick bulky copper machine housings. Cool blue SUB contrasts warm amber other cards. No animation trails or particle clouds in resting atlas.
Constraints: only isolated five event cards; no numbers, no letters, no usernames, no headings, no time elapsed, no watermark. Keep all interiors dark and empty; the runtime supplies labels, names, quantities. Real alpha transparency outside cards and between rows, never black background or fake checkerboard; preserve warm edge glow tightly around cards.

### Prompt de corrección de cantidades Urban City

Use case: precise-object-edit. Edit target is this Urban City five-card atlas. Correct ONLY the right endcap to provide dark interior under right-aligned runtime quantities. Preserve every icon, worn black metal frame, amber/blue/red lighting, row positions and sizes, no text. All five cards have excessively wide/luminous right side rails near x=1400-1470. Move and compress those endcaps into a very narrow ~20px strip at x=1460..1480. Interior must remain nearly black/dark charcoal with quiet metal texture all the way to x=1458, especially central y=35%..65% within EACH card. Remove glowing vertical lamp/reflections from x=1390..1458 and restore quiet dark panel there, retaining the straight upper/lower worn metal rails. This lets dynamic amounts finish inside the dark panel at x=1420 without colliding with ornament. Keep complete outer frame, do not crop. Keep full1536x1024 canvas; five independent separated compact industrial cards, identical positions. Preserve reference09 Urban City aesthetic, existing five amber diamond, amber heart, icy-blue crown, amber present and ember-red group icons, and all left hardware. NO labels, letters or numbers. Preserve genuine alpha0 outside card silhouettes and in all four gutters, no background, no blur backdrop or fake checkerboard. Only narrow the right endcap and remove its intrusive interior glare.


## Graffiti — 0.12.12

Imagegen integrado, generación directa desde referencia y edición de cantidades,
transparent_background=true. Se descartaron dos propuestas anteriores tras la
corrección del usuario por aspecto distinto. El resultado usado sigue solo la
columna 07 Graffiti: tiras negras, etiquetas angulares, pintura cian/magenta/
amarilla e iconos con tinta negra. No se incorpora el fondo de la lámina.
Archivo assets/graffiti-event-list.png, copia intacta del resultado final:
1536×1024 RGBA / 2.279.779 bytes.
SHA256 F5A5D78322451099FBB0C62A5B70C87E98F5DE3B08782826DC298CEAD72E8917.
Recortes explícitos contienen la cantidad sobre interior oscuro; margen 28 px.
Perfil compartido 6/gotas forma 3; doce partículas. Sin motor nuevo.
La adaptación no es una extracción idéntica píxel a píxel de la referencia.

### Prompt de referencia Graffiti final

Use case: background-extraction / precise-object-edit. EDIT TARGET is the user's ten-panel Event List design sheet. Extract and reproduce ONLY the actual GRAFFITI event-list cards in panel07 (BOTTOM ROW, SECOND COLUMN). This is a close reproduction request, NOT a redesign or a loosely inspired new style. Ignore entire other nine panels, headings, backgrounds and scenery. Deliver a production transparent atlas of FIVE original Graffiti row designs enlarged onto landscape1536x1024, stacked vertically with transparent gaps. Use these exact five card motifs from panel07, ordered: FOLLOW cyan painted HEART (use the bottommost follow heart design), SUB orange painted CROWN (third card), BITS purple faceted DIAMOND (first card), GIFTED yellow PRESENT (fourth/fifth card), RAID hot-pink THREE PEOPLE (sixth card). Reproduce the reference's asymmetrical jagged splattered paint contours, bright cyan/magenta/yellow coarse paint splotches, thick BLACK hand-inked outlines on icons, solid colored faceted cartoon shapes and SMALL nearly RECTANGULAR ANGULAR COLORED TAG at left. No thin white heart/crown/gift outlines, no glossy neon capsule shape, no soft spray-haze replacing actual discrete paint. Reference icon silhouettes and badge proportions are the priority, not new polished generic icons. Five compact black horizontal rectangular strips, each strip widthx=2%-98% and height14% of canvas, each icon sits x=3%-12%, each small colored labeltag x=15%-29%; black content region x=32%-97%, rough paint perimeter tightly outside strip edges. Spectacular energetic multicolor paint only around silhouette and icon, empty dark content inside. Remove ALL source lettering/usernames/numbers/time and redraw the surfaces beneath as flat empty dark charcoal. Leave each colored labeltag blank for dynamic runtime labels; DO NOT draw letters of any kind, no FOLLOW/SUB/BITS/GIFTED/RAID, no header. Rightmost quantities area atx=95% must stay INSIDE empty dark strip; no wide endcap or ornaments there. No background, true alpha0 everywhere outside the five actual cutout card silhouettes and between rows; no checkerboard or colored blur. Preserve actual reference's street-art row design, proportions and color blocks. Five 20%-height bands, all cards including outer ink/paint fit within their own band; no overlapping row splashes. Final asset must look like the panel07 reference cards when runtime text is added, rather than the previously generated different design.

### Prompt de cantidades Graffiti final

Use case: precise-object-edit. Edit this five-row Graffiti atlas with ONE technical correction: empty right-aligned runtime amounts need unobstructed dark interior. Keep every icon, angular label tag, paint splatter style, row layout and all colored top/bottom borders unchanged. On EVERY card remove only colored splashes that cross the dark content area in x=87%-94% of the full canvas, including the intrusive yellow paint on row4 and pink patch on row5. Replace with same flat nearly black charcoal as center display. Right border stays nearx=93%-94% but is an extremely thin ragged edge, not broad paint inside card; all paint should sit above/below the midline, not run through the runtime text zone. Region x=40%-92%, y=middle35%-65% of each card must be completely empty quiet charcoal, opaque enough for runtime text. Preserve the five distinct cyan heart/orange crown/violet diamond/yellow present/pink group icons with thick black hand-inked outlines; preserve empty ANGULAR colored tags and coarse multicolor spray paint contour from the reference. NO text, letters or numbers. Preserve1536x1024 geometry. Exterior and all spaces between five separated card silhouettes must be actual transparent alpha0, not a colored wash. No background, no colored haze or backdrop/checkerboard. Do NOT redesign cards, do NOT create round badges or new icons. Keep graffiti reference appearance, change only intrusive paint at the right amount area.

## Love — 0.12.11

Imagegen integrado, generación con referencia y tres ediciones con transparent_background=true.
Solo tarjetas Love, primera columna inferior, sin fondo. Archivo assets/love-event-list.png,
copia intacta del resultado final 1536×1024 RGBA / 1.892.478 bytes.
SHA256 1179C894F7AB88B6609CFF372AC3A0F11EAF912AE31B9869FB4ED1DBDE4AD070.
Iconos cristal/caramelo rosa-rubí específicos, badge perlado vacío y pantalla burgundy libre.
Recortes de datos ajustan la geometría; perfil compartido 0 y diez corazones forma 2.
No es extracción idéntica. La segunda edición retira alfa de fondo/huecos;
la tercera estrecha el remate derecho tras detectar solapamiento con cantidades.
Recortes de ancho 970 eliminan margen vacío exterior y dejan las cifras sobre
cristal oscuro dentro del borde. PNG final intacto, sin edición posterior.

### Prompt de generación Love

Use case: stylized-concept. Asset type: production transparent PNG sprite atlas for native OBS Event List. Input image STYLE REFERENCE ONLY: use exclusively LOVE cards in FIRST column of BOTTOM row. Ignore entire scene background and every other style. Create exactly FIVE equal-width long horizontal Love cards stacked in five separate equal-height bands on landscape 3:2 canvas, straight front view. Rich glossy rose/red glass capsule frames with hot pink beveled rim, small sculpted candy hearts hugging left edge and frame corners, elegant romantic red glow. Very dark burgundy almost black empty interior. Custom solid jewel/candy-glass event icons, not generic white outline pictograms: row1 FOLLOW a dimensional glossy pink heart with ruby glass facets; row2 SUB a delicate pearly rose-gold three-point crown with tiny pink heart jewels; row3 BITS a brilliant-cut pink ruby prism, strongly faceted crystal planes; row4 GIFTED a pearly pink glass gift box with satin crimson bow and heart-shaped knot; row5 RAID three rounded glossy rose-glass people busts joined by tiny heart, readable as three people. Saturated material fill with restrained white specular highlights. Icon contained x=3%-14%; adjacent EMPTY light rose-pink capsule badge plate x=17%-31%, bright enough for BLACK runtime labels. Empty very dark display x=34%-97%, right end unobstructed for runtime names and right-aligned amounts, any corner hearts at top/bottom border only. All cards consistent geometry x=1%-99% fitting each 20%-height band y=band+3% to band+17%, complete soft glow confined to own band with actual transparent gaps. No text, labels, lettering, numbers, usernames, elapsed time, watermark or pseudo lettering. Resting-state artwork, no explosions, no hearts floating across gaps or dark display. TRUE FULLY TRANSPARENT alpha outside five isolated card silhouettes and in four gaps; no background, scenery, haze, red backplate, overall panel, shadow across rows or checkerboard. Match polished romantic Love reference cards without its backdrop.

### Prompt de extremo derecho y transparencia

Edit target: attached five-row Love event-list sprite atlas. Two technical corrections ONLY. 1) Remove the pink/red gradient backdrop entirely. All pixels outside each of the five isolated card silhouettes including four horizontal gaps and all margins MUST be actually fully transparent alpha=0. No haze, shadow, backdrop, smoke or checkerboard between cards. Preserve red glow tightly hugging each silhouette. 2) Remove the large heart clusters from the RIGHT ends on all five cards. Extend the quiet dark burgundy text screen up to x=97%, narrow plain rounded red/pink glass endcap at x=98%; no hearts, bright ornament or icons inside x=40%-96%. Runtime amounts must fit at x=95%. Keep every LEFT-side sculpted icon and tiny surrounding hearts, pearly pink EMPTY badge plate, glossy red/pink glass capsule border, five aligned rows and exact canvas geometry unchanged. Keep all displays and badges empty: no text/letters/numbers. True transparent cutouts required. Do not alter distinctive ruby heart, rose-gold crown, pink prism, gift box and three people icons. No generic white outline pictograms.

### Prompt final de recorte transparente

BACKGROUND REMOVAL ONLY. Edit the attached five-row pink Love sprite atlas into five separate transparent cutouts. Remove ALL pink/red gradient pixels outside the five actual card silhouettes. Every pixel in the four horizontal gaps between rows, all margins, and below the last row must be alpha=0, fully transparent. No pink backdrop, haze, smoke, shadow, wallpaper or checkerboard. Preserve the cards visually exactly: left jeweled heart/crown/prism/gift/people icons with attached small hearts, pink empty label plates, dark burgundy blank screens and rounded red glossy frames. Cut around precise card/icon edges; very tight edge glow only. Do not redraw, do not change sizes or positions, do not add text. Output a genuine RGBA transparent PNG asset, not an image of transparency or a solid colored backdrop. Background extraction with FIVE isolated silhouettes.


### Prompt final de corrección de cantidades

Edit ONLY the RIGHT ENDCAP of each of the FIVE Love cards in this transparent sprite atlas. Runtime quantities drawn at x=95% currently overlap the wide bright rounded endcap, so make the endcap extremely narrow: extend the EMPTY DARK BURGUNDY interior horizontally to x=98% of the canvas. Put the thin rounded red glass end border at x=99%, no bright pink/white filled endcap at all. Region x=90%-98% on every card must be quiet almost-black burgundy like the existing center display; top and bottom pink rim continue smoothly to the thin end border, but nothing bright crosses the text line there. Preserve entire rest of artwork EXACTLY: five rows, same positions/heights, all left ruby heart/crown/prism/gift/three-people icons, decorative little hearts, empty pearly badge plates, capsule contours and canvas1536x1024. No text, no letters, numbers or new ornaments. Preserve actual fully transparent alpha=0 outside five silhouettes and in all four gaps. No scenery/backdrop/haze/checkerboard. Change rightmost5% ONLY; do not move left-side artwork or alter card heights.

## Horror — 0.12.8

Imagegen integrado, generación con referencia y `transparent_background=true`.
Solo tarjetas Horror, quinta columna superior de la referencia del usuario,
sin fondo. `assets/horror-event-list.png`, copia intacta 1536×1024 RGBA /
2.525.506 bytes; alfa exterior y cuatro separaciones centrales cero comprobado.
Corazón de porcelana agrietada, corona de hueso/plata con gemas rojas, cristal
ahumado carmesí, regalo de hueso con lazo rojo y tres calaveras marfil para Raid.
Marcos de hierro oscuro, filo rojo, badges coral vacíos y pantalla negra libre.
No es extracción idéntica. `rowCrops`/`nameStart`/`badgeBox` ajustan la geometría
sin editar el PNG. Residencia 1024×683 / 2.797.568 bytes en caché compartida,
solo visible/no vacía. Perfil Horror existente 4 y diez partículas de niebla
forma 4, sin shader/renderer/timer nuevo.

### Prompt de generación Horror

Use case: stylized-concept. Asset type: production transparent PNG atlas for native OBS Event List. Input is STYLE REFERENCE ONLY: use exclusively HORROR, fifth column in the TOP ROW. Ignore scene backdrop and every other style. Create exactly FIVE equal-width very long horizontal horror cards, stacked in five isolated bands on a landscape 3:2 canvas. Weathered charcoal wrought iron, narrow cracked dark stone frame, chipped blood-red rim and tiny thorn accents confined to edges. Black quiet empty interior. Custom material icons, NOT generic white pictograms: row1 FOLLOW a fractured ivory porcelain heart with red glowing cracks; row2 SUB a jagged antique silver/bone crown with crimson gem tips; row3 BITS an angular smoky silver crystal with deep crimson internal facets; row4 GIFTED an aged ivory bone gift box bound by dark red ribbons; row5 RAID three sculpted ivory skulls grouped together with faint red eye glow. Detailed Gothic horror finish, recognizably different icon silhouettes, clear small-scale readability. Icon centered x=9%, contained x=3%-15%. EMPTY colored label plate x=17%-31%, light coral red fill for BLACK runtime text. All five plates same size. Empty near-black text display x=34%-97% for runtime names and right-aligned amounts. Keep the RIGHT end completely free of icons, ornaments, knobs and bright marks so numbers can sit at x=95%. Cards x=1%-99%, each contained inside its own 20%-height band with thin complete glow and 3% transparent separation. Same geometry in all five rows. No lettering, usernames, labels, time, numbers, pseudo text, watermark. Resting-state cards, not explosions. Actual alpha=0 exterior, four gaps and margins: no backdrop, red fog, overall panel, wall, scenery, smoke, shadow across rows, checkerboard. Only five separate card silhouettes and restrained edge glow on genuinely transparent canvas. No graphic gore.

## Space — 0.12.7

Imagegen integrado: generación con referencia y dos ediciones, todas con
`transparent_background=true`. Solo cuarta columna superior Space de
`Imagen de ChatGPT 27 sept 2026, 16_42_30.png`, sin fondo.
Archivo `assets/space-event-list.png`, copia intacta del resultado final:
1536×1024 RGBA, 1.882.295 bytes. Alfa exterior y cuatro huecos cero comprobado.
Corazón de cristal nebuloso cian, corona cristalina azul, prisma violeta,
regalo cósmico dorado y tres astronautas magenta. Planetas y órbitas junto
al medallón; extremo derecho despejado para cantidades. Badges vacíos con
vidrio luminoso para etiquetas negras dinámicas. No es extracción idéntica.
`rowCrops`/`badgeBox`/`nameStart` adaptan geometría sin editar PNG.
Residencia 1024×683 / 2.797.568 bytes, caché compartida bajo demanda.
Perfil orbital existente 3 y doce destellos (forma 0), sin shader/timer nuevo.

### Prompt de generación Space

Use case: stylized-concept. Asset type: production transparent PNG sprite atlas for native OBS Event List. Input image is STYLE REFERENCE ONLY. Use exclusively SPACE cards in the FOURTH column of the TOP ROW. Ignore the nebula scenery, background and every other style. Create exactly FIVE equal-width long horizontal Space cards stacked in five equal-height isolated bands, straight front view, landscape 3:2 canvas. Consistent geometry: bands span 20% of height; full card silhouette including icon/orbit/glow fits x=1%..99%, y=band+3%..band+17%. Thin luminous blue/violet glass capsule frame with restrained tiny stars/orbital arcs attached to frame, deep nearly black glass screen, softly beveled rounded ends. Keep runtime text area x=33%..96% empty dark glass; no bright stars or artwork behind text. At x=5%..12% place distinctive detailed cosmic event medallions, NOT generic recolored outline pictograms: row1 FOLLOW a solid luminous ice-cyan heart formed from nebula glass inside a small dark blue planet halo with a thin orbital ring; row2 SUB a small sapphire planet with a three-point crystal crown sculpted from cosmic blue glass and tiny orbit; row3 BITS a faceted violet amethyst space crystal with multiple luminous solid glass planes and thin silver-blue elliptical orbit; row4 GIFTED an amber/gold ringed planet wrapped with a visible luminous gift ribbon/bow, rich molten-glass material; row5 RAID a magenta planetary medallion containing three small astronaut visor silhouettes formed from luminous cosmic glass, orbital trail. Icons retain same bounding size and remain recognizable at small scale, saturated body colors, restrained white highlights. Adjacent EMPTY luminous badge capsule at x=16%..30% on each row in its icon accent color, no label printed. Screen starts x=33%. Follow cyan, Sub blue, Bits violet, Gift gold, Raid magenta. Resting-state sprite artwork, no event explosions or floating particles between rows. Absolutely NO text, letters, numbers, usernames, time/age, watermark or pseudo lettering. Real fully transparent alpha outside five isolated card silhouettes and in every gap; no overall frame, background, nebula scenery, colored haze between cards, backplate or checkerboard. Match elegant blue/violet capsule and planet/orbit details of reference Space cards, without its background.

### Prompt de edición del extremo derecho Space

Use case: precise-object-edit and background-extraction. Edit target: attached five-row Space event-card atlas. Preserve all FIVE cosmic medallions and empty colored badges exactly: cyan nebula-glass heart, sapphire crystalline crown, violet faceted crystal, golden gift and three magenta astronauts. Preserve row positions and canvas dimensions. Two technical corrections ONLY. 1) Remove ALL small decorative planets at the RIGHT ends of cards. Stretch the empty black capsule interior horizontally to x=98% of canvas and put its thin rounded end border at x=99%, leaving x=38%-96% completely unobstructed dark glass for runtime names and right-aligned quantities. Any bright orbital trails at the right must hug the very top/bottom border, never cross the center text area. Left icon/badge/orbit untouched. 2) Remove the blue/purple hazy backdrop completely. Outside the five card/medallion/orbit silhouettes and in all four horizontal gaps, every background pixel must be actual transparent alpha=0. No colored hazy fill, smoke, shadow across gaps, scenery, sky or checkerboard. Five isolated transparent cutouts. Do not add text, letters, numbers or any content to the blank badges/screens. Keep existing jewel/planet art, saturation, thin frame style and quiet resting-state artwork.

### Prompt de edición de badges Space

Use case: precise-object-edit. Edit target: five-row Space atlas. Change ONLY the inner fills of the five EMPTY badge pills located at x=22%-36% of canvas. These pills will contain BLACK runtime label text, so their interiors must be light and readable: row1 bright light turquoise cyan, row2 bright icy sky blue (NOT dark royal blue), row3 luminous light lavender violet (NOT dark purple), row4 bright warm golden yellow (NOT dark amber/brown), row5 bright light pink-magenta (NOT deep maroon). Smooth mostly solid luminous light glass fills with gentle gradients, not blank dark panels. Keep badge borders, exact shapes, sizes, centers and every other pixel visually unchanged. Preserve five spectacular cosmic medallions, blue/violet capsule frames, empty DARK text screens and cleared right ends. No text, letters or numbers. Preserve actual transparent alpha=0 outside all five isolated silhouettes and in all four gaps. No scenery, overall backdrop, haze across gaps or checkerboard. Do not alter card geometry or icons. Only improve empty badge interior luminance for black text.

## Retro CRT — 0.12.6

Imagegen integrado, generación con referencia seguida de edición, ambas con
`transparent_background=true`. Solo tarjetas Retro CRT de la tercera columna
superior de `Imagen de ChatGPT 27 sept 2026, 16_42_30.png`; sin su fondo.
Archivo `assets/retro-crt-event-list.png`, copia intacta del resultado editado:
1536×1024 RGBA, 2.033.456 bytes; alfa exterior y entre tarjetas cero comprobado.
Cinco televisores con marco bronce oscuro, cristal y scanlines; iconos Follow
cian de píxeles, Sub corona azul de arcade, Bits cristal ámbar facetado, Gift
verde con lazo lima y Raid avatares de fósforo rojo. No es extracción idéntica.
Se retiraron los controles derechos para dejar libre la pantalla hasta las
cantidades alineadas a 28 px del extremo. Badge y nombres son texto dinámico.
`rowCrops`, `nameStart` y `badgeBox` adaptan el recurso sin editar el PNG.
Residencia 1024×683 / 2.797.568 bytes en AssetManager bajo demanda, visible/no
vacía. Perfil CRT ya existente `changePattern=2`, ocho partículas cuadradas
(`changeParticleShape=1`), sin shader/renderer/timer nuevo.

### Prompt de generación Retro CRT

Use case: stylized-concept. Asset type: production transparent PNG sprite atlas for native OBS Event List. Input image is STYLE REFERENCE ONLY; use exclusively RETRO CRT cards in the THIRD column of the TOP ROW. Ignore scenery/background and other styles. Create exactly FIVE equal-width long horizontal Retro CRT television cards stacked in five separate bands, straight-on front view, landscape 3:2 canvas. Same geometry on every card. Weathered dark bronze/charcoal metal TV casing, beveled warm brown frame, tiny screws, subtle analog CRT scanlines within dark glass. Small speaker vent at very left x=2%-4%; miniature dial controls ONLY at extreme right x=97%-99%. Keep the entire dark screen x=5%-96% open for content. Each card fills x=1%-99%, y=band+3%-band+17% in five 20%-height bands, with complete glow and clear transparent gaps. Top to bottom: FOLLOW glowing cyan heart, SUB glowing blue crown, BITS glowing amber faceted diamond, GIFTED glowing green wrapped gift, RAID glowing red group of people. Icon x=5%-12%, adjacent EMPTY colored badge plate x=16%-30%; empty dark screen x=33%-95% reserved for runtime names and right-aligned quantities. No text, no letters, no numbers, no usernames, no age/time, no watermark, no pseudo lettering. Avoid heavy scratches or bright decorations inside the empty text area. Real transparent alpha outside the five isolated cards; no overall frame, scenery, backplate, checkerboard or background. Resting-state sprites, not event explosions. Match the nostalgic compact television row design of the reference, not neon sci-fi cards.

### Prompt de edición del marco Retro CRT

Edit this production Retro CRT sprite atlas, preserving five cards, bronze casing, scanline glass, cyan heart/blue crown/amber diamond/green gift/red people icons and existing empty badge plates. Critical changes: remove ALL the right-hand rotary knobs. Replace them with a very narrow plain metallic endcap occupying only x=98%-99%. Extend the empty dark glass display seamlessly to x=97% on every card so right-aligned runtime amounts have unobstructed space. Exterior background between cards and outside ALL five isolated silhouettes MUST be actual fully transparent alpha, remove the existing brown hazy scenery/background completely; no checkerboard. Preserve soft glows on silhouettes, no large glow into neighboring rows. No text, no symbols beyond the existing five event icons. Same landscape 3:2 atlas geometry and five rows. Everything else unchanged.

### Prompt final de los iconos Retro CRT

Edit ONLY the FIVE left-side event icons in this Retro CRT television card atlas. User wants distinctive retro television icons, not generic white outline pictograms. Preserve all five card silhouettes, bronze metal casings, empty badge plates and unobstructed dark screens exactly in the same positions and sizes. Replace each existing icon, keeping its exact bounding area x=10%-18%, with detailed colorful unmistakably RETRO CRT material: row1 FOLLOW a luminous cyan turquoise heart made from chunky 1980s arcade pixels with glass screen curvature and phosphor RGB triad detail; row2 SUB a royal-blue three-point arcade crown, solid 8-bit geometry, metallic bevels and emissive blue pixel cells; row3 BITS a SOLID amber/orange prismatic gem with three richly colored glass facets and warm vintage phosphor highlights, not a hollow diamond outline; row4 GIFTED a solid green vintage pixel-art wrapped gift with stepped edges, green glass blocks and a luminous lime ribbon; row5 RAID a deep red trio of chunky arcade-avatar silhouettes with red CRT pixel phosphor fill, not hollow people outline. Strong saturated body color, tiny warm white specular highlights only, no mostly-white outline or generic neon vector appearance. Each symbol reads crisply at small display scale; intricate material and retro pixel geometry must be distinct. No new text, no lettering, no numbers, no usernames. No icon may overlap the empty badge at x=19%-33% or the runtime text area x=35%-96%. Keep actual transparent alpha outside all five isolated cards and in gaps; do not add scenery, background or checkerboard. Do not change rest of artwork or canvas 3:2 geometry.

### Prompt final de recorte transparente Retro CRT

Use case: background-extraction. Edit target: the attached five-row Retro CRT atlas. Remove ONLY the brown hazy background visible between and outside the five television cards. This is a technical transparent cutout for OBS. Make every pixel outside the five metal television silhouettes FULLY TRANSPARENT alpha=0, including all four horizontal gaps, the margins and the area below the final card. No hazy backdrop, smoke, colored translucent filler or shadow across gaps. Keep all pixels inside the television silhouettes visually unchanged, including their metal frames, retro pixel-art heart/crown/gem/gift/people icons, empty colored badge plates and dark blank screens. Do not redraw icons, do not change geometry, colors, dimensions, positions, cropping or spacing. Five separate precisely cut-out isolated TV cards on genuine transparent canvas. No text, watermark or checkerboard.

El ajuste de iconos introdujo alfa parcial en los huecos del atlas. La última
edición Imagegen retiró ese relleno; pruebas comprueban cuatro huecos de la
imagen residente además de la esquina. PNG final intacto, sin edición posterior.

## Cyberpunk — 0.12.4

Imagegen integrado, generación con referencia y `transparent_background=true`.
Referencia: `Imagen de ChatGPT 27 sept 2026, 16_42_30.png`, exclusivamente las
tarjetas Cyberpunk de la segunda columna superior; sin fondo de la lámina.
Archivo `assets/cyberpunk-event-list.png`, copia intacta del resultado:
1536×1024 RGBA, 1.764.120 bytes. Alfa cero comprobado en esquina y separación
entre filas. Cinco tarjetas Follow/Sub/Bits/Gift/Raid sin texto, etiquetas y
datos dinámicos. No es una extracción idéntica del original.

Recortes explícitos `rowCrops` adaptan la geometría real generada, sin editar
el PNG. Recurso residente a 1024×683/2.797.568 bytes en la caché compartida,
cargado únicamente al tener una lista visible no vacía. Perfil de animación
digital: `changePattern=1`, partículas cuadradas `changeParticleShape=1`,
doce como máximo; el programa GPU existente compone los efectos.

### Prompt final Cyberpunk

Use case: stylized-concept. Asset type: production transparent PNG sprite atlas for native OBS Event List. Input image is STYLE REFERENCE ONLY, use exclusively the CYBERPUNK cards in the second column of the TOP ROW; ignore scenery/background and all other styles. Create exactly FIVE equal-width horizontal Cyberpunk cards stacked in five separate equal-height bands. Landscape 3:2 canvas, straight front view, no perspective. All cards share same long chamfered rectangular geometry. Bands are y=0-20%,20-40%,40-60%,60-80%,80-100%; card including glow must stay inside its band, occupy x=2%..98%, y=band+3%..band+17%. Dark almost black translucent interior, thin intricate cyan angular beveled frame with technical line segments, small circuitry notches and tasteful red accents; restrained sharp digital glow matching Cyberpunk reference rather than neon fantasy ornament. Top-to-bottom order: 1 FOLLOW luminous outlined cyan heart; 2 SUB luminous hot red-pink crown; 3 BITS faceted purple-violet crystal; 4 GIFTED luminous cyan wrapped gift box; 5 RAID hot red-pink icon of three people. Each icon occupies x=4%..11%; immediately next to it an EMPTY solid luminous pill/chamfered badge panel x=14%..28%, same accent as icon. Region x=31%..94% must be completely empty dark panel reserved for runtime name, amount and age. Absolutely NO text, letters, numbers, usernames, labels, watermark or pseudo lettering. No background scene, no column frame, no HUD backplate outside cards, no checkerboard; real transparent alpha outside cards. Resting-state sprite artwork, no big sparks or explosive particles. Consistent aligned geometry, clear gaps between all five cards, glow included with no cropped ends or overlap. Match the black/cyan Cyberpunk reference with cyan/red graphic details.

## Neon DJ — 0.12.2

Herramienta integrada Imagegen, generación con referencia y
`transparent_background=true`. Referencia: `Imagen de ChatGPT 27 sept 2026,
16_42_30.png`, solo las tarjetas Neon DJ de la esquina superior izquierda.
No se incorporan los fondos de la lámina.

Archivo final: `assets/neon-dj-event-list.png`. Copia intacta del resultado de la
herramienta: 1536×1024 RGBA, 2.410.073 bytes; alfa exterior cero. Residente en la
caché existente a 1024×683 (2.797.568 bytes), solo cuando una lista visible con
eventos usa Neon DJ. Atlas Follow/Sub/Bits/Gift/Raid, sin texto; etiquetas, nombres,
cifras y edades se dibujan dinámicamente. No es una extracción idéntica del original.

El resultado no mantuvo exactamente las bandas geométricas solicitadas. Por eso
`rowCrops` define cinco recortes explícitos en milésimas, inspeccionados en el
renderer nativo, que evitan mezclar bordes vecinos. Se conservan el PNG y su alfa;
no hay edición posterior de la imagen generada. El layout usa el ImageNode genérico.

## Prompt final de generación

Use case: stylized-concept. Asset type: production transparent PNG sprite atlas for native OBS Event List. The attached image is STYLE REFERENCE ONLY: use exclusively the top-left NEON DJ event list cards, ignore all background scenery and the other nine styles. Create exactly FIVE equal-width horizontal Neon DJ event cards stacked vertically in five equal-height bands, same dimensions, straight front view, no perspective. Canvas landscape 3:2 ratio. Each band spans exactly 20 percent of canvas height; each card fills x=2% to98%, y=(band top+3%) to(band top+17%), with soft glow entirely within band. Order top to bottom: 1 FOLLOW bright cyan glowing outlined heart; 2 SUB magenta-pink luminous crown; 3 BITS purple faceted crystal; 4 GIFTED golden yellow wrapped gift; 5 RAID red-pink group of three people. In EACH card, icon occupies x=3%..10%, EMPTY luminous pill badge occupies x=12%..24%, remaining x=27%..95% is perfectly empty dark charcoal translucent panel for runtime names/amounts/time. No text whatsoever, no letters, numbers, sample usernames, labels or watermark. Ornate sharp neon electric frame matching reference, luminous beveled edges, small shards confined to frame perimeter, tasteful pink/cyan complementary highlights on purple card. Rounded/chamfered rectangular long cards. Real transparent alpha outside cards, no checkerboard, no photo background, no scene backplate. This is resting-state artwork, not a full-screen event explosion. Precise consistent geometry matters: five isolated long cards, five equally spaced horizontal bands, no cropped ends, glow included, no overlap between cards.
