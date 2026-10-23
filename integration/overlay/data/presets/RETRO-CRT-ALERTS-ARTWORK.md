# Retro CRT · Animated Alerts

Referencia: tercera columna de «Imagen de ChatGPT 27 sept 2026, 16_14_17.png».
Solo la alerta transparente; ningún damero/fondo de la lámina se incorpora.

Perfil 2 del shader compartido: carcasa marrón/bronce, biseles, tornillos,
dos mandos con marcador móvil, LED, rejilla y vidrio de tubo. La pantalla tiene
barrido de fósforo, reflejos y línea de encendido. La foto real ocupa una
ventana cuadrada centrada y recibe un pase GPU de vidrio; no se sustituye por
una ilustración ni se decodifica/rasteriza durante la animación.

Seis hojas propias `crt-alert-icon-*.png`: 1024×768, celdas 128×128, 48 frames
a 24 fps/2 s. Corazón modular con latido, corona con joyas secuenciales,
regalos con tapa/lazo móviles, pack escalonado, personas celebrando y cristal
pixelado con iluminación de facetas. Se generan desde geometría articulada
en `scripts/Generate-AlertIcons.py --style retro-crt`; GIF/APNG solo revisión.

Texto horizontal contenido en el tubo, ambos FONT nativos con herencia,
nombre autoajustado y centrado, detalle debajo. Mantener los tres tiempos
independientes, salida invertida desde el estado actual y límites 1000×1000.
Un solo renderer/AssetManager/cola/Twitch, sin audio ni timers adicionales.
