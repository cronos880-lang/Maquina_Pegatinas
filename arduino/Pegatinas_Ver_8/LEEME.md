# Pegatinas Ver_8 (8.0.0 de prueba)

Versión pulida del programa de la máquina de pegatinas.

- **No sustituye a `Pegatinas_14_01_2024_Ver_7_Full_Doble_Bajad__Barrera`.** La Ver_7 sigue siendo la que funciona y la de vuelta atrás.
- **No es oficial** hasta que la apruebes tras probarla en la máquina.
- **Lo que no cambia:** el mismo cableado, los mismos pines y el mismo panel de Virtuino.
- **Lo único que se añade en Virtuino:** el botón PARO (V31) y, si lo quieres, el interruptor de modo de velocidad (V32).

## Qué cambia respecto a Ver_7

| Ver_7 | Ver_8 |
|---|---|
| Programa con esperas que lo bloquean: durante los giros la tablet no responde | **Sin bloqueos**: motor, tiempos y Bluetooth a la vez |
| Barrera mirada solo en algunos momentos. No la mira en la búsqueda del cero, la inclinación, el giro final ni durante los tiempos | **Barrera vigilada continuamente** mientras hay movimiento |
| Barrera cortada: corta el aire, pero **el motor sigue girando** | **PAUSA**: aire cortado, motor parado y tiempos congelados |
| Al quitar la mano, el ciclo **sigue solo** | **Sigue parada**: START reanuda donde estaba y PARO cancela |
| El aire podía quedar cortado durante la carga si se pasaba la mano al retirar la pieza | **Carga con la barrera ignorada**: aire dado y vacío disponible con la mano dentro |
| START con la mano en la barrera | **No arranca** (hay que pulsar START con la barrera libre) |
| Para parar solo estaba la seta (reset) | **PARO (V31)**: para el ciclo. Otra pulsación busca el cero y vuelve a carga, **sin perder los ajustes** |
| Si el pistón no llega o falla el sensor de cero, espera para siempre | **Tiempo límite**: para la máquina y parpadea el LED del paso donde se ha quedado |
| Tiempos SOPLADO / SUBIR / BAJAR perdidos tras cada reset | **Guardados en la EEPROM**, con límites de 0 a 30 s |
| Mando de VELOCIDAD sin efecto | Velocidad **fija (8000, como Ver_7)** o **la del mando**, según el interruptor V32 |
| LED MARCHA siempre encendido y LEDs de pasos anteriores encendidos | Corregido: un solo LED de paso encendido y MARCHA se apaga al acabar |
| La flecha "PASO 2" a veces había que pulsarla dos veces | Vale **a la primera**, aunque se pulse con el pistón todavía subiendo |
| Comprobaciones del pistón cada 1 s | **Continuas**: el ciclo es unos 2–3 s más corto (ver «Tiempo de prensado») |
| Giros con arranque brusco | **Rampa** suave de arranque y frenada (6 pasos) |

## Cuándo manda la barrera

| Momento | Barrera |
|---|---|
| Esperando SET POINT al encender | Ignorada. Si se activa SET POINT con la barrera cortada, no arranca: hay que volver a activarlo con la barrera libre |
| Búsqueda del cero e inclinación | **Vigilada** → pausa |
| Carga de la hoja (paso 2, esperando START) | **Ignorada** |
| Desde START hasta que el pistón vuelve arriba tras la primera bajada | **Vigilada** → pausa |
| Esperando la flecha "PASO 2" con el pistón arriba | **Ignorada** |
| Desde la flecha hasta volver a carga | **Vigilada** → pausa |
| Parada (tras PARO o un fallo) | El aire se da si la barrera está libre o si el pistón ya está arriba. Con la barrera cortada y el pistón abajo, el aire espera a que se libere y entonces **el pistón sube solo** (es su posición segura) |

> **La seta de emergencia** va directa a la placa (reset) y no depende del programa: funciona igual que antes.

## Botones nuevos en Virtuino

| V | Elemento | Configuración |
|---|---|---|
| **V31** | Botón **PARO** | Botón. Valor al pulsar: 1. Sirve el modo pulsador o el de interruptor: el programa lo vuelve a 0 |
| **V32** | Interruptor **modo de velocidad** | Interruptor 0/1. 0 = velocidad fija 8000 (como Ver_7) · 1 = la del mando VELOCIDAD |

**Mando de velocidad:**
- Es el pin **V9**, el que muestra "STEPS". **Comprueba en Virtuino que el mando escribe en V9.**
- El valor es el tiempo entre pasos en µs: **más alto = más lento**.
- Se limita de **6000 a 12000**. Ver_7 usaba 8000; el 10000 que mostraba el panel no se aplicaba.

## PARO y reinicio

1. **PARO durante el ciclo o la pausa:**
   - el motor se para al momento;
   - se corta el soplado;
   - **se apaga el vacío**;
   - el pistón sube;
   - se apaga MARCHA.
2. **Parada sin fallo:** parpadea el LED **INICIO DE SECUENCIA** (V20).
3. **Parada por fallo:** parpadea el LED del **paso donde se ha quedado**.
4. **Otra pulsación de PARO** (con la barrera libre):
   - sube el pistón si hace falta;
   - busca el cero despacio;
   - inclina la bandeja;
   - queda en carga.

## Tiempo de prensado

En Ver_7, el programa miraba el sensor "pistón abajo" **1 s después** de mandar bajar. Por eso el pistón se quedaba prensando, con vacío, hasta ese segundo antes de empezar SUBIR. En la Ver_8 se detecta en cuanto llega.

**Si notas que las pegatinas pegan peor**, sube un poco el tiempo **SUBIR** en la tablet. Es el tiempo que el pistón está abajo antes de subir.

## Variables V

**Las de Ver_7 (el panel actual funciona igual)**

| V | Qué es |
|---|---|
| V0 | Punto cero encontrado |
| V6 | LED MARCHA (parpadea en pausa) |
| V7 | LED VACÍO |
| V9 | Mando de velocidad |
| V11 | START (también reanuda tras una pausa) |
| V13 | SET POINT |
| V15 / V19 | Vacío encendido / apagado |
| V16 / V17 / V18 | Tiempos SOPLADO / SUBIR / BAJAR (s) |
| V20–V26 | LEDs de los pasos 0–6 |
| V30 | Flecha "PASO 2" |

**Nuevas** (no hace falta ponerlas en el panel; están para el diagnóstico)

| V | Qué es |
|---|---|
| V4 / V5 | Pistón arriba / abajo (1 = sí) |
| V31 | PARO |
| V32 | Modo de velocidad |
| V50 | Versión (800 = 8.0.0) |
| V51 | Estado: modo × 100 + paso × 10 + fase. Modo 0 normal, 1 pausa, 2 parada |
| V52 | Fallo: 0 ninguno, 1 el pistón no baja, 2 el pistón no sube, 3 no encuentra el cero |
| V53 | Ciclos desde el arranque |
| V54 | Velocidad en uso (µs) |

## Valores que se pueden cambiar en el código (`Pegatinas_Ver_8.ino`)

| Constante | Valor | Para qué |
|---|---|---|
| `LIMITE_PISTON_MS` | 8000 | Tiempo máximo para que el pistón llegue arriba o abajo |
| `PASOS_MAX_CERO` | 250 | Pasos máximos buscando el cero (una vuelta son 200) |
| `PASOS_RAMPA` | 6 | Pasos de arranque y frenada suaves (0 = sin rampa) |
| `SEMI_FIJA` | 8000 | Velocidad fija (la de Ver_7) |
| `SEMI_MIN` / `SEMI_MAX` | 6000 / 12000 | Límites del mando de velocidad |
| `PAUSA_EV_PASO*_MS` | 100 / 1000 / 200 / 300 | Pausas entre electroválvulas, las mismas de Ver_7 |

## Cargar en el Arduino

1. **Guarda una copia de la Ver_7** y comprueba que sabes cargarla (es la vuelta atrás).
2. Arduino IDE → abre `Pegatinas_Ver_8/Pegatinas_Ver_8.ino`. Se abren todas las pestañas.
3. Placa **Arduino Mega 2560**. Librería **VirtuinoCM** (la misma de Ver_7).
4. Carga el programa con la máquina **sin hojas y sin producción**.

## Lista de comprobación en la máquina

Hazla en este orden, sin hojas. Si algo no sale como se indica, vuelve a cargar la Ver_7 y dime en qué punto falló.

1. **Arranque:**
   - aire dado y pistón arriba;
   - la bandeja **no se mueve** hasta activar SET POINT.
2. **SET POINT:**
   - busca el cero despacio;
   - se inclina;
   - queda en carga: LED STEP 2 encendido y visto bueno del SET POINT.
3. **Carga con la mano en la barrera:**
   - el aire no se corta;
   - VACÍO enciende la bomba.
4. **START con la mano en la barrera:** no arranca.
5. **START con la barrera libre:**
   - gira 180° y baja;
   - sopla y sube;
   - espera la flecha.
6. **Con la máquina esperando la flecha, mano en la barrera:** no pasa nada. La flecha con la mano dentro no hace nada.
7. **Flecha:**
   - gira 180° y baja;
   - sube y vuelve a carga;
   - **MARCHA se apaga** y solo queda encendido el LED STEP 2.
8. **Barrera durante un giro:**
   - se para y se corta el aire;
   - MARCHA parpadea.
9. **Quita la mano:** sigue parada. **START:** continúa y termina el giro en su sitio.
10. **Barrera durante un tiempo** (por ejemplo, SUBIR): se para. Al reanudar, termina el tiempo que faltaba.
11. **PARO durante el ciclo:**
    - se para;
    - el vacío se apaga;
    - el pistón sube;
    - parpadea INICIO DE SECUENCIA.
12. **PARO otra vez:** busca el cero, se inclina y queda en carga.
13. **Cambia SOPLADO**, espera 3 s y pulsa la **seta**. Al volver, el tiempo se conserva.
14. **Seta durante un ciclo:** se comporta como antes (reset).
15. **Varios ciclos con hojas:** comprueba que las pegatinas pegan igual (ver «Tiempo de prensado»).
16. **Opcional, modo de velocidad:**
    - V32 = 1;
    - mueve el mando y comprueba que cambia la velocidad de los giros;
    - **no la subas de golpe**: prueba primero valores cercanos a 8000.

## Pruebas automáticas en el PC

```bash
arduino/Pegatinas_Ver_8/pruebas/ejecutar_pruebas.sh
```

**Qué hace:**
- Ejecuta el sketch real con un **simulador de la máquina**: bandeja, sensor de cero, pistón, aire y barrera.
- Recorre **18 escenarios**: ciclo completo, tiempos, barrera en cada fase, PARO, fallos, ajustes, velocidad, rampa y Bluetooth.

**Qué no es:** el simulador no es la máquina. La prueba buena es la lista de comprobación de arriba.

**Compilación automática:** GitHub Actions compila el sketch para Arduino Mega en cada subida. El resultado está en la pestaña **Actions**.
