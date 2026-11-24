// ============================================================================
//  MÁQUINA DE PEGATINAS · Ver_8 (8.0.0 de prueba)
//  Arduino Mega 2560 · Bluetooth en Serial1 (9600) · panel Virtuino (VirtuinoCM)
//
//  Basada en Pegatinas_14_01_2024_Ver_7_Full_Doble_Bajad__Barrera, que sigue siendo la
//  versión que funciona y la de vuelta atrás. Mismo cableado, mismos pines y mismo panel.
//  Cambios (ver LEEME.md):
//   - Programa sin bloqueos: el motor, los tiempos y el Bluetooth funcionan a la vez.
//   - Barrera vigilada continuamente durante los movimientos. Si se corta: PAUSA
//     (aire cortado, motor parado, tiempos congelados). Al liberarla NO sigue sola: START reanuda.
//   - Barrera ignorada mientras la máquina espera quieta (carga de la hoja, flecha PASO 2).
//   - Botón PARO (V31): para el ciclo; otra pulsación busca el cero y vuelve a carga.
//   - Tiempo límite en cada movimiento: si el pistón no llega o no aparece el cero, se para.
//   - Tiempos SOPLADO / SUBIR / BAJAR con límites y guardados en la EEPROM.
//   - Velocidad fija (8000) o desde el mando, según el interruptor de modo (V32).
//   - LEDs corregidos (MARCHA y pasos) y estado de los sensores del pistón.
//
//  Librerías: VirtuinoCM 1.0.02, EEPROM.
// ============================================================================

#include <EEPROM.h>
#include "VirtuinoCM.h"
#include "Tipos.h"

const int VERSION_FIRMWARE = 800;   // 8.0.0 (se publica en V50)
const bool DEPURAR = false;         // true = mensajes por el USB a 115200 baudios

// ----------------------------------------------------------------------------- pines (los de Ver_7)
const uint8_t PIN_CERO = 2;           // sensor de punto cero de la bandeja (LOW = en cero)
const uint8_t PIN_PISTON_ABAJO = 3;   // LOW = pistón abajo
const uint8_t PIN_PISTON_ARRIBA = 4;  // LOW = pistón arriba
const uint8_t PIN_EV_BAJAR = 5;       // electroválvula pistón: bajar
const uint8_t PIN_EV_SUBIR = 6;       // electroválvula pistón: subir
const uint8_t PIN_VACIO = 7;          // relé de la bomba de vacío
const uint8_t PIN_DIR = 8;            // motor: dirección
const uint8_t PIN_STEP = 9;           // motor: pasos
const uint8_t PIN_SOPLO = 10;         // electroválvula de soplado
const uint8_t PIN_AIRE = 11;          // corte general del aire (HIGH = aire, LOW = cortado)
const uint8_t PIN_BARRERA = 12;       // barrera (HIGH = libre, LOW = cortada)

// ----------------------------------------------------------------------------- bandeja
const uint8_t DIR_HACIA_CERO = HIGH;  // sentido de búsqueda del cero (como Ver_7)
const uint8_t DIR_AVANCE = LOW;       // sentido de inclinar y de los giros de 180°
const int PASOS_INCLINAR = 25;        // 45° (200 pasos = 1 vuelta)
const int PASOS_GIRO = 100;           // 180°
const int PASOS_MAX_CERO = 250;       // más de una vuelta sin ver el cero = fallo del sensor

// Semiperiodo del paso en microsegundos: más alto = más lento
const unsigned int SEMI_CERO_ARRANQUE = 12000;  // búsqueda lenta del cero (como Ver_7)
const unsigned int SEMI_FIJA = 8000;            // velocidad fija de Ver_7
const unsigned int SEMI_MIN = 6000;             // límites del mando de velocidad
const unsigned int SEMI_MAX = 12000;
const unsigned int SEMI_RAMPA = 12000;          // arranque y frenada suaves desde este valor
const int PASOS_RAMPA = 6;                      // 0 = sin rampa

// ----------------------------------------------------------------------------- tiempos
const unsigned long LIMITE_PISTON_MS = 8000;    // si el pistón no llega en este tiempo: fallo
const float TIEMPO_MAX_S = 30.0;                // límite de los deslizadores de tiempos
// Pausas fijas entre electroválvulas (las mismas de Ver_7)
const unsigned long PAUSA_VACIO_MS = 200;
const unsigned long PAUSA_EV_PASO2_MS = 100;
const unsigned long PAUSA_EV_PASO3_MS = 1000;
const unsigned long PAUSA_EV_PASO4_MS = 200;
const unsigned long PAUSA_EV_PASO5_MS = 300;
const unsigned long PAUSA_INCLINAR_MS = 200;
const unsigned long GUARDAR_TRAS_MS = 2000;     // guarda los ajustes 2 s después del último cambio

// ----------------------------------------------------------------------------- variables V
// Las de Ver_7 (el panel actual funciona igual)
const uint8_t V_CERO_OK = 0;          // LED: punto cero encontrado
const uint8_t V_PISTON_ARRIBA = 4;    // 1 = pistón arriba (nuevo uso: antes no se enviaba)
const uint8_t V_PISTON_ABAJO = 5;     // 1 = pistón abajo
const uint8_t V_MARCHA = 6;           // LED MARCHA (parpadea en pausa)
const uint8_t V_VACIO = 7;            // LED VACÍO
const uint8_t V_VEL_MANDO = 9;        // mando de VELOCIDAD (semiperiodo en µs)
const uint8_t V_START = 11;           // botón START (también reanuda tras una pausa)
const uint8_t V_SETPOINT = 13;        // interruptor SET POINT
const uint8_t V_VACIO_ON = 15;        // botón VACÍO
const uint8_t V_T_SOPLO = 16;         // tiempos en segundos
const uint8_t V_T_SUBIR = 17;
const uint8_t V_T_BAJAR = 18;
const uint8_t V_VACIO_OFF = 19;       // apagar vacío
const uint8_t V_PASO0 = 20;           // LEDs de los pasos: V20..V26
const uint8_t V_FLECHA = 30;          // botón flecha "PASO 2"
// Nuevas
const uint8_t V_PARO = 31;            // botón PARO / REINICIO
const uint8_t V_MODO_VEL = 32;        // 0 = velocidad fija (8000) · 1 = la del mando
const uint8_t V_VERSION = 50;         // 800 = 8.0.0
const uint8_t V_ESTADO = 51;          // modo×100 + paso×10 + fase
const uint8_t V_FALLO = 52;           // 0 sin fallo · 1 no baja · 2 no sube · 3 sin cero
const uint8_t V_CICLOS = 53;          // ciclos completos desde el arranque
const uint8_t V_VEL_ACTUAL = 54;      // semiperiodo que se está usando

const uint8_t F_NINGUNO = 0, F_NO_BAJA = 1, F_NO_SUBE = 2, F_SIN_CERO = 3;

// ----------------------------------------------------------------------------- estado
#define V_MEMORIA 250
VirtuinoCM virtuino;
float V[V_MEMORIA];

enum Modo : uint8_t { M_NORMAL = 0, M_PAUSA = 1, M_PARADO = 2 };
Modo modo = M_NORMAL;
uint8_t paso = 0;        // 0..6, como los pasos de Ver_7 (LED V20 + paso)
uint8_t fase = 0;        // sub-paso dentro de cada paso
unsigned long espera = 0;   // ms que faltan de la espera en curso (se congela en pausa)
unsigned long limite = 0;   // ms que le quedan al pistón para llegar (se congela en pausa)

bool marcha = false;     // ciclo en curso (LED MARCHA)
bool vacio = false;      // bomba de vacío encendida
bool cero_ok = false;
uint8_t fallo_codigo = F_NINGUNO;
uint8_t paso_fallo = 0;
bool setpoint_bloqueado = false;
unsigned long ciclos = 0;
unsigned long t_anterior = 0;

// Pulsaciones que llegan de la app (se guardan aunque el programa esté ocupado)
bool pulsado_start = false, pulsado_flecha = false, pulsado_paro = false;

// ============================================================================ SETUP
void setup() {
  if (DEPURAR) Serial.begin(115200);
  Serial1.begin(9600);
  virtuino.begin(alRecibir, alPedir, 256);

  pinMode(PIN_CERO, INPUT);
  pinMode(PIN_PISTON_ABAJO, INPUT_PULLUP);
  pinMode(PIN_PISTON_ARRIBA, INPUT_PULLUP);
  pinMode(PIN_BARRERA, INPUT_PULLUP);

  // Salidas: primero el nivel y después el modo, para que no haya pulsos al arrancar
  salida(PIN_EV_BAJAR, LOW);
  salida(PIN_EV_SUBIR, HIGH);   // pistón arriba, como Ver_7
  salida(PIN_VACIO, LOW);
  salida(PIN_SOPLO, LOW);
  salida(PIN_STEP, LOW);
  salida(PIN_DIR, DIR_HACIA_CERO);
  salida(PIN_AIRE, barreraLibre() ? HIGH : LOW);

  cargarAjustes();
  V[V_VERSION] = VERSION_FIRMWARE;
  t_anterior = millis();
  irA(0, 0);
}

void salida(uint8_t pin, uint8_t nivel) {
  digitalWrite(pin, nivel);
  pinMode(pin, OUTPUT);
}

// ============================================================================ LOOP
void loop() {
  virtuinoRun();

  unsigned long ahora = millis();
  unsigned long dt = ahora - t_anterior;
  t_anterior = ahora;
  bool libre = barreraLibre();

  atenderParo(libre);

  if (modo == M_NORMAL) {
    if (vigilada() && !libre) {
      entrarPausa();
    } else {
      descontar(dt);
      olvidarPulsacionesFueraDeTiempo();
      secuencia(libre);
    }
  } else if (modo == M_PAUSA) {
    atenderPausa(libre);
  } else {
    pulsado_start = false;
    pulsado_flecha = false;
  }

  motorServicio();
  actualizarAire(libre);
  vigilarAjustes(ahora);
  publicar(ahora);
}

// ----------------------------------------------------------------------------- sensores
bool barreraLibre() { return digitalRead(PIN_BARRERA) == HIGH; }
bool pistonArriba() { return digitalRead(PIN_PISTON_ARRIBA) == LOW; }
bool pistonAbajo() { return digitalRead(PIN_PISTON_ABAJO) == LOW; }
