// ============================================================================
//  SEGURIDAD: barrera (pausa), botón PARO, fallos y corte de aire (pin 11).
//  La seta de emergencia va directa a la placa (reset) y no depende de este programa.
// ============================================================================

// Barrera cortada durante un movimiento: todo se congela
void entrarPausa() {
  modo = M_PAUSA;
  motorPausar();
  pulsado_start = false;          // una pulsación anterior no puede reanudar
}

// En pausa: al liberar la barrera NO sigue solo. START reanuda donde estaba.
void atenderPausa(bool libre) {
  if (!pulsado_start) return;
  pulsado_start = false;
  if (!libre) return;
  modo = M_NORMAL;
  motorReanudar();
}

// Botón PARO / REINICIO (V31)
void atenderParo(bool libre) {
  if (!pulsado_paro) return;
  pulsado_paro = false;
  if (modo == M_PARADO) {
    if (!libre) return;                       // para reiniciar, barrera libre
    fallo_codigo = F_NINGUNO;
    modo = M_NORMAL;
    irA(0, 1);                                // pistón arriba → cero → inclinar → carga
  } else if (modo == M_NORMAL && paso == 0 && fase <= 1) {
    // todavía sin referencia y sin moverse: no hay nada que parar
  } else {
    parar();
  }
}

// Parada: motor quieto, sin soplo, sin vacío y pistón arriba
void parar() {
  motorParar();
  digitalWrite(PIN_SOPLO, LOW);
  bomba(false);
  digitalWrite(PIN_EV_BAJAR, LOW);
  digitalWrite(PIN_EV_SUBIR, HIGH);
  marcha = false;
  cero_ok = false;
  modo = M_PARADO;
  pulsado_start = false;
  pulsado_flecha = false;
}

void fallo(uint8_t codigo) {
  paso_fallo = paso;
  parar();
  fallo_codigo = codigo;
}

// Pin 11: aire general
//  - Pausa: cortado.
//  - Movimiento: dado (si la barrera estuviera cortada ya estaríamos en pausa).
//  - Esperas y parada: dado si la barrera está libre o si el pistón ya está arriba
//    (entonces el aire no mueve nada). Así se puede cargar la hoja con la mano dentro.
//    Con la barrera cortada y el pistón a medio subir, el aire espera a que se libere.
void actualizarAire(bool libre) {
  bool aire;
  if (modo == M_PAUSA) aire = false;
  else if (vigilada()) aire = true;
  else aire = libre || pistonArriba();
  digitalWrite(PIN_AIRE, aire ? HIGH : LOW);
}
