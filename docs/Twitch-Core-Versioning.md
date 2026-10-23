# Twitch Core: versión y compatibilidad

Primera entrega identificada: **Twitch Core 1.0.0 · ABI 6**. Windows x64, OBS 32.x, Qt 6.8.3. Es independiente de Dynamic Stream Title 0.10.0 y Overlay 0.14.8-core1. Las pruebas de eventos reales y convivencia prolongada siguen pendientes.

El archivo fuente de la versión es `shared/twitch-core-version.json`. CMake genera la cabecera y el recurso de versión de Windows de la DLL. Cada consumidor lleva un contrato JSON dentro de su propia DLL (recurso RCDATA `NEXUS_TWITCH_CONTRACT`): identificador, ABI y rango de versión mínimo inclusivo/máximo exclusivo. En esta entrega ambos consumidores requieren ABI 6 y `>=1.0.0, <2.0.0`.

La versión identifica la entrega. La ABI identifica el contrato binario, incluidas las interfaces C++ compartidas por Overlay, sus clases, Qt y compilador; no basta con conservar la estructura C si cambia una de esas dependencias. El hash SHA-256 identifica el archivo y comprueba integridad; no sustituye la comprobación de compatibilidad.

## Política de versiones

- PATCH: corrección compatible con todos los consumidores admitidos, sin cambiar interfaces ni disposición binaria.
- MINOR: capacidades adicionales compatibles, preservando contratos y comportamiento existente.
- MAJOR y nueva ABI cuando se rompe el contrato binario o se elimina una capacidad requerida. Un cambio de Qt/toolchain o clases C++ requiere revisión explícita de la ABI.
- No republicar contenidos diferentes con el mismo número de versión. El instalador rechaza esa ambigüedad.
- Actualizar el contrato de cada plugin cuando cambie su mínimo o su máximo compatible. Probar todas las combinaciones que se declaren admitidas antes de distribuirlas.

## Instalación y actualización

El instalador lee recursos de las DLL como datos, sin ejecutar código ni cargar sus dependencias. Comprueba el consumidor incluido, los plugins presentes y los consumidores registrados. Conserva un Core posterior compatible y registra su versión/hash reales. Rechaza incompatibilidades indicando el consumidor y el rango requerido. Solo actualiza un Core anterior identificado y administrado tras validar todos los consumidores. Los binarios anteriores al versionado solo se migran mediante hashes explícitamente reconocidos.

Una declaración de compatibilidad es un contrato del distribuidor; no es una firma digital ni una defensa frente a archivos locales maliciosos. El instalador no sustituye un Core desconocido ni elimina copias ajenas. No se copian sesiones ni credenciales al paquete.

Las escrituras de Overlay/Core/Title y manifiestos son transaccionales con backups y rollback. Se conservan configuración, escenas, presets y credenciales. Al eliminar Title se conserva el Core si Overlay u otro consumidor lo utiliza. Los registros antiguos desconocidos bloquean una actualización hasta identificar sus requisitos.

## Identificar el Core cargado

Ambos paneles muestran `Twitch Core 1.0.0 · ABI 6`. El texto de ayuda de esa etiqueta muestra la ruta de la DLL realmente cargada. El registro de OBS incluye versión, ABI y ruta para cada consumidor, permitiendo detectar copias distintas. No se obtiene la versión a partir del nombre de Title o Overlay ni de un archivo vecino.

`data/nexus-twitch-core/runtime.json` registra Version, Abi, Hash, consumidores y sus requisitos. El paquete incluye `Twitch-Core-1.0.0.json` con el hash de la entrega y los contratos declarados.

## Validación

30 suites nativas, pruebas de versión/ruta en ejecución, comprobaciones del instalador con fixtures PE sintéticos (solo datos; no se ejecutan): conservación de versión posterior compatible, rechazo de ABI/major incompatibles, rango inválido, conflicto de integridad, actualización de Core anterior administrado, migración de Overlay y rollback. La prueba visual en OBS portable confirma que ambos consumidores cargan el mismo archivo.


## Revisión 1.1.0 — Dynamic Stream Title 1.0.0

Extensión compatible de guarded_update: require_live=true comprueba Get Streams para el mismo canal antes de comparar y publicar un título automático. Canal offline retorna 412 interno sin PATCH; respuesta inválida retorna 502 y un fallo HTTP se propaga sin publicar. Sin consultas periódicas ni suscripciones nuevas. Las peticiones antiguas sin require_live mantienen su semántica. No se modifica NexusTwitchApi ni el layout de Service/Endpoints; se conserva ABI 6. Title 1.0.0 declara mínimo 1.1.0 para impedir que un Core antiguo ignore la condición. Overlay conserva su rango >=1.0.0, <2.0.0.

Un Core registrado y verificado por hash puede actualizarse tras validar todos los consumidores, aunque su bandera OwnedByTitle sea falsa; esa bandera controla la eliminación, no la compatibilidad. No se reemplazan copias sin registro ni de integridad dudosa.

Referencia primaria: https://dev.twitch.tv/docs/api/reference/#get-streams
