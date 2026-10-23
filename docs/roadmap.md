# Hoja de ruta aprobada

Fuente: once fases aportadas directamente por el usuario el 15 de octubre de 2026. Cada fase se compila, prueba y valida antes de avanzar.

Excepción autorizada el 15 de octubre: las pruebas de Raiders y Hype con eventos reales quedan pendientes hasta un próximo directo. Se permite continuar el resto con validación local y revisión visual, registrando esa limitación.

1. **Estructura del plugin.** CMake, módulo nativo, Dock, logs y carga/descarga segura. Validar acoplar, desacoplar y redimensionar dentro de OBS.
2. **Interfaz Nexus.** Base Title, Category, Tags, Automatic Events, Title Preview y Apply to Twitch; scroll vertical, conexión inferior y selector Unicode con categorías, búsqueda y recientes. Datos simulados y Dock estrecho. Mockup exacto pendiente de recuperar.
3. **Reutilización de Twitch Core.** Auditar OAuth, Helix y EventSub de OBS Twitch Overlay. Compartir instancia y ciclo de vida sin conexiones duplicadas ni regresiones en Overlay.
4. **Autenticación y conexión.** Permisos, información del canal e indicador compacto. Funcionar sin Overlay instalado.
5. **Edición y publicación manual.** Separar borrador de datos aplicados; título, categoría y tags manuales solo se publican con Apply to Twitch.
6. **Title Composer.** Base aplicada y bloques automáticos, límites, contador, prioridad y vista previa. No publicar borradores ni duplicar prefijos/sufijos.
7. **Raiders automáticos.** Sufijo Thanks Raiders con @usuarios únicos, sesión, deduplicación y espacio disponible. Pruebas simuladas y reales.
8. **Hype Train ON.** Añadir/retirar | HYPE TRAIN ON | al inicio/final, sin alterar la base y compatible con Raiders.
9. **Persistencia y protección.** Configuración mínima, recuperación, bloqueo temporal, cambios externos y reconciliación sin sobreescrituras accidentales.
10. **Optimización y pruebas.** Medir CPU, RAM, API, reconexión y estabilidad. Eventos simultáneos, sesiones largas, cierre/apertura y convivencia. Objetivo de reposo prácticamente nulo, todavía no medido; sin polling innecesario.
11. **Instalador y distribución.** Windows x64, incluir/reutilizar Core, versiones y desinstalación respetuosa. Validar instalación, actualización y convivencia.

Hitos: 1–2 visual; 3–5 funcional manual; 6–8 automatización; 9–11 distribución.

Restricciones: reutilizar Core existente; independencia de Overlay; únicos eventos automáticos Raiders y Hype Train; Dock compacto responsive; sin presets, historial, estadísticas, automatizaciones complejas ni servicios externos. Arquitectura dirigida por eventos.

Revisión 1.0.0 solicitada por el usuario: sustituye la casilla de bloqueo de la fase 9 por avisos y pausa de la operación afectada. La recuperación normal continúa automáticamente sin publicar borradores. Se añade comprobación explícita de directo antes de cada envío automático, sin polling.
