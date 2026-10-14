# ClayFighter 64 2.0 — Training Mode

Fecha: 13 de octubre de 2026. Dirección y trabajo propio: **djultra64**.
Estado: **diseño aprobado por el usuario** mediante «Hazlo», el 13 de octubre de 2026. Define el alcance de 2.0; el modo todavía no está implementado ni validado. La versión publicada sigue siendo 1.0.

## Objetivo y requisitos del usuario

Practicar el sistema de combate de esta fusión sin abandonar la pelea: seleccionar personaje y rival, ver el daño real, consultar todos los movimientos del personaje controlado y repetir Claytalities con poca espera.

Requisitos explícitos: Training Mode en el menú principal; selección de ambos personajes; combate infinito sin límite de tiempo; vida del rival que baja normalmente y se recupera después de unos segundos; Training Options en pausa; Move List completa; preset para dejar al rival al borde de la derrota y reiniciar tras morir.

Decisiones propuestas: recuperación tras 3 segundos de inactividad de combate; rival quieto por defecto; dos presets (Normal y Claytalities); reinicio rápido; medidor de daño; historial de entradas opcional; controles del dummy y de recursos. Estos valores son decisiones de diseño, no cifras copiadas de otros juegos.

## Referencias y criterio de diseño

Fuentes oficiales consultadas el 13 de octubre de 2026:

- [Capcom Fighting Collection — manual de modos](https://game.capcom.com/manual/MVCFC/en/switch/page/1/1): seleccionar personaje y rival, entrenar sin tiempo, Training Options en pausa y acción de reinicio. Es la referencia más cercana a añadir entrenamiento a un juego clásico existente.
- [Street Fighter 6 — Practice](https://game.capcom.com/manual/SF6/en/ps5/page/6/3): configuración detallada y Simple Training Settings. Adoptar presets claros para poder comenzar sin recorrer muchas opciones.
- [Tekken 8 — Practice](https://tk8.tekken-official.jp/en/mode/offline.php): lista de comandos y repetición de situaciones específicas. Adoptar consulta de movimientos y repetición rápida; sus demostraciones y guardado de estados no son requisitos de esta versión.
- [Tekken 8 — actualización 3.00](https://www.bandainamcoent.com/news/tekken-8-patch-notes-v3-00): reinicio cambiando posiciones y mejora de apertura/cierre del menú de práctica. Adoptar reinicio accesible y cambio de lado.
- [Guilty Gear Strive — modos](https://www.guiltygear.com/ggst/en/howto/mode/): entrenamiento para situaciones y combos. Mantener práctica libre y condiciones configurables.

La conclusión aplicada al proyecto es una recomendación propia: facilitar ciclos cortos de intento → observar → corregir → repetir. No reproducir íntegramente las herramientas de esos juegos.

## Alternativas

| Enfoque | Ventaja | Coste o limitación |
| --- | --- | --- |
| **Training integrado con reglas propias — recomendado** | Menús y presentación nativos; conserva combate y comandos originales; permite reinicios coherentes | Requiere verificar ciclo de vida, KO, finishers y controles del rival |
| Vida/timer forzados sobre un combate normal | Prototipo pequeño | No resuelve por sí solo victoria, dos barras, animaciones, IA ni Claytalities; insuficiente como 2.0 |
| Laboratorio avanzado con grabaciones, reproducción y estados arbitrarios | Reproduce muchas situaciones | Amplía mucho el alcance; recursos/hilos del runtime hacen que un snapshot de RAM no sea un reset fiable |

## Entrada y selección

Orden del menú principal: **Game Start / Training Mode / Options / Exit game**. Conservar la tipografía, animaciones, colores y estilo del menú original; reajustar geometría y borde para cuatro opciones visibles en 4:3 y 16:9.

Flujo: Training Mode → Choose Your Fighter → Choose Opponent → Choose Stage → combate. El jugador 1 controla ambas selecciones, sin exigir segundo mando. Reutilizar el selector nativo de 16 personajes, retratos y desbloqueos; permitir enfrentamientos espejo y colores distinguibles. Atrás en rival vuelve a personaje; atrás en escenario vuelve a rival; atrás en la primera selección vuelve al menú principal. Mantener las selecciones al retroceder.

El escenario predeterminado es el último utilizado en entrenamiento, si sigue disponible; en la primera sesión usar el escenario válido que el juego asigna al personaje. Ofrecer los escenarios ya integrados, sin introducir escenarios nuevos.

Cambiar personajes desde pausa regresa al selector con las elecciones actuales. Permitir cambiar escenario mediante el mismo flujo, conservando personajes y opciones.

## Práctica normal: sesión infinita

- Mostrar `TRAINING` y sustituir el contador por una indicación de tiempo infinito renderizable con la fuente nativa. Desactivar la condición de timeout, no únicamente su dibujo.
- Mantener las reglas, velocidad de simulación, hitstun, hitstop, bloqueos, combos, daño y transiciones entre barras del juego integrado.
- P1 y rival pierden vida de forma visible. Recuperar ambas reservas completas tras **3 segundos / 180 ticks de simulación activa** desde que termina la interacción; cada golpe o daño de bloqueo reinicia la espera.
- No recuperar durante combo, hitstun, blockstun, agarre, derribo pendiente, proyectil activo que pueda continuar el ataque o una transición cinematográfica. Esperar al estado seguro de reposo. La pausa y los menús detienen la cuenta.
- Opciones de recuperación: 1, 3 o 5 segundos; y Manual, que conserva daño hasta Restart Training. Cambiar esta opción reinicia la espera.
- Impedir el KO final de ambos luchadores en este preset, sin cortar el combo. La última reserva puede quedar en el mínimo positivo necesario; registrar el daño calculado antes de cualquier protección contra KO para que el medidor no subestime golpes letales.
- La protección final no debe impedir la transición ordinaria entre reservas de vida. Verificar y restaurar todas las reservas/flags, no solo el valor observado de HP.
- Sin pantalla de resultados, avance de arcade, continue, puntuación persistente ni obligación de cargar otra pelea. Regenerar vida mantiene la posición actual; Restart Training restablece posición y estado del combate.

Si el motor no permite proteger el KO manteniendo un combo íntegro, se debe revisar esta decisión antes de implementar una alternativa que distorsione el daño. Es un criterio de aceptación técnico, no una autorización para cambiar las reglas silenciosamente.

## Pausa y Training Options

Pausa de entrenamiento: Resume / Training Options / Restart Training / Change Fighters & Stage / Main Menu. Training Options usa subpáginas cortas y desplazamiento visible para que el texto sea legible a 480p.

| Página | Contenido y valores propuestos |
| --- | --- |
| Move List | Movimientos del personaje controlado; categorías y comando seleccionable para fijar en pantalla |
| Practice Preset | Normal / Claytalities |
| Opponent | Quieto / agachado / salto repetido / CPU / Player 2; bloqueo desactivado / siempre / después del primer impacto |
| Resources | Recuperación 1/3/5 s o Manual; súper Normal / lleno al reiniciar / infinito |
| Display | Daño y combo On/Off; historial de entradas On/Off; movimiento fijado On/Off |
| Restart Setup | Centro / esquina izquierda / esquina derecha; lado de P1 izquierdo / derecho; Restart Training |
| Claytality Practice | Elegir Claytality verificada; preparación Golpe final / Listo para ejecutar; ventana Original / extendida; distancia Automática / Manual |

Predeterminado: Normal, rival quieto, sin bloqueo, recuperación 3 s, súper lleno al reiniciar, daño visible, historial apagado, centro y P1 a la izquierda. Persistir opciones y últimas selecciones en un archivo propio versionado dentro del directorio de preferencias existente; ignorar valores desconocidos/inválidos y recuperar defaults. No persistir actores, vida ni animaciones.

CPU utiliza la IA original y su dificultad disponible; no inventar un nuevo sistema de IA. Player 2 requiere segundo dispositivo asignado: si falta o se desconecta, pasar a Quieto y mostrar un aviso breve. En los demás comportamientos el dummy no recibe entradas físicas de P2. El comportamiento sintético debe entrar por una vía que realmente gobierne al rival; inyectar input sin desactivar la IA no basta.

Bloqueo después del primer impacto permite practicar continuidad del combo: el rival recibe el primer golpe y bloquea en su primera oportunidad legal. No debe cancelar artificialmente su hitstun.

En Claytalities, forzar rival quieto y sin bloqueo; suspender recuperación y cualquier recurso que interfiera con el finisher. Restaurar la configuración normal al salir del preset. Al cambiar un comportamiento durante combate, aplicarlo al reanudar en un punto seguro y limpiar entradas sintéticas anteriores.

## Move List y controles

La lista cubre los **16 personajes** y los movimientos realmente ejecutables en esta fusión. Categorías: básicos y variantes de pie/agachado/aire; agarres y comandos propios; especiales; supers; herramientas del sistema de combos (starters, autos, linkers, enders, breakers y otras técnicas presentes); Claytalities. Indicar estados o requisitos particulares y distinguir ejemplos de combos de movimientos ejecutables.

Cada registro contiene ID de personaje/movimiento, nombre breve, categoría, secuencia de acciones lógicas, direcciones relativas, simultaneidad, carga si aplica, recursos, estado requerido y distancia para finishers. Evidencia interna por registro: definición/tabla verificada, escenario de ejecución y resultado. El material de ROM y capturas derivadas se mantiene privado.

La autoridad es el runtime integrado: originales con base 63⅓ y personajes donantes con las definiciones adaptadas cargadas por el port. Guías externas sirven como pistas, no como prueba de que un comando funciona. No copiar íntegramente listas o explicaciones de terceros. No asumir que todos los personajes tienen las mismas técnicas o Claytalities; una ausencia comprobada se muestra explícitamente. Una técnica pendiente de verificar bloquea declarar la lista completa.

Mostrar adelante/atrás relativos al rival, flechas orientadas según el lado actual, y nombre de ataque más glifo del botón físico vigente. Separar direcciones de botones: `B` nunca debe significar simultáneamente atrás y el botón físico B. La leyenda distingue secuencia, botones simultáneos, carga y cualquier fuerza de ataque permitida.

Resolver **dos niveles de remapeo**: permutación nativa de ataques configurada en el juego → acción del host → botón físico/tecla. El código actual de `port_options.cpp` ya consulta la permutación nativa; una lista que solo use A/B/C originales ignoraría esa configuración.

Reutilizar `binding_marker` para comandos de combate y `menu_marker` para navegación. Actualizar glifos al último dispositivo activo, mantener Auto/Xbox/PlayStation/Switch como respaldo y respetar A/B y X/Y intercambiados de Nintendo. Fijar un movimiento muestra su comando y requisitos sin tapar las barras. No mostrar comandos de un personaje anterior tras cambiar de selección.

Ofrecer Restart Training como acción lógica independiente y remapeable para teclado/mando. Auditar disponibilidad y conflictos antes de asignar un atajo predeterminado: ningún ataque, deslizamiento, pausa o componente de una Claytality debe reiniciar por accidente. Si no hay botón libre fiable, conservar acceso desde pausa y permitir asignación opcional. Esperar a soltar los botones de confirmación al reanudar/reiniciar para evitar un ataque o reinicio involuntario.

## Práctica de Claytalities

Preparación por defecto **Golpe final**: cargar el mismo enfrentamiento con P1 sano, rival en su última reserva y vida mínima positiva verificada. El jugador da el golpe final; el juego entra por su transición auténtica a la ventana de Claytality. No activar un finisher simplemente escribiendo HP=0.

Preparación alternativa **Listo para ejecutar**: entrar directamente en el estado de finisher mediante una ruta nativa validada, para ensayar el comando sin repetir el golpe previo. Ofrecerla solo cuando su transición esté verificada, como requisito de la 2.0 completa.

Seleccionar una Claytality fija su comando y requisitos. Distancia Automática prepara su rango válido y orientación; Manual conserva el punto de reinicio configurado. Los rangos se obtienen de comprobación del juego, no de convertir a píxeles aproximaciones de una guía. Los cambios de distancia se aplican durante preparación, nunca en mitad de la ejecución. Un golpe final puede desplazar al rival: comprobar el rango después de su transición y, en asistencia automática, ajustar cuando ambos actores estén estables. La opción Manual permite practicar posicionamiento real.

Ventana Original respeta el tiempo real del finisher; extendida permite hasta 30 segundos de simulación activa para aprender. Pausar y consultar Move List congela ambas ventanas. El temporizador de finisher es distinto del reloj de combate y requiere su propia verificación.

Ciclo: preparación → golpe final (si aplica) → ventana de ejecución → animación/fracaso → reinicio del mismo ejercicio. Si hay Claytality, dejar terminar animación y anuncio completos y esperar 1 segundo antes de reiniciar. Si expira la ventana o se produce una victoria convencional, indicar intento no completado y volver a preparar tras 1 segundo. Detectar eventos/estados nativos, no usar un retraso fijo para adivinar cuándo acaba una animación. Restart Training siempre permite repetir manualmente desde un punto seguro.

Nunca regenerar al rival durante la ventana o animación. Reiniciar también tras finisher sobre P1 en Player 2 no corresponde a este preset: el rival aquí permanece quieto. Sin entrada en resultados, ladder ni selector entre intentos. Mostrar intentos y Claytalities completadas durante la sesión, basados en eventos reales; no atribuir éxito solo porque se introdujo una secuencia de botones.

Un personaje sin Claytalities verificadas muestra esa condición y deshabilita este preset con explicación, manteniendo práctica normal y Move List. No inventar finishers para completar el roster.

## Feedback y reinicio

HUD de entrenamiento discreto: daño del último impacto, daño acumulado del combo y número de impactos. Respetar reducción de daño real y daño de bloqueo; no contar regeneración como daño ni mezclar impactos de ambos jugadores. Tomar eventos del cálculo/aplicación de daño: muestrear HP una vez por frame puede perder impactos múltiples, reservas y la protección de KO.

Historial opcional con 12 entradas recientes de P1: acciones ya remapeadas, direcciones, simultaneidad y duración en ticks. Registrar input entregado al combate, no navegación de pausa ni repeticiones por frame renderizado. Mantener el último combo visible hasta el siguiente o el reinicio, aunque se recupere la vida. No publicar datos de frames de ventaja ni hitboxes sin análisis específico.

Restart Training conserva personajes, escenario, opciones y comando fijado. Limpia combos, inputs pendientes, proyectiles, agarres, hitstun, stun, recursos según preset, orientación, cámara, posiciones y estado de escenario. Reponer escenario si fue destruido/cambió de zona. Cortar interpolación entre el estado viejo y el nuevo. No dejar audio del combate anterior activo ni cortar el finisher con un reset automático antes de su final.

Preferir una reinicialización nativa de combate verificada. No copiar una instantánea completa de RAM: contiene recursos y estado de hilos. Si para restaurar correctamente es necesario salir/reentrar en la escena, hacerlo conservando la sesión de entrenamiento y sin recorrido de menús o intro repetida. Objetivo de experiencia: control disponible en aproximadamente 1 segundo; medir en Linux y Windows. Si la ruta correcta requiere más tiempo, registrar la limitación y revisar la solución antes de cerrar 2.0.

## Arquitectura y evidencia del repositorio

Componentes propuestos, con nombres orientativos para implementación:

| Componente | Responsabilidad | Integración existente |
| --- | --- | --- |
| TrainingSession / training_mode | Estado de modo, selecciones, reglas, timers, eventos y solicitudes de reinicio | Puentes de combate y hooks reproducibles |
| TrainingMenu / training_options | Pausa, páginas, opciones y navegación | Fuente nativa y patrón de `port_options` |
| MoveCatalog / move_list | Datos auditables por personaje y notación independiente de botones | Definiciones integradas y `native_prompts` |
| TrainingDummy | IA/input sintético/Player 2 y bloqueo | Input P1/P2 y rutinas de actor verificadas |
| TrainingTelemetry | Daño, combos, historial y estado de intento | Eventos de simulación y renderizado nativo |

Puntos observados en el código actual:

- `host/main_menu.cpp` extiende el menú con Exit game y usa memoria reservada para sus tablas: ampliar de forma comprobada los tamaños y offsets; añadir una fila ingenuamente podría solapar las tablas actuales.
- `host/sc_roster.*` y sus fixtures ya amplían el selector a 16 slots.
- `host/input_mode.cpp` publica estado de combate/pausa mediante atomics; SDL no lee RDRAM concurrentemente. Mantener este límite.
- `host/port_options.cpp` aporta opciones nativas y resolución de la permutación de ataques; `host/native_prompts.*` resuelve glifos y marcadores.
- `tools/generate_cpu.py` inserta hooks con comprobación de instrucciones originales. Las modificaciones deben vivir en el generador y helpers, no solo en C generado.
- `analysis/combat_runtime_hooks.json` identifica `func_8003C740` como carga/ciclo de escena, `func_8003D940` como función real del frame de combate y `func_8003ED2C` como hilo de render. **Actualizar reglas una vez por tick de simulación verificado, no desde cada llamada del render.**
- El puente observa actores desde `0x801028B8/BC`, vida en `actor+0x2C0`, pausa en `0x8010849C` y attract en `0x80106D58`. Son evidencia parcial: no prueban todas las reservas, KO, finishers ni rangos.
- `analysis/hidden-highfive-rounds.json` registra HP inicial 170 para ese escenario y llegada a cero; declara expresamente que no trazó fin de bout. No convertir 170 ni HP=0 en constantes universales de entrenamiento.

Training permanece limitado a una sesión iniciada explícitamente. Entrada/cancelación/salida limpian el estado; attract, arcade y versus conservan sus reglas. Todas las escrituras de actores ocurren en el hilo de simulación propietario, con punteros y estado validados; opciones desde otros hilos se entregan como solicitudes o snapshots sincronizados.

Memoria de textos, cursores y glifos tendrá límites explícitos; paginar listas largas. Revisar reservas RDRAM y lifetimes de fuente/escena con el presupuesto existente. Integrar nuevos archivos tanto en `CMakeLists.txt` como en el proyecto Windows y regeneración/fixtures correspondientes.

## Trabajo técnico previo a la implementación de reglas

Resolver con trazas/fixtures privados: (1) selección secuencial P1/rival y asignación de control humano; (2) reservas de vida y transición no letal/KO; (3) contador de combate, contador de finisher y tick autoritativo; (4) ventana/éxito/fallo/fin de animación de Claytality; (5) reinicialización de actores, escena y recursos; (6) eventos de daño/combos; (7) entradas del dummy y oportunidad legal de bloqueo; (8) catálogo ejecutable y rangos por personaje.

Cada punto produce evidencia de función, instrucciones esperadas, campos implicados y reproducción. Las direcciones todavía no demostradas se descubren durante esta fase; no se inventan en la especificación. Una transición no comprendida impide implementar su mutación. No reutilizar direcciones de Sculptor's Cut como si pertenecieran al ejecutable base 63⅓.

## Entregas y validación de 2.0

Orden recomendado de trabajo después de aprobar el diseño: investigación de estados y catálogo; flujo de menú/selección; sesión normal y dummy; Move List/feedback; preset Claytalities/reinicios; validación y paquetes Linux/Windows. Este orden define dependencias, no sustituye al plan de implementación detallado posterior.

Criterios de aceptación:

1. Navegación de las cuatro filas, confirmación/atrás/idle/attract y selector secuencial sin segundo mando; espejos y selección de los 16 personajes; etiquetas y bordes legibles en 4:3/16:9 a 480p.
2. Sesión activa de al menos 30 minutos sin timeout ni resultados. Recuperación exacta tras 180 ticks elegibles; reinicio de la espera por golpes/bloqueo; pausa no consume ticks; distintos targets de presentación no cambian el comportamiento.
3. Agotar ambas reservas sin terminar práctica normal ni romper combos; regenerar ambas; daño de golpes letales, multigolpe, chip y cambio de reserva medido correctamente.
4. Quieto, agachado, saltos, CPU, Player 2 y bloqueo comprobados; desconexión/hotplug no asigna el mando al jugador incorrecto.
5. Lista completa contrastada por cada personaje y comando disponible, en ambos lados; remapeo host y permutación nativa; simultaneidad y carga; casos sin técnica/finisher documentados.
6. Cada Claytality disponible ejecutada con su rango y requisitos; éxito/fallo/timeout, pausa durante ventana, ambas preparaciones, reposicionamiento y final de animación. Como mínimo 100 reinicios consecutivos representativos, incluyendo finalizadores que cambian cámara o escena, sin crecimiento sostenido de recursos ni bloqueo.
7. Restart limpia proyectiles, agarres, cámara, entradas, stun y estado de escenario; conserva opciones/selecciones; salida a arcade/versus/attract no conserva reglas de training.
8. Prompts y acciones Xbox/PlayStation/Switch y teclado verificados automáticamente; comprobar ambos con mandos reales cuando estén disponibles. Registrar explícitamente el hardware no probado.
9. Tests de reglas con ticks controlados y fixtures de rutinas originales; sesiones nativas con SDL y render/audio reales en Linux y Windows. Ejecutar suites existentes para detectar regresiones, sin presentar la validación 1.0 como prueba de training.
10. Instalación limpia y actualización conservan ROMs locales y preferencias; paquetes/source archive/checksums y documentación coinciden con binarios verificados. Atribución propia y Git author/committer comprobados como djultra64 antes de commit/publicación; avisos de terceros intactos. Sin ROMs ni recursos extraídos en distribución.

La publicación 2.0 depende de cumplir estos criterios y registrar alcance/resultados. No cambiar las etiquetas de release a 2.0 mientras solo exista el diseño.

## Fuera del alcance inicial

Grabación/reproducción del dummy con múltiples slots, guardado de estados arbitrarios, análisis de ventaja por frames, visualizador de hitboxes, demostraciones automáticas de cada movimiento, desafíos de combos con puntuación y entrenamiento online. Son ampliaciones posteriores; no necesarias para cubrir lo pedido con un modo completo y usable.

## Aprobación y siguiente etapa

El usuario aprobó el diseño mediante «Hazlo». Producir el plan técnico detallado y acordar su ejecución conforme al flujo de planificación. Esta aprobación incluye los defaults de recuperación (3 segundos después del combate), las dos preparaciones de Claytality y las herramientas incluidas en 2.0. No constituye una afirmación de implementación, validación o publicación.
