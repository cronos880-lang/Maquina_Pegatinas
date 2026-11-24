#!/usr/bin/env bash
# Pruebas de Pegatinas_Ver_8 en el PC: el sketch real con un simulador de la máquina.
# Necesita g++, python3 y la librería VirtuinoCM original (se descarga si no está).
set -euo pipefail
cd "$(dirname "$0")"
TMP="${TMPDIR:-/tmp}/pegatinas_ver8_pruebas"
mkdir -p "$TMP/vcm"
if [ ! -f "$TMP/vcm/VirtuinoCM.cpp" ]; then
  for f in VirtuinoCM.h VirtuinoCM.cpp; do
    curl -fsSL -o "$TMP/vcm/$f" "https://raw.githubusercontent.com/iliaslamprou/virtuinoCM/master/$f"
  done
fi
python3 construir_sketch.py .. "$TMP/sketch_combinado.cpp"
g++ -std=c++17 -O1 -Wall -Wno-unused-parameter -Isimulado -I"$TMP/vcm" -I"$TMP" -I.. test_sketch.cpp "$TMP/vcm/VirtuinoCM.cpp" -o "$TMP/test_sketch"
fallo=0
for e in arranque ciclo tiempos_ciclo flecha_temprana barrera_carga barrera_giro barrera_tiempo barrera_flecha \
         paro paro_en_pausa fallo_piston fallo_cero setpoint_barrera barrera_cero ajustes velocidad rampa bluetooth; do
  "$TMP/test_sketch" "$e" || fallo=1
done
exit $fallo
