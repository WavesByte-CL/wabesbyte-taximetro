// Boleto.h
#ifndef WB_UTILS_H
#define WB_UTILS_H

#include <Arduino.h>

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
);

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
);


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
);

#endif // BOLETO_H
