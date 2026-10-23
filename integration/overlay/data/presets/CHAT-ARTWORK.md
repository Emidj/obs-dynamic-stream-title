# Chat artwork — ten reviewed styles

## Futuristic Hologram — 0.12.52

`assets/futuristic-hologram-chat-frame-v1.png` is a new1254×1254 RGBA bitmap
generated with built-in imagegen on2026-10-07. Column10 of the user's
`Imagen de ChatGPT 27 sept 2026, 16_59_51.png` and row10 of
`Imagen de ChatGPT 27 sept 2026, 17_01_22.png` are design references only.
Narrow projected blue glass rails, cyan highlights, violet/magenta glints,
chamfered corners and short projection strips. Center/exterior alpha zero.
No city, scene backdrop, message, badge or emote is baked in. This is an
adaptation, not an identical extraction. Original generated file:
`C:/NEXUS-Development/.codex/generated_images/01a0e3c7-8ae7-74a0-8516-1adaaf3b5751/exec-96ae77f3-ba27-48db-b77c-b7befa01f270.png`.

Shared nine-slice: normalized crop1000², caps200/200, corner56 with uniform
aspect, insets28/18, padding8/gap3, Bahnschrift SemiCondensed. Individual
rounded blue glass cells use CardNode's shared gradient/glow. The data-driven
`holographic` option adds three cached PathNodes for cyan glints and magenta
projection lines, confined to an outer6px band. It defaults to false; only
this chat preset enables it. `fillBehindRows=false` keeps adjustable cell
opacity independent of the border without a second opaque backing.
Frame OFF preserves cell details, full messages, inline emotes and font,
size/color selection/inheritance. No new transport, audio, renderer per style,
or per-frame raster/uploads. All ten chat references have been adapted;
badges, moderation and special layers remain separate checkpoints.

Original generation prompt: [futuristic-hologram-chat-frame-v1-prompt.txt](futuristic-hologram-chat-frame-v1-prompt.txt).

## Urban City — 0.12.51

`assets/urban-city-chat-frame-v1.png` is a new1254×1254 RGBA bitmap generated
with built-in imagegen on2026-10-07. Column09 of the user's
`Imagen de ChatGPT 27 sept 2026, 16_59_51.png` is a design reference only.
Narrow charcoal steel rails, worn copper/brass trim, small mechanical
brackets and screws, amber neon and a caged filament lamp at the top right.
Center and exterior alpha are zero. No wall/street/scene backdrop, message,
badge or emote is baked into the asset. Adaptation, not an identical extraction.
Original generated file:
`C:/NEXUS-Development/.codex/generated_images/01a0e3c7-8ae7-74a0-8516-1adaaf3b5751/exec-c79e762a-eab7-4c07-b7fb-b6ec2a9fd62b.png`.

Shared nine-slice path: normalized crop1000², caps260/260, corner72 with
uniform aspect, insets40/20, padding8/gap3, Bahnschrift SemiCondensed.
Dark individual cells have cut corners, copper contours and recessed rails
with slotted fasteners. The data-driven `riveted` option prepares two cached
PathNodes per cell in an outer6px band. It defaults to false; no other
preset enables it. `fillBehindRows=false` avoids a second opaque backing.
Frame OFF preserves cell details, opacity, complete messages, inline emotes,
font/size/color selection and inheritance. No renderer per style, new
connection, audio, or raster/upload work during animation frames.

Original generation prompt: [urban-city-chat-frame-v1-prompt.txt](urban-city-chat-frame-v1-prompt.txt).

## Minimal — 0.12.50

La octava referencia no lleva marco exterior decorativo. Sus celdas se
construyen con CardNode del motor compartido, sin PNG nuevo ni recurso
generado. Bordes azul grisáceo, radio16, padding8/gap3, fondo casi negro
y Bahnschrift SemiCondensed. Opacidad independiente del contorno.
fillBehindRows=false; marco ON/OFF produce las mismas celdas y contenido.
Adaptación de la referencia, no extracción idéntica. Sin fondo fotográfico.

## Graffiti — 0.12.49

`assets/graffiti-chat-frame-v1.png` is a new1254×1254 RGBA bitmap generated
with built-in imagegen on2026-10-07. Column07 of the user's
`Imagen de ChatGPT 27 sept 2026, 16_59_51.png` is a design reference only.
Rough cyan/magenta/yellow brush rails, paint splashes, drips and spray flecks
surround a transparent opening. Center and exterior have alpha zero; no
wall or scene backdrop, text, photo, badge or emote is baked in.
This is an adaptation, not an identical extraction. Original generated file:
`C:/NEXUS-Development/.codex/generated_images/01a0e3c7-8ae7-74a0-8516-1adaaf3b5751/exec-9eca96b4-4877-42da-83d9-da6aaf45c7e5.png`.

Shared nine-slice path: normalized crop1000², caps260/260, corner96 with
uniform aspect, insets48/22, padding8/gap3. Rounded dark cells use the shared
data-driven `painted` option: two cached vector paths for brush chips and
spray dots confined to a6px edge band. Colors come from edge/accent; live
messages and emotes remain unobstructed. The option defaults to false.
`fillBehindRows=false` preserves adjustable cell opacity without a second
dark base. Frame OFF retains painted cells, fonts, sizes, colors, inheritance
and complete content. No new transport, audio or renderer per style.

Original generation prompt: [graffiti-chat-frame-v1-prompt.txt](graffiti-chat-frame-v1-prompt.txt).

## Love — 0.12.48

`assets/love-chat-frame-v1.png` is a new1254×1254 RGBA bitmap generated
with built-in imagegen on2026-10-07. Column06 of the user's
`Imagen de ChatGPT 27 sept 2026, 16_59_51.png` is a design reference only.
Glossy crimson/rose hearts cluster in the corners, with narrow pink rails
and sharp white highlights. Center and exterior are transparent. No scenic
backdrop, text, photo, badge or emote is printed in the asset. This is an
adaptation, not an identical extraction. Original generated file:
`C:/NEXUS-Development/.codex/generated_images/01a0e3c7-8ae7-74a0-8516-1adaaf3b5751/exec-a08a23e9-b6c5-4ce8-bb0d-f3ef65ff194c.png`.

The existing nine-slice path uses caps300/300 per1000, corner112 with uniform
aspect, insets56/20, padding8/gap3. Rounded individual cells retain their
pink contour at zero fill opacity. `fillBehindRows=false` prevents a second
black base underneath them. The option defaults to true, preserving other
styles; Horror retains its existing separate-cell behavior. Frame OFF hides
only the outer artwork. Fonts, sizes, colors, inheritance, complete messages
and inline emotes remain live. Resources are cached and drawn by the existing
shared renderer. No new connection or audio.

Original generation prompt: [love-chat-frame-v1-prompt.txt](love-chat-frame-v1-prompt.txt).

## Horror — 0.12.47

`assets/horror-chat-frame-v1.png` is a new 1254×1254 RGBA bitmap generated
with built-in imagegen on 2026-10-07. Column 05 of the user's
`Imagen de ChatGPT 27 sept 2026, 16_59_51.png` is a design reference only.
Charred angular fragments and scarlet fissures hug the outer contour;
center and exterior are transparent. No wall, scene backdrop, text, badge,
photo or emote is baked in. This is an adaptation, not an identical extraction.
Original generated file:
`C:/NEXUS-Development/.codex/generated_images/01a0e3c7-8ae7-74a0-8516-1adaaf3b5751/exec-0b7e329c-0bf2-4f88-90b3-4492e46c85eb.png`.

The shared nine-slice path uses caps 160/160 per 1000, corner 40 with uniform
aspect, insets 20/14, padding 8 and gap 3. Individual dark cells use the
data-driven `fractured` option: an irregular boundary and branching edge
fissures prepared as three cached PathNodes, adapting to message dimensions.
The outer frame has no shared dark fill, so cell opacity is not compounded.
Frame OFF retains cell contours, text and emotes. Fonts, sizes, colors and
inheritance remain live. No new connection, audio or renderer per style.

Original generation prompt: [horror-chat-frame-v1-prompt.txt](horror-chat-frame-v1-prompt.txt).

## Space — 0.12.46

`assets/space-chat-frame-v1.png` is a new 1254×1254 RGBA bitmap generated
with the built-in imagegen tool on 2026-10-07. The fourth column of the user's
`Imagen de ChatGPT 27 sept 2026, 16_59_51.png` is a design reference only.
Blue/cyan/violet orbital rails, small stars and corner planets surround a
transparent opening. Center and exterior have alpha zero; no photographic
backdrop, text, photos or emotes are baked in. This is an adaptation, not an
identical extraction. Original generated file:
`C:/NEXUS-Development/.codex/generated_images/01a0e3c7-8ae7-74a0-8516-1adaaf3b5751/exec-95639cf0-ef11-43f9-8738-3fd2729f8011.png`.

The shared nine-slice path uses caps 240/240 per 1000, destination corner 100,
insets 64/20, padding 8 and gap 3. `preserveCornerAspect` uses one uniform
scale for corner slices, retaining round planets in the compact horizontal
bar. It is disabled by default for other styles. A common dark blue panel
supplies adjustable opacity; rows have no individual cards. Frame OFF hides
only the outer artwork. Native fonts, sizes, colors and inheritance remain.
Preparation is cached and the existing renderer draws immutable textures;
no new connection, renderer per style or audio.

Original generation prompt: [space-chat-frame-v1-prompt.txt](space-chat-frame-v1-prompt.txt).

## Retro CRT — 0.12.45

`assets/retro-crt-chat-frame-v1.png` is a new 1254×1254 RGBA bitmap generated
with the built-in imagegen tool on 2026-10-07. The third column of the user's
`Imagen de ChatGPT 27 sept 2026, 16_59_51.png` is a design reference only.
The worn bronze housing, recessed dark rim, screws and grooves are authored
in the asset; both its center and exterior are transparent. No backdrop,
text, photos or emotes are baked in. This is an adaptation, not an extraction.

The shared nine-slice path uses caps 160/240 per 1000 and destination corners
48 px (limited to one third of the canvas in compact layouts). Insets 28/20,
padding 8 and gap 3 keep live content within the screen. No individual row
cards. The style uses Bahnschrift SemiCondensed unless custom fonts are chosen.
Existing fonts, sizes, colors, opacity and frame visibility controls remain.

The shared scanlines option prepares fine green phosphor lines once on the
common glass surface behind text and emotes. SourceAtop composition preserves
the requested background alpha, including zero opacity. Frame OFF preserves
this screen while hiding the bezel. Immutable textures are reused after
warm-up; no shader or renderer per style, no audio or new Twitch connection.

Original generation prompt: [retro-crt-chat-frame-v1-prompt.txt](retro-crt-chat-frame-v1-prompt.txt).

## Cyberpunk — 0.12.44

The Cyberpunk chat uses cached vector geometry, not a generated bitmap. This
matches the fine technical rails, cut corners, dark cells, cyan traces and
red accent marks of column 02 in the user's 2026-09-27 chat reference.
It replaces the reused supporter frame; no scene backdrop is included.
Registry selects the shared `circuit` decoration and `accent` color, with
insets 12/8, padding 8, gap 3 and bevel 9 px. Geometry adapts to message width
and wrapped height. Two paths hold all trace marks; preparation is cached on
GUI changes and OBS draws the immutable result. Other styles keep their
existing paths. The outer frame can be hidden while retaining the cells.
This is an adaptation, not an identical image extraction.

## Neon DJ — 0.12.43

`assets/neon-dj-chat-frame-v1.png` is a new RGBA bitmap generated with the
built-in image generation tool on 2026-10-07. Reference: the Neon DJ panel at
the top left of `Imagen de ChatGPT 27 sept 2026, 16_59_51.png` supplied by the
user. No scene backdrop, headings, sample names/messages or headphones are
baked into the asset. The center is transparent. This is a close adaptation
of the electrical outline, not an identical extraction from the reference.

The shared chat renderer uses nine slices: source caps 180/1000 on both axes,
destination corners 28 px. Interior insets 22/14 px reserve space for live
content. One dark panel supplies the adjustable background; rows have no
individual cards. Padding 8 px and row gap 4 px produce compact chat lines.
Custom fonts, sizes and colors remain live; no text or emote is baked into art.
Only visible sources load assets. Preparation runs on GUI changes; OBS video
draws immutable cached textures and existing animated emote frames.

The shared Show style frame checkbox hides the outer atlas without removing
messages, emotes or the adjustable dark panel. It defaults to enabled, applies
to all ten styles and both layouts, and does not replay message entrances.

Original generation prompt:

Create a production game UI bitmap asset for native OBS Twitch chat. Reference
image at supplied path: use ONLY the Neon DJ chat design at top left, its
electric cyan/blue/violet/magenta outline, angular beveled corners, sharp white
light cores, fine lightning strands, tiny diamond shards and sparks hugging
the outer outline. Generate ONLY ONE empty square chat panel FRAME, no text,
no icons, no people, no badges, no headphones, no DJ equipment, no scene
background. This is a reusable nine-slice border texture for tall chat panels
and extremely wide chat bars: make all four edge runs straight and slim,
almost identical thickness, detailed lightning lines running along them, keep
interesting tiny ornaments clustered in the corners. Square overall
composition, centered, fully inside the image with transparent safety margin
around the exterior glow. Corner decoration should occupy roughly 8% of each
edge length. Outer border occupies the outer 4% of the square; keep 90% of
the inside COMPLETELY TRANSPARENT and empty, including no dark fill (the
program supplies the configurable dark fill separately). Alpha transparency
both outside and inside. Fine saturated cyan-electric-blue and violet-magenta
alternating energy strands, razor-sharp luminous white highlights, a rich
polished broadcast overlay look faithfully like Neon DJ reference. No pink
clouds or huge fuzzy glow; slim sharp energetic outline. No letters, logos,
typography, placeholders or checkerboard drawn into the image. Export a
square RGBA transparent frame texture.
