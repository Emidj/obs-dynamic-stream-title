# Horror · Animated Alerts

Referencia: variante negra/roja angular de la lámina de alertas 27/09/2026
16:14:17, conservando el retrato y los datos reales; sin fondo de escena.

`alertStyle` selecciona `energyPattern=4` del mismo shader compartido:
armadura oscura fracturada, bandas angulares que despliegan y repliegan,
grietas incandescentes, aro hexagonal con espinas, humo que fluye y brasas.
No PNG de una composición completa ni otro renderer/timer por estilo.

Seis clips independientes creados por `scripts/Generate-AlertIcons.py --style horror`:
corazón facetado con latido y grietas, corona de espinas y joyas móviles,
regalo maldito con tapa articulada y sello, tres cajas en fases distintas,
tres máscaras de hueso con mandíbulas móviles y cristal rubí que gira.
Hoja transparente RGBA 1024×768, 48 celdas de 128² a 24 fps/2 s.
Solo se prepara el icono activo en AssetManager/cache; no trabajo CPU por frame.
APNG/GIF son copias de revisión; producción usa la misma hoja con alpha completo.

Foto centrada obtenida por el transporte Twitch compartido antes del inicio,
1000² transparente, tiempos separados, salida desde la pose actual al revés,
placa de nombre automática centrada/ping-pong y dos FONT con herencia.
Los perfiles de barras/listas y las cuatro alertas anteriores se conservan.
Fase 12 abierta; este checkpoint no implementa Love ni otras fuentes.
