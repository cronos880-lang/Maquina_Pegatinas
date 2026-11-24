// ============================================================================
//  SECUENCIA del ciclo: los mismos pasos 0..6 de Ver_7, sin esperas bloqueantes.
//  Cada paso se divide en fases; las esperas se descuentan en loop() y se congelan en pausa.
// ============================================================================

void irA(uint8_t p, uint8_t f) {
  paso = p;
  aFase(f);
}

void aFase(uint8_t f) {
  fase = f;
  espera = 0;
  limite = LIMITE_PISTON_MS;
}

void esperar(unsigned long ms) { espera = ms; }
bool esperaHecha() { return espera == 0; }

void descontar(unsigned long dt) {
  espera = (espera > dt) ? espera - dt : 0;
  limite = (limite > dt) ? limite - dt : 0;
}

unsigned long segundosAms(float s) { return (unsigned long)(s * 1000.0 + 0.5); }

// ¿Hay movimiento posible en este punto? Entonces la barrera manda.
// En las esperas con la máquina quieta (SET POINT, carga, flecha PASO 2) se ignora.
bool vigilada() {
  if (modo != M_NORMAL) return false;
  switch (paso) {
    case 0: return fase >= 2;          // búsqueda del cero
    case 2: return fase >= 2;          // desde que se acepta START
    case 4: return fase != 1;          // salvo esperando la flecha con el pistón arriba
    default: return true;              // pasos 1, 3, 5 y 6
  }
}

// Las pulsaciones solo valen en el momento en que se esperan
void olvidarPulsacionesFueraDeTiempo() {
  if (!(paso == 2 && fase == 1)) pulsado_start = false;
  if (!(paso == 4 && fase <= 1)) pulsado_flecha = false;
}

void secuencia(bool libre) {
  switch (paso) {
    case 0: paso0(libre); break;
    case 1: paso1(); break;
    case 2: paso2(libre); break;
    case 3: paso3(); break;
    case 4: paso4(libre); break;
    case 5: paso5(); break;
    case 6: paso6(); break;
  }
}

// ---------------------------------------------------------------- PASO 0: buscar el punto cero
void paso0(bool libre) {
  switch (fase) {
    case 0:   // al encender: espera pistón arriba + SET POINT (como Ver_7)
      if (V[V_SETPOINT] != 1) { setpoint_bloqueado = false; return; }
      if (!libre) { setpoint_bloqueado = true; return; }   // hay que volver a activarlo
      if (setpoint_bloqueado || !pistonArriba()) return;
      motorBuscarCero(SEMI_CERO_ARRANQUE);
      aFase(2);
      break;
    case 1:   // reinicio con PARO: espera a que el pistón esté arriba
      if (pistonArriba()) {
        motorBuscarCero(SEMI_CERO_ARRANQUE);
        aFase(2);
      } else if (limite == 0) {
        fallo(F_NO_SUBE);
      }
      break;
    case 2:
      if (motorOcupado()) return;
      if (motorFallo()) { fallo(F_SIN_CERO); return; }
      cero_ok = true;
      irA(1, 0);
      esperar(PAUSA_INCLINAR_MS);
      break;
  }
}

// ---------------------------------------------------------------- PASO 1: inclinar la bandeja
void paso1() {
  switch (fase) {
    case 0:
      if (!esperaHecha()) return;
      motorMover(DIR_AVANCE, PASOS_INCLINAR, velocidad());
      aFase(1);
      break;
    case 1:
      if (motorOcupado()) return;
      irA(2, 0);
      break;
  }
}

// ---------------------------------------------------------------- PASO 2: carga y START
void paso2(bool libre) {
  switch (fase) {
    case 0:   // entrada (como Ver_7: se borran START, VACÍO y flecha)
      V[V_START] = 0;
      V[V_VACIO_ON] = 0;
      V[V_FLECHA] = 0;
      pulsado_start = false;
      pulsado_flecha = false;
      aFase(1);
      break;
    case 1:   // carga de la hoja: máquina quieta, barrera ignorada
      if (V[V_VACIO_OFF] == 1) bomba(false);
      else if (V[V_VACIO_ON] == 1) bomba(true);
      if (!pulsado_start) return;
      pulsado_start = false;
      if (!vacio || !libre) return;          // sin vacío o con la barrera cortada no arranca
      marcha = true;
      motorBuscarCero(velocidad());
      aFase(2);
      break;
    case 2:   // vuelve al cero
      if (motorOcupado()) return;
      if (motorFallo()) { fallo(F_SIN_CERO); return; }
      motorMover(DIR_AVANCE, PASOS_GIRO, velocidad());
      aFase(3);
      break;
    case 3:   // giro de 180°
      if (motorOcupado()) return;
      aFase(4);
      esperar(segundosAms(ajustes.bajar));
      break;
    case 4:   // tiempo BAJAR
      if (!esperaHecha()) return;
      digitalWrite(PIN_EV_SUBIR, LOW);
      aFase(5);
      esperar(PAUSA_EV_PASO2_MS);
      break;
    case 5:
      if (!esperaHecha()) return;
      digitalWrite(PIN_EV_BAJAR, HIGH);
      irA(3, 0);
      break;
  }
}

// ---------------------------------------------------------------- PASO 3: primera bajada
void paso3() {
  switch (fase) {
    case 0:   // espera pistón abajo
      if (pistonAbajo()) {
        bomba(false);
        aFase(1);
        esperar(PAUSA_VACIO_MS);
      } else if (limite == 0) {
        fallo(F_NO_BAJA);
      }
      break;
    case 1:
      if (!esperaHecha()) return;
      digitalWrite(PIN_SOPLO, HIGH);
      aFase(2);
      esperar(segundosAms(ajustes.subir));
      break;
    case 2:   // tiempo SUBIR
      if (!esperaHecha()) return;
      digitalWrite(PIN_EV_BAJAR, LOW);
      aFase(3);
      esperar(PAUSA_EV_PASO3_MS);
      break;
    case 3:
      if (!esperaHecha()) return;
      digitalWrite(PIN_EV_SUBIR, HIGH);
      digitalWrite(PIN_SOPLO, HIGH);
      aFase(4);
      esperar(segundosAms(ajustes.soplo));
      break;
    case 4:   // tiempo SOPLADO (subiendo)
      if (!esperaHecha()) return;
      digitalWrite(PIN_SOPLO, LOW);
      pulsado_flecha = false;               // la flecha vale desde aquí
      irA(4, 0);
      break;
  }
}

// ---------------------------------------------------------------- PASO 4: flecha PASO 2 y segundo giro
void paso4(bool libre) {
  switch (fase) {
    case 0:   // espera pistón arriba
      if (pistonArriba()) aFase(1);
      else if (limite == 0) fallo(F_NO_SUBE);
      break;
    case 1:   // espera la flecha: máquina quieta, barrera ignorada
      if (!pulsado_flecha) return;
      pulsado_flecha = false;
      if (!libre) return;                   // con la barrera cortada no arranca
      motorMover(DIR_AVANCE, PASOS_GIRO, velocidad());
      aFase(2);
      break;
    case 2:
      if (motorOcupado()) return;
      aFase(3);
      esperar(segundosAms(ajustes.bajar));
      break;
    case 3:   // tiempo BAJAR
      if (!esperaHecha()) return;
      digitalWrite(PIN_EV_SUBIR, LOW);
      aFase(4);
      esperar(PAUSA_EV_PASO4_MS);
      break;
    case 4:
      if (!esperaHecha()) return;
      digitalWrite(PIN_EV_BAJAR, HIGH);
      irA(5, 0);
      break;
  }
}

// ---------------------------------------------------------------- PASO 5: segunda bajada
void paso5() {
  switch (fase) {
    case 0:
      if (pistonAbajo()) {
        aFase(1);
        esperar(segundosAms(ajustes.subir));
      } else if (limite == 0) {
        fallo(F_NO_BAJA);
      }
      break;
    case 1:   // tiempo SUBIR
      if (!esperaHecha()) return;
      digitalWrite(PIN_EV_BAJAR, LOW);
      aFase(2);
      esperar(PAUSA_EV_PASO5_MS);
      break;
    case 2:
      if (!esperaHecha()) return;
      digitalWrite(PIN_EV_SUBIR, HIGH);
      aFase(3);
      esperar(PAUSA_EV_PASO5_MS);
      break;
    case 3:
      if (!esperaHecha()) return;
      irA(6, 0);
      break;
  }
}

// ---------------------------------------------------------------- PASO 6: volver a inclinar
void paso6() {
  switch (fase) {
    case 0:
      if (pistonArriba()) {
        motorMover(DIR_AVANCE, PASOS_INCLINAR, velocidad());
        aFase(1);
      } else if (limite == 0) {
        fallo(F_NO_SUBE);
      }
      break;
    case 1:
      if (motorOcupado()) return;
      marcha = false;
      ciclos++;
      irA(2, 0);
      break;
  }
}

// ----------------------------------------------------------------------------- actuadores
void bomba(bool on) {
  vacio = on;
  digitalWrite(PIN_VACIO, on ? HIGH : LOW);
}
