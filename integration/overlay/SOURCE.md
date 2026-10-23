# Copia de integración de OBS Twitch Overlays

Origen: C:/NEXUS-Dependencies/OBS Twitch alerts/OBS Twitch Overlays, versión base 0.14.7. Esta copia se compila como 0.14.9-core1.1; no es una actualización instalada de Overlay.

Se copian src, common, tests, catálogos, presets/recursos y branding necesarios para compilar y ejecutar la regresión. La fuente original queda intacta. No se copian configuraciones, sesiones Twitch, credenciales ni binarios instalados.

Cambios respecto al origen:
- src/twitch/shared-provider.hpp/cpp: proveedor de la ABI C común.
- src/twitch/service.hpp/cpp: callback compartido adicional, entrega de eventos después de aplicar batches y lectura de información del canal sobre el transporte existente.
- src/plugin-main.cpp: publicar/inutilizar la interfaz en el ciclo de vida existente.
- tests/twitch-service-tests.cpp: dos consumidores, lectura Helix y eventos duplicados sobre un único socket local de prueba.
- CMakeLists.txt: construcción dentro del proyecto de integración, sin el bootstrap del repositorio original; conserva sus 25 suites CTest.

La cabecera común está en ../../shared/twitch-core-api.h. La prueba nueva de ciclo de vida y el cliente del título están en el proyecto padre. La DLL Title no enlaza el motor Twitch ni crea un segundo Service.

La copia conserva la licencia GPL-2.0-or-later de Overlay. Los presets y el branding se conservan como recursos del proyecto original.

## Adaptación de fase 4

shared-provider se compila ahora dentro de nexus-twitch-core.dll junto con shared-runtime.hpp/cpp. El runtime retiene una única Service y su Core; el último consumidor los detiene. Overlay usa retainRuntime/release en lugar de construir y detener una Service privada. Title accede por ABI C 2 y puede iniciar el mismo runtime sin el módulo Overlay. Se añade autorización opcional channel:manage:broadcast, estado de permisos, URL de autorización y desconexión compartida. La ruta de sesión común es nexus-twitch-core/twitch-session.bin; no se migran credenciales originales. Los cambios y pruebas se limitan a esta copia.

## Adaptación de fase 5

ABI 3 añade Search Categories y Modify Channel Information con validación de los tres campos admitidos. Service reutiliza el transporte para búsqueda GET y publicación PATCH autenticadas. Transport admite PATCH como único método explícito adicional. Se amplían las pruebas con servidor local: payload, permisos, JSON inválido, error sin reintento y envío único. No se copian sesiones ni credenciales a los artefactos.

## Adaptación de fases 7–9

ABI 4 transporta ID/login del raider y añade PATCH exclusivo del título; RaidEvent conserva el login validado sin modificar el nombre visible de Overlay. ABI 5 transporta ID y fecha de inicio de Hype Train. ABI 6 añade GET previo a PATCH y respuesta 409 ante cambios externos. Se reutilizan Service, transporte y EventSub existentes. Las pruebas cubren metadatos y ausencia de conexiones duplicadas. No se modifica el repositorio original ni se distribuyen credenciales.


## Twitch Core 1.0.0

Se incorpora versión independiente de Core, VERSIONINFO Windows, recurso PE inerte de contrato para Core y consumidores y exportación de identidad/ruta cargada. Los rangos ABI/versión de cada consumidor se generan a partir de su contrato JSON y se verifican antes de retener el runtime. Ambos paneles muestran versión/ABI y ruta en tooltip. El contrato actual es ABI 6, >=1.0.0, <2.0.0. El instalador migra únicamente hashes antiguos reconocidos; las versiones con contrato se comprueban por rango y ABI.

Core 1.1.0 añade verificación de directo opcional por petición, sin modificar estructuras C ni layout de Service/Endpoints. La copia de Overlay conserva el contrato >=1.0.0 ABI 6; no obliga a reemplazar un Overlay versionado compatible.
