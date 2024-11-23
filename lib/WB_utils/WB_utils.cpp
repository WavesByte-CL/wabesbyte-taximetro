#include "WString.h"
// Boleto.cpp
#include "WB_utils.h"

void imprimirBoleto(
  HardwareSerial& serial, 
  String patente,
  String serial_number,
  String fecha,
  String horaInicio,
  String horaFin,
  String bajadaBanderaPrecio,
  String cada200MetrosPrecio,
  String cada60SegundosPrecio,
  String totalBajadaBandera,
  String totalMetros,
  String totalPrecioMetros,
  String totalMinutos,
  String totalPrecioMinutos,
  String totalCobrado,
  String metrosControl,
  String minutosControl,
  String segundosControl,
  String propaganda1,
  String propaganda2,
  String propaganda3,
  String propaganda4
) {
    const String boleto[] = {
        " \x1B\x14",
        "       " + patente + "      #" + serial_number + "    ",
        "                                ",
        "       Fecha     " + fecha + "     ",
        "       De   " + horaInicio + "  a  " + horaFin + "     ",
        "              TARIFA            ",
        "       B. BANDERA     $" + bajadaBanderaPrecio + "     ",
        "       C/200 mts      $" + cada200MetrosPrecio + "     ",
        "       C/60 seg       $" + cada60SegundosPrecio + "     ",
        "                                ",
        "           TOTAL A PAGAR        ",
        "       B. BANDERA   $" + totalBajadaBandera + "     ",
        "       " + totalMetros + "mts    $" + totalPrecioMetros + "     ",
        "       " + totalMinutos + "min    $" + totalPrecioMinutos + "     ",
        "                  +--------    ",
        "       TOTAL \x1B\x0E$" + totalCobrado + "",
        "\x1B\x14",
        " ",
        " ",
        "       " + propaganda1,
        "       " + propaganda2,
        "       " + propaganda3,
        "       " + propaganda4,
        " ",
        " ",
        " ",
    };
    
    for(const auto& linea : boleto) {
        serial.println(linea);
    }
}

void imprimirBoletoControl(
  HardwareSerial& serial, 
  String patente,
  String serial_number,
  String fecha,
  String horaInicio,
  String horaFin,
  String bajadaBanderaPrecio,
  String cada200MetrosPrecio,
  String cada60SegundosPrecio,
  String totalBajadaBandera,
  String totalMetros,
  String totalPrecioMetros,
  String totalMinutos,
  String totalPrecioMinutos,
  String totalCobrado,
  String metrosControl,
  String minutosControl,
  String segundosControl
) {
    const String boleto[] = {
        " \x1B\x14",
        "       " + patente + "      #" + serial_number + "    ",
        "                                ",
        "       Fecha     " + fecha + "     ",
        "       De   " + horaInicio + "  a  " + horaFin + "     ",
        "              TARIFA            ",
        "       B. BANDERA     $" + bajadaBanderaPrecio + "     ",
        "       C/200 mts      $" + cada200MetrosPrecio + "     ",
        "       C/60 seg       $" + cada60SegundosPrecio + "     ",
        "                                ",
        "           TOTAL A PAGAR        ",
        "       B. BANDERA   $" + totalBajadaBandera + "     ",
        "       " + totalMetros + "mts    $" + totalPrecioMetros + "     ",
        "       " + totalMinutos + "min    $" + totalPrecioMinutos + "     ",
        "                  +--------    ",
        "       TOTAL \x1B\x0E$" + totalCobrado + "",
        " ",
        "\x1B\x14              CONTROL           ",
        "       " + metrosControl + "mts  " + minutosControl + "min ",
        " ",
        " ",
        " ",
    };
    
    for(const auto& linea : boleto) {
        serial.println(linea);
    }
}


void imprimirBoletoVariables(
  HardwareSerial& serial, 
  String fecha,
  String horaInicio,
  String MODELO_TAXIMETRO,
  String PATENTE,
  String CANTIDAD_PULSOS,
  String METROS_POR_1_VUELTA_RUEDA,
  String RESOLUCION,
  String TARIFA_INICIAL,
  String TARIFA_CAIDA_PARCIAL_METROS,
  String TARIFA_CAIDA_PARCIAL_MINUTO
) {
    const String boleto[] = {
        " ",
        "PARAMETROS CONFIGURACION: ",
        "FECHA: " + fecha,
        "HORA: " + horaInicio,
        "MODELO_TAXIMETRO: " + MODELO_TAXIMETRO,
        "PATENTE: " + PATENTE,
        "CANTIDAD_PULSOS: " + CANTIDAD_PULSOS,
        "METROS_POR_1_VUELTA_RUEDA: " + METROS_POR_1_VUELTA_RUEDA,
        "RESOLUCION: " + RESOLUCION,
        "TARIFA_INICIAL: " + TARIFA_INICIAL,
        "TARIFA_CAIDA_PARCIAL_METROS: " + TARIFA_CAIDA_PARCIAL_METROS,
        "TARIFA_CAIDA_PARCIAL_MINUTO: " + TARIFA_CAIDA_PARCIAL_MINUTO,
        " ",
        " ",
        " ",
    };
    
    for(const auto& linea : boleto) {
        serial.println(linea);
    }
}
