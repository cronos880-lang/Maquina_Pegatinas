// ============================================================================
//  AJUSTES: tiempos y velocidad. Se validan al llegar de la app y se guardan en la EEPROM
//  2 s después del último cambio (los deslizadores envían muchos valores seguidos).
// ============================================================================

// La estructura Ajustes está en Tipos.h (el IDE de Arduino necesita los tipos antes de las funciones)
const uint16_t MAGIA_AJUSTES = 0x5038;   // "P8"
const int DIR_EEPROM = 16;

Ajustes ajustes;
bool ajustes_pendientes = false;
unsigned long t_cambio_ajustes = 0;

uint8_t sumaAjustes(const Ajustes& a) {
  const uint8_t* p = (const uint8_t*)&a;
  uint8_t s = 0x5A;
  for (size_t i = 0; i < sizeof(Ajustes) - 1; i++) s = (uint8_t)(s * 31 + p[i]);
  return s;
}

bool tiempoValido(float s) { return s >= 0 && s <= TIEMPO_MAX_S; }

void ajustesPorDefecto() {
  ajustes.magia = MAGIA_AJUSTES;
  ajustes.soplo = 3;     // valores de Ver_7
  ajustes.subir = 1;
  ajustes.bajar = 0;
  ajustes.vel_mando = SEMI_FIJA;
  ajustes.modo_vel = 0;
}

void cargarAjustes() {
  EEPROM.get(DIR_EEPROM, ajustes);
  bool ok = ajustes.magia == MAGIA_AJUSTES && ajustes.suma == sumaAjustes(ajustes) &&
            tiempoValido(ajustes.soplo) && tiempoValido(ajustes.subir) && tiempoValido(ajustes.bajar) &&
            ajustes.vel_mando >= SEMI_MIN && ajustes.vel_mando <= SEMI_MAX && ajustes.modo_vel <= 1;
  if (!ok) {
    ajustesPorDefecto();
    guardarAjustes();
  }
  V[V_T_SOPLO] = ajustes.soplo;
  V[V_T_SUBIR] = ajustes.subir;
  V[V_T_BAJAR] = ajustes.bajar;
  V[V_VEL_MANDO] = ajustes.vel_mando;
  V[V_MODO_VEL] = ajustes.modo_vel;
}

void guardarAjustes() {
  ajustes.suma = sumaAjustes(ajustes);
  EEPROM.put(DIR_EEPROM, ajustes);
  ajustes_pendientes = false;
}

// Un tiempo fuera de límites no se acepta: se vuelve a enviar el anterior
bool revisarTiempo(uint8_t v, float& actual) {
  if (V[v] == actual) return false;
  if (tiempoValido(V[v])) { actual = V[v]; return true; }
  V[v] = actual;
  return false;
}

void vigilarAjustes(unsigned long ahora) {
  bool cambio = false;
  cambio |= revisarTiempo(V_T_SOPLO, ajustes.soplo);
  cambio |= revisarTiempo(V_T_SUBIR, ajustes.subir);
  cambio |= revisarTiempo(V_T_BAJAR, ajustes.bajar);

  // Mando de velocidad: se ajusta a los límites
  float m = V[V_VEL_MANDO];
  if (m != ajustes.vel_mando) {
    if (m < SEMI_MIN) m = SEMI_MIN;
    if (m > SEMI_MAX) m = SEMI_MAX;
    uint16_t nuevo = (uint16_t)(m + 0.5);
    V[V_VEL_MANDO] = nuevo;
    if (nuevo != ajustes.vel_mando) { ajustes.vel_mando = nuevo; cambio = true; }
  }

  float modoV = V[V_MODO_VEL];
  if (modoV != ajustes.modo_vel) {
    if (modoV == 0 || modoV == 1) { ajustes.modo_vel = (uint8_t)modoV; cambio = true; }
    else V[V_MODO_VEL] = ajustes.modo_vel;
  }

  if (cambio) { ajustes_pendientes = true; t_cambio_ajustes = ahora; }
  if (ajustes_pendientes && ahora - t_cambio_ajustes >= GUARDAR_TRAS_MS) guardarAjustes();
}
