// ============================================================================
//  MOTOR paso a paso de la bandeja, sin bloquear: un cambio de nivel del pin STEP cada
//  semiperiodo, desde loop(). Arranque y frenada suaves (rampa) en los giros.
// ============================================================================

struct Motor {
  bool activo = false;
  bool pausado = false;
  bool buscando = false;   // girar hasta el sensor de cero
  bool alto = false;       // nivel actual del pin STEP
  bool fallo = false;      // no se encontró el cero
  int objetivo = 0;
  int hechos = 0;
  unsigned int semi = SEMI_FIJA;
  unsigned long t = 0;
};
Motor motor;

void motorIniciar(uint8_t dir, int pasos, unsigned int semi, bool buscar) {
  digitalWrite(PIN_DIR, dir);
  digitalWrite(PIN_STEP, LOW);
  motor.activo = true;
  motor.pausado = false;
  motor.buscando = buscar;
  motor.alto = false;
  motor.fallo = false;
  motor.objetivo = pasos;
  motor.hechos = 0;
  motor.semi = semi;
  motor.t = micros();
}

void motorMover(uint8_t dir, int pasos, unsigned int semi) { motorIniciar(dir, pasos, semi, false); }
void motorBuscarCero(unsigned int semi) { motorIniciar(DIR_HACIA_CERO, PASOS_MAX_CERO, semi, true); }
bool motorOcupado() { return motor.activo; }
bool motorFallo() { return motor.fallo; }

void motorParar() {
  motor.activo = false;
  motor.alto = false;
  digitalWrite(PIN_STEP, LOW);
}

void motorPausar() { motor.pausado = true; }

void motorReanudar() {
  motor.pausado = false;
  motor.t = micros();
}

// Semiperiodo del paso actual con rampa de arranque y de frenada
unsigned int semiActual() {
  if (PASOS_RAMPA <= 0 || motor.semi >= SEMI_RAMPA) return motor.semi;
  int k = motor.hechos;
  if (!motor.buscando) {
    int quedan = motor.objetivo - motor.hechos - 1;
    if (quedan < k) k = quedan;
  }
  if (k < 0) k = 0;
  if (k >= PASOS_RAMPA) return motor.semi;
  unsigned long extra = (unsigned long)(SEMI_RAMPA - motor.semi) * (PASOS_RAMPA - k) / PASOS_RAMPA;
  return motor.semi + (unsigned int)extra;
}

void motorServicio() {
  if (!motor.activo || motor.pausado) return;
  if (!motor.alto) {                                   // antes de empezar un paso nuevo
    if (motor.buscando && digitalRead(PIN_CERO) == LOW) { motor.activo = false; return; }
    if (motor.hechos >= motor.objetivo) {
      motor.activo = false;
      motor.fallo = motor.buscando;                    // una vuelta entera sin ver el cero
      return;
    }
  }
  unsigned long ahora = micros();
  if (ahora - motor.t < semiActual()) return;
  motor.t = ahora;
  motor.alto = !motor.alto;
  digitalWrite(PIN_STEP, motor.alto ? HIGH : LOW);
  if (!motor.alto) motor.hechos++;
}

// Velocidad de los giros: fija (como Ver_7) o la del mando, según V32
unsigned int velocidad() {
  return ajustes.modo_vel == 1 ? ajustes.vel_mando : SEMI_FIJA;
}
