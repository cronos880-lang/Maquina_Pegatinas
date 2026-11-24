// ============================================================================
//  COMUNICACIÓN con Virtuino por Bluetooth (Serial1) — protocolo VirtuinoCM.
//  Lectura sin esperas: los caracteres se van juntando y el comando se atiende cuando llega
//  el '$' final. Así el motor no se frena mientras llega un comando.
// ============================================================================

String comando = "";
bool en_comando = false;

// La app cambia un valor. Los botones se "recuerdan" en el momento en que llegan.
void alRecibir(char tipo, uint8_t indice, String valor) {
  if (tipo != 'V' || indice >= V_MEMORIA) return;
  float v = valor.toFloat();
  V[indice] = v;
  if (v != 1) return;
  if (indice == V_START) pulsado_start = true;
  else if (indice == V_FLECHA) pulsado_flecha = true;
  else if (indice == V_PARO) { pulsado_paro = true; V[V_PARO] = 0; }
}

// La app pide un valor
String alPedir(char tipo, uint8_t indice) {
  if (tipo == 'V' && indice < V_MEMORIA) return String(V[indice]);
  return "";
}

// Atiende como mucho un comando por pasada (igual que Ver_7), sin esperar a que llegue entero
void virtuinoRun() {
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == CM_START_CHAR) {
      comando = c;
      en_comando = true;
    } else if (en_comando) {
      comando += c;
      if (c == CM_END_CHAR) {
        en_comando = false;
        virtuino.readBuffer = comando;
        String* respuesta = virtuino.getResponse();
        Serial1.print(*respuesta);
        if (DEPURAR) {
          Serial.print(F("<- "));
          Serial.print(comando);
          Serial.print(F("  -> "));
          Serial.println(*respuesta);
        }
        comando = "";
        return;
      }
      if (comando.length() > 64) { en_comando = false; comando = ""; }   // basura
    }
  }
}

// ----------------------------------------------------------------------------- indicadores
void publicar(unsigned long ahora) {
  bool lento = (ahora / 500) % 2;   // parpadeo de 1 s

  for (uint8_t i = 0; i <= 6; i++) V[V_PASO0 + i] = (modo != M_PARADO && paso == i) ? 1 : 0;
  if (modo == M_PARADO) {
    if (fallo_codigo != F_NINGUNO) V[V_PASO0 + paso_fallo] = lento;   // dónde se ha quedado
    else V[V_PASO0] = lento;                                           // parado: pulsar PARO
  }

  if (modo == M_PAUSA) V[V_MARCHA] = lento;
  else V[V_MARCHA] = marcha ? 1 : 0;

  V[V_CERO_OK] = cero_ok ? 1 : 0;
  V[V_VACIO] = vacio ? 1 : 0;
  V[V_PISTON_ARRIBA] = pistonArriba() ? 1 : 0;
  V[V_PISTON_ABAJO] = pistonAbajo() ? 1 : 0;
  V[V_ESTADO] = modo * 100 + paso * 10 + fase;
  V[V_FALLO] = fallo_codigo;
  V[V_CICLOS] = ciclos;
  V[V_VEL_ACTUAL] = velocidad();
}
