# Hype Train Meter · 0.12.35

Referencia del usuario: `Imagen de ChatGPT 27 sept 2026, 17_30_07.png`.
Orden: Neon DJ/Cyberpunk, Retro CRT/Space, Horror/Love, Graffiti/Minimal,
Urban City/Futuristic Hologram. Dos columnas, cinco filas.

`assets/hype-reference-atlas.png` conserva la composición, las carcasas,
ornamentos e iconos y los títulos HYPE TRAIN. Los nombres, cifras y etiquetas
pequeñas se eliminaron para dibujarlos con datos reales. La barra se dejó vacía
para aplicar un relleno nativo. `assets/hype-thanks-atlas.png` contiene los
agradecimientos con las tipografías gráficas de los mismos diseños.

Ediciones realizadas con imagegen integrado. Prompts reproducibles en
`docs/design/hype-artwork-prompts.json`. Los recursos son artes basadas en la
referencia, no una garantía de identidad píxel por píxel con la ilustración.

Cada preset define el recorte del atlas y las zonas de título, barra, nivel,
porcentaje, reloj y contribuyentes en milésimas. AssetManager decodifica y
cachea solo el recorte solicitado, sin reducir todo el atlas a un único recurso
de baja resolución. VisualEngine comparte las capas idénticas entre instancias.

Los títulos gráficos heredados son planos separados. Al personalizar la
tipografía o el color de las etiquetas se usa texto nativo. El renderer común
produce revelado del contorno, luz interna, partículas acotadas, llenado,
recogida y morphing. El modo de medidor es dato del preset/fuente: fijo,
semáforo o animado. No hay vídeos ni un renderer separado por estilo.

Glitch de fósforo (CRT), descarga roja (Horror), ondas rosas (Love), pintura
(Graffiti), neón ámbar (Urban City), energía orbital (Space), circuitos
(Cyberpunk), barrido proyectado (Hologram), luz cian/magenta (Neon DJ) y brillo
discreto (Minimal). Los diez comparten el mismo motor y límites de partículas.
