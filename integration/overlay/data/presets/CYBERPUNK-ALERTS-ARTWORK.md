# Animated Alerts · Cyberpunk · 0.12.17

Referencia: segunda columna de `Imagen de ChatGPT 27 sept 2026, 16_14_17.png`.
Bandas diagonales blancas con texto negro, recortes negros, acento violeta y
detalles cian. Sin fondo de escena. Retrato real y textos siguen dinámicos.

energyPattern=1 selecciona procedimientos del único programa GPU compartido:
placas angulares que se despliegan, líneas de circuito con pulsos viajeros,
escáner, arcos segmentados/hexágono alrededor del retrato y fragmentos
triangulares giratorios. Dos bandas blancas diagonales se revelan y retraen
con el título. Las superficies y sus brillos evolucionan internamente.
No PNG estático de composición, vídeo ni nuevo reproductor.

Seis clips RGBA propios, generados localmente con geometría vectorial/3D:
corazón facetado con pista de pulso; corona con perspectiva y módulos luminosos;
caja regalo con tapa/lazo y prismas; tres cajas desfasadas; figuras hexagonales
Raid con brazos articulados; cristal giratorio con órbita de corchetes cian.
`Generate-AlertIcons.py --style cyberpunk --assets data/presets/assets
--preview docs/validation/phase-12/animated-icons-0.12.17/cyberpunk`.
48 frames/24 fps/2 s, una hoja 1024×768/3 MiB por icono activo. Los GIF son
revisión con damero; producción usa alpha completo. No CPU raster/upload por
fotograma. El perfil clips-only no necesita ni carga el atlas histórico.

Composición compartida, 1000×1000 transparente, retrato centro (500,315),
nombre/placa siempre centrados en x=500 con ancho medido 192–480 px. Textos
largos con ping-pong, ambos FONT/herencias. Entrada/espera/salida persistidas
independientes, salida invierte gestos y reloj desde el estado de espera actual.
Descarga del retrato/estado/cursor/caché siguen centrales. Sin audio.
