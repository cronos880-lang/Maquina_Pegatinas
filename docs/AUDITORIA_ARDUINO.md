# Auditoría del sketch `Pegatinas_14_01_2024_Ver_7_Full_Doble_Bajad__Barrera`

- **Fecha:** 24-11-2026.
- **Placa:** Arduino Mega 2560, módulo Bluetooth en `Serial1` (9600 baudios) y tablet Android de 7" en horizontal con Virtuino.
- **Actuadores:** motor paso a paso de la bandeja (driver STEP/DIR), pistón neumático con dos electroválvulas, bomba de vacío y electroválvula de soplado.
- **Seguridad:** barrera fotoeléctrica (cortina amarilla).
- **Alcance:**
  - Revisión del código fuente tal como está, sin modificarlo.
  - No se ha compilado ni ejecutado en la placa.
  - Las fotos de las tres páginas de Virtuino se usan para cruzar las variables V.
- **Hechos e hipótesis:** se indica qué está comprobado en el código y qué hay que verificar en la máquina (marcado **[VERIFICAR]**).

## 1. Qué hace la máquina según el código

### Entradas y salidas

| Pin | Uso | Tipo |
|---|---|---|
| 2 | Sensor de punto cero de la bandeja (LOW = en cero) | Entrada |
| 3 | Sensor del pistón **abajo** (LOW = activo) | Entrada con pull-up |
| 4 | Sensor del pistón **arriba** (LOW = activo) | Entrada con pull-up |
| 5 | Electroválvula del pistón: **bajar** (HIGH) | Salida |
| 6 | Electroválvula del pistón: **subir** (HIGH) | Salida |
| 7 | Relé de la bomba de vacío | Salida |
| 8 / 9 | Motor paso a paso: DIR / STEP (200 pasos por vuelta) | Salidas |
| 10 | Electroválvula de soplado | Salida |
| 11 | Corte general del aire por barrera (HIGH = aire, LOW = corte). El comentario del código dice "reset", pero está desfasado (ver C3) | Salida |
| 12 | Barrera de seguridad (HIGH = libre, LOW = interrumpida) | Entrada con pull-up |

> **Comentarios cruzados.** Los comentarios de `setup()` dicen "pin 3 = pistón arriba" y "pin 5 = sube". El código hace lo contrario:
> - pin 4 = arriba y pin 3 = abajo;
> - pin 5 = bajar y pin 6 = subir.
>
> La tabla sigue al código, no a los comentarios.

### Secuencia (máquina de pasos `step[0..6]`)

| Paso | LED Virtuino | Qué hace |
|---|---|---|
| 0 | V20 | **Busca el cero.** Espera a que el pistón esté arriba y a que se active **SET POINT** (V13). Gira despacio (24 ms por paso) hasta el sensor de cero. Avisa con V0. |
| 1 | V21 | **Inclina la bandeja.** 25 pasos (45°) a la posición de carga. |
| 2 | V22 | **Espera la marcha.** Con **VACÍO** (V15) se enciende la bomba (V19 la apaga). Con **START** (V11) y el vacío encendido, la bandeja vuelve al cero y gira 100 pasos (180°). Después espera el tiempo BAJAR (V18) y **baja el pistón**. |
| 3 | V23 | **Primera bajada.** Con el pistón abajo apaga el vacío y sopla. Espera SUBIR (V17) y sube el pistón mientras sigue soplando durante SOPLADO (V16). |
| 4 | V24 | **Espera el botón "PASO 2"** (flecha, V30). Gira otros 100 pasos (180°), espera BAJAR (V18) y **baja el pistón** otra vez. |
| 5 | V25 | **Segunda bajada.** Con el pistón abajo espera SUBIR (V17) y sube. |
| 6 | V26 | **Fin de ciclo.** Con el pistón arriba vuelve a inclinar la bandeja 25 pasos y regresa al paso 2. |

**[VERIFICAR]** Interpretación física probable:
- la bandeja de ventosas (dos filas de 8) recoge las pegatinas por vacío;
- gira 180° y las presiona sobre la pieza;
- se suelta con soplado;
- en la "doble bajada" vuelve a presionar.

Confírmalo para ponerle nombres correctos a cada paso en la app.

### Página por página (fotos de Virtuino)

| Página | Elemento | Variable |
|---|---|---|
| 1 | SET POINT (interruptor) + visto bueno | V13 / V0 |
| 1 | MARCHA (LED) + START | V6 / V11 |
| 1 | PASO 2 (flecha) | V30 |
| 1 | VACÍO (LED ventilador) + botón | V7 / V15 (V19 apaga) |
| 2 | VELOCIDAD "10000 STEPS" (mando) | V9 (**no lo usa el código**, ver A3) |
| 2 | TIEMPOS: SOPLADO / SUBIR / BAJAR | V16 / V17 / V18 (segundos) |
| 3 | INICIO DE SECUENCIA, STEP 1, 2, 4, 5, 6, 7 | V20…V26 (los nombres no coinciden con los pasos del código, ver M4) |

## 2. Lo que está bien hecho

- **Secuencia clara por pasos**, cada uno con su LED en la app: muy útil para saber dónde se ha quedado la máquina.
- **`vDelay()`** atiende el Bluetooth mientras espera, en lugar de usar `delay()`.
- **Vacío obligatorio para arrancar**: no deja empezar el ciclo sin la bomba encendida.
- **Búsqueda de cero** al encender y antes de cada ciclo: corrige los pasos perdidos.
- **Tiempos ajustables** desde la tablet, sin recompilar.
- **Barrera fotoeléctrica integrada** en la secuencia.

## 3. Hallazgos, ordenados por gravedad

### CRÍTICOS (seguridad de las personas)

**C1. La barrera se consulta por software y solo en algunos momentos.**
- **Qué pasa:** `Emer()` solo se llama entre pasos y en algunos bucles. **No se llama**:
  - en la búsqueda de cero (paso 0);
  - en la inclinación de la bandeja (paso 1);
  - en el regreso al cero antes del ciclo (bucle `otra`);
  - en el giro final (paso 6);
  - durante ninguna espera `vDelay()` (los tiempos SOPLADO/SUBIR/BAJAR y las esperas de 1 s).
- **Consecuencia:** la bandeja puede girar y el pistón puede bajar con la mano dentro de la barrera.
- **Solución:**
  - **Hardware (imprescindible):** la barrera debe cortar la alimentación de las electroválvulas y del driver del motor mediante un **relé de seguridad** (categoría y PL según evaluación de riesgos, EN ISO 13849-1). Un Arduino no es un dispositivo de seguridad.
  - **Software (complementario):** consultar la barrera en **cada pasada** del programa sin bloquear y llevar la máquina a un estado seguro.

**C2. `Emer()` tiene un error de sintaxis que la hace funcionar "de rebote".**
- **Qué pasa:** `if(stop==1);{ digitalWrite(11, HIGH);}`. El `;` cierra el `if`, así que el pin 11 se pone a HIGH **siempre**. Después, si la barrera está cortada, vuelve a LOW.
- **Consecuencia:** con la barrera cortada, el pin 11 hace pulsos HIGH/LOW en cada llamada. Hoy funciona porque el pulso LOW es el que actúa, pero es por casualidad.
- **Solución:** quitar el `;`. En la V2, sustituir el método completo (ver C3).

**C3. La barrera corta el aire, pero no detiene el motor y el ciclo se reanuda solo. [Actualizado 24-11-2026]**
- **Información del propietario:**
  - al poner la mano en la barrera **se corta el aire, sin reset**;
  - la seta de emergencia va directa a la placa y hace el reset.
- **Conclusión:**
  - el pin 11 manda, casi seguro, la **válvula o relé general del aire** (HIGH = aire, LOW = corte);
  - el comentario "reset de la placa" está desfasado;
  - encaja con que la placa no se reinicie sin fin al arrancar.
- **Consecuencias:**
  - **el motor de la bandeja sigue girando** con la barrera cortada (no necesita aire);
  - al retirar la mano, la siguiente llamada a `Emer()` **devuelve el aire y el ciclo sigue solo**, sin confirmación;
  - si la última llamada a `Emer()` del ciclo encuentra la barrera cortada (al retirar la pieza), **el aire queda cortado durante la carga** hasta pulsar START.
- **Al arrancar:** el aire está cortado unos 0,9 s; después, `setup()` lo devuelve y **sube el pistón sin mirar la barrera**.
- **Seta (reset):** se pierden los tiempos ajustados (vuelven a 3 / 1 / 0, no hay EEPROM) y hay que volver a buscar el cero.

**C4. Al encender, el pistón se mueve solo.**
- **Qué pasa:** `setup()` activa la electroválvula de subir (pin 6) sin comprobar la barrera ni esperar a nadie.
- **Solución:** arrancar con todo en reposo y pedir un rearme consciente: SET POINT con la barrera libre.

### ALTOS

**A1. Sin tiempos límite en los sensores.**
- **Qué pasa:** si el pistón no llega abajo o arriba (falta aire, sensor roto, pegatina atascada), la máquina espera **para siempre** sin avisar.
- **Lo mismo en la búsqueda de cero:** si el sensor del pin 2 falla, el motor gira sin fin.
- **Solución:** límite de tiempo en cada movimiento, alarma en la app y salidas a reposo.

**A2. La app se queda "congelada" durante los movimientos.**
- **Qué pasa:** los giros se hacen con bucles `delayMicroseconds()`. Mientras duran, ni se atiende el Bluetooth ni se mira la barrera:
  - 100 pasos × 16 ms = 1,6 s;
  - la búsqueda de cero a 24 ms por paso puede pasar de 4 s.
- **En el paso 0, con el pistón abajo:** el bucle `while(pulso_cero==0)` no llama a `vDelay()`, así que la app **no puede ni activar SET POINT**.
- **Solución:** motor sin bloqueo (un paso cada X µs desde `loop()`), con rampa de aceleración.

**A3. El mando de VELOCIDAD no hace nada.**
- **Qué pasa:** la velocidad está fija en el código (`V12_velocidad_Motor = 8000;  //V[12]`). El mando de la página 2 escribe en V9/V12, pero nadie lo lee.
- **Solución:** leer la velocidad de la app con límites seguros, o quitar el mando.

**A4. Los ajustes no se guardan.**
- **Qué pasa:** SOPLADO, SUBIR y BAJAR vuelven a 3 / 1 / 0 en cada reinicio, que con C3 es cada vez que salta la barrera.
- **Solución:** EEPROM con validación y valores por defecto, como en el acuario.

**A5. No hay botón de PARO ni de cancelar ciclo.**
- **Qué pasa:** la única forma de parar es cortar la barrera o desenchufar.
- **Solución:** PARO desde la app, que lleva a reposo, y REARME.

### MEDIOS

**M1. El LED de MARCHA no se apaga nunca.**
- **Qué pasa:** al terminar el ciclo se escribe `V[4] = 0` en lugar de `V[6] = 0`.

**M2. El LED del último paso (V26) se queda encendido.**
- **Qué pasa:** al volver al paso 2 no se apaga V26, ni V23 ni V25 en algunos casos.
- **Se ve en la foto 3:** STEP 5 y STEP 7 encendidos a la vez.

**M3. Indicadores que nunca se actualizan.**
- **Qué pasa:** V1, V2, V3, V4 y V5 (giro 180°, giro 0°, posición inicial, pistón arriba y abajo) están declarados, pero el código no los envía.
- **Consecuencia:** la app no puede mostrar los sensores.

**M4. Los nombres de la página 3 no coinciden con los pasos.**
- **Qué pasa:** la página pone "STEP 1, 2, 4, 5, 6, 7" y el código usa los pasos 0 a 6. No hay "STEP 3".

**M5. Paso 4: puede hacer falta pulsar la flecha dos veces.**
- **Qué pasa:** el sensor "arriba" se lee **antes** de esperar la flecha (V30). Si el pistón aún no había llegado arriba, se descarta la pulsación y vuelve a esperar.

**M6. Esperas fijas de 1 s en los pasos 3, 4 y 5.**
- **Qué pasa:** cada comprobación del sensor añade hasta 1 s.
- **Consecuencia:** se pierden unos 3 s por ciclo.

**M7. Los tiempos no se validan.**
- **Qué pasa:** un valor negativo o enorme desde la app se aplica tal cual.
- **Desbordamiento:** `vDelay(int)` pasa de 32 s; el mismo cálculo con `long` desbordaría a los 49 días.

### BAJOS (limpieza)

- **`debug = true`:** imprime cada comando por `Serial` a 9600 baudios, lo que ralentiza la respuesta. Ponerlo a `false`.
- **Código muerto:**
  - variable `salta`: nunca cambia, así que todos los `if(salta==0)` sobran;
  - `stepDelay`, `Emergencia`, `giro`, `V3_Pos_ini_ok`, `step1` y `cero` no se usan;
  - `borra = 10` tiene el mismo valor que `soplo` (pin 10), aunque se usa como contador.
- **Pines mezclados:** a veces con nombre (`soplo`) y a veces con número (`10`).
- **Doble `;`** en `int inicio=0;;`.

## 4. Opciones (como en el acuario)

### Opción A: app nueva sin tocar el Arduino (rápida)

Acuaris ya habla VirtuinoCM, así que puede sustituir a Virtuino **con el sketch actual sin cambios**.
- **Proyecto `Pegatinas.acuaris.json`:**
  - en horizontal para la tablet de 7";
  - las mismas tres páginas: Mando, Ajustes y Secuencia;
  - nombres de pasos corregidos.
- **Simulador de la máquina** para probar la interfaz en el PC.
- **Lo que no arregla:** los fallos C1–C4 y A1–A5 siguen en el Arduino.

### Opción B: sketch V2 + app (recomendada, el mismo camino que el acuario)

1. **`Pegatinas_V2`** por pestañas: `Secuencia`, `Motor`, `Piston`, `Vacio`, `Seguridad`, `Ajustes`, `Comunicacion`, `Diagnostico`.
2. **Funcionamiento:**
   - máquina de estados **sin bloqueos** (`millis()`/`micros()`);
   - la barrera se consulta en cada pasada;
   - con la barrera cortada, **estado seguro**: todo a reposo, motor parado y **rearme** obligatorio, sin reiniciar la placa.
3. **Fallos y avisos:** tiempos límite en cada movimiento, con **alarmas** y código de fallo en la app.
4. **Ajustes:** velocidad real desde la app (con rampa), tiempos con límites y guardado en **EEPROM**.
5. **Mandos nuevos:** PARO, REARME y **modo manual** para pruebas (subir/bajar pistón, vacío, soplo, girar).
6. **Contador** de ciclos y diagnóstico: versión, reinicios y causa del último reinicio.
7. **Compatibilidad:** las variables V actuales se mantienen, así que el panel de Virtuino sigue funcionando mientras tanto.
8. **Pruebas:** automáticas en el PC con Arduino simulado, igual que `Control_Acuario_V2`.
9. **Vuelta atrás:** el sketch original queda intacto en `arduino/` como referencia.

### Opción C: B + migración a ESP32 (opcional, más adelante)

- **Ventajas:** WiFi y Bluetooth integrados, más memoria y actualización por WiFi.
- **Inconvenientes:** obliga a cambiar el cableado a 3,3 V (sensores y driver).
- **Valoración:** para esta máquina la Mega sobra; no lo recomiendo salvo que quieras WiFi.

### En cualquier opción: seguridad física

La barrera debe actuar por **hardware** mediante un relé de seguridad que corte la neumática y el driver. El software solo informa y gestiona el rearme. Conviene revisarlo antes de cualquier cambio de programa.

## 5. Preguntas para continuar

1. ¿Qué hay conectado al **pin 11** (reset, relé, driver)? ¿Hay relé de seguridad en la barrera?
2. ¿Es correcta la interpretación física del ciclo (recoger con vacío, girar, presionar, soplar y segunda bajada)? ¿Por qué la segunda bajada espera a la flecha?
3. ¿Modelo del módulo Bluetooth (HC-05 o HC-06) y de la tablet?
4. ¿Qué driver y qué motor paso a paso lleva? ¿Micropasos?
5. ¿El botón de VACÍO de la página 1 enciende y apaga, o hay otro botón (V19) que no sale en las fotos?
6. ¿Opción A, B o ambas en paralelo?
