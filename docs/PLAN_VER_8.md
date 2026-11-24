# Plan de la Ver_8 (pulido del sketch)

- **Estado:** desarrollada (`arduino/Pegatinas_Ver_8/`), pendiente de probar en la máquina y de tu aprobación.
- **Base:** `Pegatinas_14_01_2024_Ver_7_Full_Doble_Bajad__Barrera`.

## Condiciones

- **Sin cambios físicos** en la máquina. Mismo cableado y mismos pines.
- **Sin interfaz nueva.** El panel de Virtuino actual sigue igual y conserva las mismas variables V.
  - Única excepción: un botón **PARO** (V31), que añade el propietario para las pruebas.
- **La Ver_7 se conserva intacta** para volver atrás.

## Comportamiento nuevo

### Barrera (pin 12 → corte de aire por pin 11)

| Situación | Ver_7 | Ver_8 |
|---|---|---|
| Carga de la hoja (paso 2, esperando START, máquina quieta) | No se mira; puede quedar el aire cortado del ciclo anterior | **Se ignora.** Aire dado, vacío disponible |
| START con la barrera cortada | Corta el aire y sigue el ciclo | **No arranca** hasta que la barrera esté libre |
| Barrera cortada durante el ciclo | Corta el aire, pero el **motor sigue**. Solo se mira en algunos puntos | **Pausa:** corte de aire, motor parado y tiempos congelados. Se mira continuamente |
| Barrera libre otra vez | El ciclo **sigue solo** | **Sigue parado**. Para continuar: START (reanuda) o PARO (cancela) |
| Esperando la flecha "PASO 2" (paso 4) | Se mira antes y después | **Se ignora** con el pistón arriba; la flecha con la barrera cortada no arranca |

### Botón PARO / REINICIO (V31)

1. **Una pulsación durante el ciclo:**
   - el motor se detiene al momento;
   - se corta el soplado;
   - el pistón sube;
   - **se apaga el vacío** (decisión del propietario);
   - se apaga el LED de MARCHA.
2. **Otra pulsación con la máquina parada:**
   - busca el cero;
   - inclina la bandeja;
   - queda en carga (paso 2).
3. **Sin seta y sin reset:** se conservan los tiempos ajustados.

### Pulido

- Sensores del pistón vigilados continuamente, sin esperas fijas de 1 s: el ciclo es unos 3 s más corto.
- Flecha "PASO 2": responde siempre a la primera.
- Motor sin bloqueo y con rampa. La tablet responde mientras gira.
- Tiempos SOPLADO / SUBIR / BAJAR con límites y guardados en EEPROM.
- Velocidad: fija (8000) o la del mando, según un interruptor nuevo en Virtuino (V32).
- **Tiempos límite** en cada movimiento: parada segura y LED del paso parpadeando.
- **LEDs corregidos:**
  - MARCHA (escribía V4 en lugar de V6);
  - V26 y otros pasos que se quedaban encendidos.
- `Emer()` (con el `;` de más) se sustituye por la vigilancia continua de la barrera.
- `debug = false`.

## Cómo se prueba

1. **Pruebas automáticas en el PC** con el Arduino simulado: sensores, barrera, PARO, tiempos límite y EEPROM.
2. **En la máquina, sin hojas y sin producción,** siguiendo la lista de comprobación:
   - barrera en cada paso;
   - seta;
   - PARO y reinicio;
   - ciclo completo.
3. **Si algo falla:** se carga la Ver_7.
