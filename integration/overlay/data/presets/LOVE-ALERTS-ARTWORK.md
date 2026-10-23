# Animated Alerts · Love — 0.12.21

Referencia: sexta columna del diseño 27/09/2026 16:14:17: marco de cristal
violeta/magenta, perfil circular, títulos blancos/rosa e icono a la derecha.
El fondo de escena no forma parte de la fuente. Love añade corazones de perla,
cintas curvas y reflejos en movimiento a ese marco.

energyPattern=5 del programa GPU compartido: cristal biselado que abre su
anchura con la entrada y la recoge con la salida inversa; reflejos, bordes
violeta/rosa, seis corazones que laten alrededor, cintas y aro con gemas.
Partículas de corazón/destello calculadas en GPU. El reloj conserva el estado
de espera y se invierte, sin reiniciar fases ni deformar los componentes.

Seis clips independientes 48 frames/24 fps/2 s, hojas RGBA 1024×768:
corazón de facetas con doble latido; corona de corazones y perlas; regalo de
satén con tapa/lazos articulados y corazones; pack con tres aperturas propias;
figuras de comunidad con brazos que saludan; cristal rosa con facetas que
giran y perlas orbitales. Solo la hoja del icono activo se carga bajo demanda.
GIF/APNG de revisión no se reproducen ni decodifican por frame en OBS.

1000×1000 transparente incluyendo glow y partículas. Foto circular centrada
en (500,315), obtenida antes del inicio; reserva rosa si falla o es anónimo.
Título contenido, nombre en placa centrada medida una vez de 192–480 px;
desplazamiento ping-pong para nombres largos. Dos FONT nativos con herencia,
entrada/espera/salida separadas, TEST privado con login. Sin audio, conexión,
cola, renderer o timer nuevos. Los cinco estilos anteriores se conservan.
