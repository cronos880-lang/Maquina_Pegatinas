// Ejecuta el sketch Pegatinas_Ver_8 REAL en el PC con un simulador de la máquina:
// bandeja (motor paso a paso + sensor de cero), pistón neumático con sus sensores,
// aire general (pin 11) y barrera (pin 12).
// Uso: ./test_sketch <escenario>    (lo lanza ejecutar_pruebas.sh con todos los escenarios)
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "Arduino.h"
#include "EEPROM.h"

uint32_t g_micros = 0;
uint8_t g_pin[80];
int g_pwm[80];
SerialPrueba Serial, Serial1;
EEPROMPrueba EEPROM;

#include "sketch_combinado.cpp"

// ----------------------------------------------------------------------------- máquina simulada
struct Maquina {
  long bandeja = 137;          // posición en pasos (200 = 1 vuelta); arranca en cualquier sitio
  uint8_t step_ant = LOW;
  long pasos_totales = 0;
  double piston = 0;           // 0 = arriba · 1 = abajo
  double recorrido_ms = 400;   // tiempo de subida o bajada
  bool barrera_libre = true;
  bool sensor_cero_roto = false;
  bool piston_atascado = false;
  long max_semi_us = 0;        // para medir la velocidad: tiempo entre flancos
  uint32_t t_flanco = 0;
  uint32_t min_semi_us = 0xFFFFFFFF;
} M;

static long modulo(long a, long m) { return ((a % m) + m) % m; }

static void entradas() {
  g_pin[PIN_CERO] = (!M.sensor_cero_roto && modulo(M.bandeja, 200) == 0) ? LOW : HIGH;
  g_pin[PIN_PISTON_ARRIBA] = M.piston <= 0.0 ? LOW : HIGH;
  g_pin[PIN_PISTON_ABAJO] = M.piston >= 1.0 ? LOW : HIGH;
  g_pin[PIN_BARRERA] = M.barrera_libre ? HIGH : LOW;
}

static void fisica(uint32_t dt_us) {
  if (g_pin[PIN_STEP] != M.step_ant) {
    uint32_t semi = g_micros - M.t_flanco;
    if (M.t_flanco && semi < M.min_semi_us) M.min_semi_us = semi;
    M.t_flanco = g_micros;
    if (g_pin[PIN_STEP] == HIGH) {
      M.bandeja += (g_pin[PIN_DIR] == HIGH) ? -1 : 1;
      M.pasos_totales++;
    }
    M.step_ant = g_pin[PIN_STEP];
  }
  bool aire = g_pin[PIN_AIRE] == HIGH;
  if (aire && !M.piston_atascado) {
    double d = dt_us / 1000.0 / M.recorrido_ms;
    if (g_pin[PIN_EV_BAJAR] == HIGH && g_pin[PIN_EV_SUBIR] == LOW) M.piston += d;
    if (g_pin[PIN_EV_SUBIR] == HIGH && g_pin[PIN_EV_BAJAR] == LOW) M.piston -= d;
    if (M.piston < 0) M.piston = 0;
    if (M.piston > 1) M.piston = 1;
  }
}

const uint32_t TICK_US = 250;

static void correr(uint32_t ms) {
  uint64_t fin = (uint64_t)g_micros + (uint64_t)ms * 1000;
  while (g_micros < fin) {
    entradas();
    loop();
    g_micros += TICK_US;
    fisica(TICK_US);
  }
}

// Corre hasta que se cumpla la condición (o se acabe el tiempo). Devuelve si se cumplió.
template <typename C> static bool hasta(C cond, uint32_t max_ms) {
  uint64_t fin = (uint64_t)g_micros + (uint64_t)max_ms * 1000;
  while (g_micros < fin) {
    if (cond()) return true;
    entradas();
    loop();
    g_micros += TICK_US;
    fisica(TICK_US);
  }
  return cond();
}

static void enviar(const char* cmd) { for (const char* p = cmd; *p; p++) Serial1.entrada.push_back(*p); }
static void escribir(int v, float x) { char b[40]; std::snprintf(b, sizeof b, "!V%02d=%g$", v, x); enviar(b); }
static void pulsar(int v) { escribir(v, 1); correr(20); escribir(v, 0); correr(20); }   // pulsador de Virtuino

static int ok = 0, fallos = 0;
#define COMPROBAR(c, txt) do { if (c) ok++; else { fallos++; std::printf("  FALLO: %s (linea %d)\n", txt, __LINE__); } } while (0)

static bool en(int p, int f) { return modo == M_NORMAL && paso == p && fase == f; }
static int leds_encendidos() { int n = 0; for (int i = 0; i < 7; i++) n += V[20 + i] == 1; return n; }

static void arrancar() {
  std::memset(EEPROM.mem, 0xFF, sizeof EEPROM.mem);
  entradas();
  setup();
}

// Arranque + SET POINT hasta quedar en carga
static void hasta_carga() {
  arrancar();
  correr(100);
  escribir(V_SETPOINT, 1);
  hasta([] { return en(2, 1); }, 10000);
}

// Carga con vacío + START hasta que el pistón está abajo por primera vez
static void empezar_ciclo() {
  escribir(V_VACIO_ON, 1); correr(300);
  pulsar(V_START);
}

int main(int argc, char** argv) {
  std::string e = argc > 1 ? argv[1] : "";

  if (e == "arranque") {
    arrancar(); correr(2000);
    COMPROBAR(g_pin[PIN_AIRE] == HIGH && g_pin[PIN_EV_SUBIR] == HIGH && g_pin[PIN_EV_BAJAR] == LOW, "aire dado y pistón arriba al arrancar");
    COMPROBAR(g_pin[PIN_VACIO] == LOW && g_pin[PIN_SOPLO] == LOW, "vacío y soplo apagados");
    COMPROBAR(M.pasos_totales == 0, "sin SET POINT la bandeja no se mueve");
    COMPROBAR(V[V_T_SOPLO] == 3 && V[V_T_SUBIR] == 1 && V[V_T_BAJAR] == 0, "tiempos por defecto de Ver_7");
    COMPROBAR(V[V_VERSION] == 800 && V[V_VEL_MANDO] == 8000 && V[V_MODO_VEL] == 0, "versión y velocidad fija");
    COMPROBAR(V[20] == 1 && leds_encendidos() == 1, "LED del paso 0");
    escribir(V_SETPOINT, 1);
    COMPROBAR(hasta([] { return en(2, 1); }, 10000), "SET POINT: busca el cero, inclina y queda en carga");
    COMPROBAR(modulo(M.bandeja, 200) == 25, "bandeja inclinada 25 pasos desde el cero");
    COMPROBAR(V[V_CERO_OK] == 1 && V[22] == 1 && leds_encendidos() == 1, "LED cero y LED del paso 2");

  } else if (e == "ciclo") {
    hasta_carga();
    long n0 = M.pasos_totales;
    pulsar(V_START);
    correr(1000);
    COMPROBAR(en(2, 1) && M.pasos_totales == n0, "sin vacío START no arranca");
    COMPROBAR(V[V_MARCHA] == 0 && modulo(M.bandeja, 200) == 25, "sigue en carga");
    empezar_ciclo();
    COMPROBAR(V[V_MARCHA] == 1 && V[V_VACIO] == 1, "MARCHA y VACÍO encendidos");
    COMPROBAR(hasta([] { return paso == 3; }, 5000), "giro de 180° y bajada");
    COMPROBAR(modulo(M.bandeja, 200) == 100, "bandeja a 180°");
    COMPROBAR(hasta([] { return paso == 4 && fase == 1; }, 10000), "sopla, sube y espera la flecha");
    COMPROBAR(g_pin[PIN_VACIO] == LOW && g_pin[PIN_SOPLO] == LOW, "vacío y soplo apagados al esperar la flecha");
    COMPROBAR(V[24] == 1 && leds_encendidos() == 1, "solo el LED del paso 4");
    correr(3000);
    COMPROBAR(paso == 4 && fase == 1, "sin flecha se queda esperando");
    pulsar(V_FLECHA);
    COMPROBAR(hasta([] { return en(2, 1); }, 10000), "segundo giro, segunda bajada y vuelta a carga");
    COMPROBAR(modulo(M.bandeja, 200) == 25, "vuelve a la posición de carga");
    COMPROBAR(V[V_MARCHA] == 0, "LED MARCHA apagado al acabar (fallo M1 de Ver_7)");
    COMPROBAR(V[22] == 1 && leds_encendidos() == 1 && V[26] == 0, "solo el LED del paso 2 (fallo M2 de Ver_7)");
    COMPROBAR(V[V_CICLOS] == 1, "contador de ciclos");
    COMPROBAR(V[V_START] == 0 && V[V_FLECHA] == 0 && V[V_VACIO_ON] == 0, "START, flecha y vacío borrados en carga (como Ver_7)");

  } else if (e == "tiempos_ciclo") {
    hasta_carga();
    escribir(V_T_SOPLO, 2); escribir(V_T_SUBIR, 1.5); escribir(V_T_BAJAR, 0.5); correr(100);
    empezar_ciclo();
    hasta([] { return paso == 2 && fase == 4; }, 5000);
    uint32_t t0 = g_micros;
    hasta([] { return paso == 3; }, 5000);
    double bajar = (g_micros - t0) / 1000.0;
    COMPROBAR(bajar >= 590 && bajar <= 620, "BAJAR 0,5 s + 100 ms entre válvulas");
    hasta([] { return paso == 3 && fase == 2; }, 5000);
    t0 = g_micros;
    hasta([] { return paso == 3 && fase == 3; }, 5000);
    double subir = (g_micros - t0) / 1000.0;
    COMPROBAR(subir >= 1495 && subir <= 1510, "SUBIR 1,5 s");
    hasta([] { return paso == 3 && fase == 4; }, 5000);
    t0 = g_micros;
    hasta([] { return paso == 4; }, 5000);
    double soplo = (g_micros - t0) / 1000.0;
    COMPROBAR(soplo >= 1995 && soplo <= 2010, "SOPLADO 2 s");

  } else if (e == "flecha_temprana") {
    hasta_carga();
    M.recorrido_ms = 6000;                    // pistón lento: la flecha llega antes de que esté arriba
    empezar_ciclo();
    hasta([] { return paso == 4 && fase == 0; }, 20000);
    pulsar(V_FLECHA);
    COMPROBAR(paso == 4 && fase == 0, "el pistón aún sube");
    COMPROBAR(hasta([] { return paso == 5; }, 10000), "la flecha vale a la primera (fallo M5 de Ver_7)");

  } else if (e == "barrera_carga") {
    hasta_carga();
    M.barrera_libre = false;                  // mano dentro para poner la hoja
    escribir(V_VACIO_ON, 1); correr(500);
    COMPROBAR(g_pin[PIN_AIRE] == HIGH && g_pin[PIN_VACIO] == HIGH, "en carga la barrera no corta el aire y el vacío funciona");
    COMPROBAR(modo == M_NORMAL && en(2, 1), "no entra en pausa");
    pulsar(V_START); correr(500);
    COMPROBAR(en(2, 1) && V[V_MARCHA] == 0, "START con la barrera cortada: no arranca");
    M.barrera_libre = true; correr(1000);
    COMPROBAR(en(2, 1), "al liberar la barrera no arranca sola");
    pulsar(V_START);
    COMPROBAR(hasta([] { return paso == 3; }, 5000), "START con la barrera libre: arranca");

  } else if (e == "barrera_giro") {
    hasta_carga();
    empezar_ciclo();
    hasta([] { return paso == 2 && fase == 3 && motor.hechos == 40; }, 5000);
    M.barrera_libre = false; correr(50);
    long pos = M.bandeja;
    correr(2000);
    COMPROBAR(modo == M_PAUSA && M.bandeja == pos, "barrera cortada en el giro: pausa y motor parado");
    COMPROBAR(g_pin[PIN_AIRE] == LOW, "aire cortado en pausa");
    M.barrera_libre = true; correr(2000);
    COMPROBAR(modo == M_PAUSA && M.bandeja == pos && g_pin[PIN_AIRE] == LOW, "al liberar la barrera sigue parada (no arranca sola)");
    pulsar(V_START);
    COMPROBAR(modo == M_NORMAL && g_pin[PIN_AIRE] == HIGH, "START reanuda");
    hasta([] { return paso == 3; }, 5000);
    COMPROBAR(modulo(M.bandeja, 200) == 100, "termina el giro en su sitio (180°)");

  } else if (e == "barrera_tiempo") {
    hasta_carga();
    escribir(V_T_SUBIR, 2); correr(100);
    empezar_ciclo();
    hasta([] { return paso == 3 && fase == 2; }, 10000);
    correr(500);                              // 0,5 s de los 2 s
    M.barrera_libre = false; correr(5000);
    COMPROBAR(modo == M_PAUSA && paso == 3 && fase == 2, "pausa durante el tiempo SUBIR");
    COMPROBAR(g_pin[PIN_EV_BAJAR] == HIGH && M.piston >= 1.0, "el pistón no se mueve");
    M.barrera_libre = true; correr(200);
    pulsar(V_START);
    uint32_t t0 = g_micros;
    hasta([] { return paso == 3 && fase == 3; }, 5000);
    double resto = (g_micros - t0) / 1000.0;
    COMPROBAR(resto >= 1400 && resto <= 1520, "el tiempo se congeló: faltaba 1,5 s");

  } else if (e == "barrera_flecha") {
    hasta_carga();
    empezar_ciclo();
    hasta([] { return paso == 4 && fase == 1; }, 20000);
    M.barrera_libre = false; correr(1000);
    COMPROBAR(modo == M_NORMAL && g_pin[PIN_AIRE] == HIGH, "esperando la flecha la barrera se ignora");
    pulsar(V_FLECHA); correr(500);
    COMPROBAR(paso == 4 && fase == 1 && modulo(M.bandeja, 200) == 100, "flecha con la barrera cortada: no gira");
    M.barrera_libre = true; correr(500);
    COMPROBAR(paso == 4 && fase == 1, "al liberar no gira sola");
    pulsar(V_FLECHA);
    COMPROBAR(hasta([] { return paso == 5; }, 5000), "flecha con la barrera libre: gira");

  } else if (e == "paro") {
    hasta_carga();
    empezar_ciclo();
    hasta([] { return paso == 2 && fase == 3 && motor.hechos == 30; }, 5000);
    pulsar(V_PARO);
    long pos = M.bandeja;
    correr(2000);
    COMPROBAR(modo == M_PARADO && M.bandeja == pos, "PARO: motor parado al momento");
    COMPROBAR(g_pin[PIN_VACIO] == LOW && V[V_VACIO] == 0, "PARO: vacío apagado");
    COMPROBAR(g_pin[PIN_SOPLO] == LOW && g_pin[PIN_EV_SUBIR] == HIGH && g_pin[PIN_EV_BAJAR] == LOW, "PARO: sin soplo y pistón arriba");
    COMPROBAR(V[V_MARCHA] == 0 && V[V_CERO_OK] == 0 && V[V_PARO] == 0, "PARO: MARCHA apagado y botón vuelto a 0");
    bool on20 = false, off20 = false;
    for (int i = 0; i < 8; i++) { correr(250); if (V[20] == 1) on20 = true; else off20 = true; }
    COMPROBAR(on20 && off20 && V[22] == 0, "parado: parpadea el LED de inicio de secuencia");
    pulsar(V_START); correr(1000);
    COMPROBAR(modo == M_PARADO && M.bandeja == pos, "parado: START no hace nada");
    pulsar(V_PARO);
    COMPROBAR(hasta([] { return en(2, 1); }, 10000), "segundo PARO: busca el cero, inclina y queda en carga");
    COMPROBAR(modulo(M.bandeja, 200) == 25 && V[V_CERO_OK] == 1, "posición de carga correcta");
    COMPROBAR(V[V_T_SOPLO] == 3, "no se pierden los ajustes");
    empezar_ciclo();
    pulsar(V_PARO);
    COMPROBAR(modo == M_PARADO, "PARO también durante la búsqueda de cero del ciclo");

  } else if (e == "paro_en_pausa") {
    hasta_carga();
    empezar_ciclo();
    hasta([] { return paso == 3 && fase == 2; }, 10000);
    M.barrera_libre = false; correr(200);
    pulsar(V_PARO); correr(200);
    COMPROBAR(modo == M_PARADO, "PARO en pausa: queda parada");
    COMPROBAR(g_pin[PIN_AIRE] == LOW && M.piston >= 1.0, "con la barrera cortada y el pistón abajo, el aire sigue cortado");
    pulsar(V_PARO); correr(200);
    COMPROBAR(modo == M_PARADO, "reinicio con la barrera cortada: no hace nada");
    M.barrera_libre = true;
    COMPROBAR(hasta([] { return M.piston <= 0.0; }, 2000), "al liberar la barrera vuelve el aire y el pistón sube");
    COMPROBAR(modo == M_PARADO && M.pasos_totales > 0, "sigue parada esperando el reinicio");
    pulsar(V_PARO);
    COMPROBAR(hasta([] { return en(2, 1); }, 10000), "reinicio: vuelve a carga");

  } else if (e == "fallo_piston") {
    hasta_carga();
    M.piston_atascado = true;
    empezar_ciclo();
    hasta([] { return paso == 3; }, 5000);
    correr(7000);
    COMPROBAR(modo == M_NORMAL && paso == 3, "aún dentro del tiempo límite");
    correr(1500);
    COMPROBAR(modo == M_PARADO && V[V_FALLO] == F_NO_BAJA, "pistón que no baja: parada con fallo 1");
    bool vio_on = false, vio_off = false;
    for (int i = 0; i < 8; i++) { correr(250); if (V[23] == 1) vio_on = true; else vio_off = true; }
    COMPROBAR(vio_on && vio_off && V[20] == 0, "parpadea el LED del paso donde se quedó (paso 3)");
    M.piston_atascado = false;
    pulsar(V_PARO);
    COMPROBAR(hasta([] { return en(2, 1); }, 10000) && V[V_FALLO] == 0, "PARO reinicia y borra el fallo");

  } else if (e == "fallo_cero") {
    arrancar(); correr(100);
    M.sensor_cero_roto = true;
    escribir(V_SETPOINT, 1);
    correr(8000);
    COMPROBAR(modo == M_PARADO && V[V_FALLO] == F_SIN_CERO, "sin sensor de cero: para tras una vuelta");
    COMPROBAR(M.pasos_totales == PASOS_MAX_CERO, "no gira más de 250 pasos");

  } else if (e == "setpoint_barrera") {
    arrancar(); correr(100);
    M.barrera_libre = false;
    escribir(V_SETPOINT, 1); correr(1000);
    COMPROBAR(M.pasos_totales == 0, "SET POINT con la barrera cortada: no se mueve");
    M.barrera_libre = true; correr(1000);
    COMPROBAR(M.pasos_totales == 0, "al liberar la barrera no arranca sola");
    escribir(V_SETPOINT, 0); correr(100); escribir(V_SETPOINT, 1);
    COMPROBAR(hasta([] { return en(2, 1); }, 10000), "volviendo a activar SET POINT: arranca");

  } else if (e == "barrera_cero") {
    arrancar(); correr(100);
    escribir(V_SETPOINT, 1);
    hasta([] { return M.pasos_totales == 10; }, 5000);
    M.barrera_libre = false; correr(50);
    long n = M.pasos_totales; correr(1000);
    COMPROBAR(modo == M_PAUSA && M.pasos_totales == n, "barrera en la búsqueda del cero: pausa (Ver_7 no la miraba)");
    M.barrera_libre = true; pulsar(V_START);
    COMPROBAR(hasta([] { return en(2, 1); }, 10000) && modulo(M.bandeja, 200) == 25, "START reanuda y llega a carga");

  } else if (e == "ajustes") {
    arrancar(); correr(100);
    escribir(V_T_SOPLO, 4.5); correr(500);
    COMPROBAR(ajustes.soplo == 4.5f, "tiempo nuevo aceptado");
    int antes = EEPROM.escrituras;
    correr(1000);
    COMPROBAR(EEPROM.escrituras == antes, "no guarda mientras se mueve el deslizador");
    correr(1500);
    COMPROBAR(EEPROM.escrituras > antes, "guarda 2 s después del último cambio");
    escribir(V_T_SUBIR, 99); correr(100);
    COMPROBAR(V[V_T_SUBIR] == 1 && ajustes.subir == 1, "tiempo fuera de límites: se rechaza");
    escribir(V_T_BAJAR, -2); correr(100);
    COMPROBAR(V[V_T_BAJAR] == 0, "tiempo negativo: se rechaza");
    correr(3000);
    // reinicio (seta): los tiempos se recuperan
    std::memset(V, 0, sizeof V);
    entradas(); setup(); correr(100);
    COMPROBAR(V[V_T_SOPLO] == 4.5f && V[V_T_SUBIR] == 1, "tras un reset los tiempos se conservan");
    EEPROM.mem[DIR_EEPROM + 3] ^= 0x55;       // EEPROM estropeada
    std::memset(V, 0, sizeof V); setup(); correr(100);
    COMPROBAR(V[V_T_SOPLO] == 3, "EEPROM estropeada: valores por defecto");

  } else if (e == "velocidad") {
    hasta_carga();
    escribir(V_VEL_MANDO, 6500); correr(100);
    COMPROBAR(velocidad() == SEMI_FIJA && V[V_VEL_ACTUAL] == 8000, "modo fijo: el mando no cambia la velocidad");
    M.min_semi_us = 0xFFFFFFFF;
    empezar_ciclo();
    hasta([] { return paso == 3; }, 5000);
    COMPROBAR(M.min_semi_us >= 7900 && M.min_semi_us <= 8300, "giros a 8000 µs como Ver_7");
    escribir(V_MODO_VEL, 1); correr(100);
    COMPROBAR(velocidad() == 6500 && V[V_VEL_ACTUAL] == 6500, "modo mando: usa el mando");
    escribir(V_VEL_MANDO, 1000); correr(100);
    COMPROBAR(V[V_VEL_MANDO] == SEMI_MIN, "mando demasiado rápido: se limita a 6000");
    escribir(V_VEL_MANDO, 50000); correr(100);
    COMPROBAR(V[V_VEL_MANDO] == SEMI_MAX, "mando demasiado lento: se limita a 12000");
    escribir(V_MODO_VEL, 7); correr(100);
    COMPROBAR(V[V_MODO_VEL] == 1, "modo no válido: se rechaza");

  } else if (e == "rampa") {
    hasta_carga();
    empezar_ciclo();
    // Primeros semiperiodos del giro de 180°: empiezan lentos y bajan a 8000
    hasta([] { return paso == 2 && fase == 3; }, 5000);
    uint32_t t_ant = g_micros; uint8_t s_ant = g_pin[PIN_STEP];
    std::vector<uint32_t> semis;
    while (paso == 2 && fase == 3 && g_micros < 60u * 1000000u) {
      entradas(); loop(); g_micros += TICK_US; fisica(TICK_US);
      if (g_pin[PIN_STEP] != s_ant) { semis.push_back(g_micros - t_ant); t_ant = g_micros; s_ant = g_pin[PIN_STEP]; }
    }
    COMPROBAR(semis.size() >= 190, "giro completo");
    COMPROBAR(semis.size() > 10 && semis[1] > 10000 && semis[semis.size() / 2] < 8300, "arranque suave y velocidad de crucero 8000");
    COMPROBAR(semis.back() > 9500 || semis[semis.size() - 2] > 9500, "frenada suave al final");

  } else if (e == "bluetooth") {
    hasta_carga();
    empezar_ciclo();
    hasta([] { return paso == 2 && fase == 3 && motor.hechos == 20; }, 5000);
    Serial1.salida.clear();
    enviar("!V22=?$");
    correr(5);
    COMPROBAR(Serial1.salida.find("!V22=1.00$") != std::string::npos, "responde en menos de 5 ms aunque gire el motor");
    // comando que llega a trozos: no frena el motor
    M.min_semi_us = 0xFFFFFFFF;
    long antes = M.pasos_totales;
    enviar("!V1"); correr(30); enviar("6=?$"); correr(300);
    COMPROBAR(M.pasos_totales - antes >= 17, "el motor no se para mientras llega un comando");

  } else {
    std::printf("escenario desconocido: %s\n", e.c_str());
    return 2;
  }

  std::printf("%-18s %d correctas, %d fallos\n", e.c_str(), ok, fallos);
  return fallos ? 1 : 0;
}
