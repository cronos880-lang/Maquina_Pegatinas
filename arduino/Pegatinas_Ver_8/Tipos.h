// Tipos compartidos por las pestañas del sketch.
#pragma once
#include <stdint.h>

// Ajustes guardados en la EEPROM (ver Ajustes.ino)
struct Ajustes {
  uint16_t magia;
  float soplo;        // s
  float subir;        // s
  float bajar;        // s
  uint16_t vel_mando; // semiperiodo en µs
  uint8_t modo_vel;   // 0 fija · 1 mando
  uint8_t suma;
};
