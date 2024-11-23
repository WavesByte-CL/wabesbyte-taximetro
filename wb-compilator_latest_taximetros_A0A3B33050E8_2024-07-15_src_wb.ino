//////////////////////////////////////////////////////
////////////////////// LIBRERIAS  ////////////////////
//////////////////////////////////////////////////////
#include <Arduino.h>
#include "Ticker.h"
#include "TFT_eSPI.h"
#include <Wire.h>
#include <RTClib.h>
#include <driver/ledc.h>
#include <esp_system.h>
#include <WB_utils.h>
#include <Free_Fonts.h>
#include "config.h"
#include <Preferences.h>
#include <deque>
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////

bool isPaused = false;
bool printerFinish = false;
String SerialNumber = "";
float METROS_TIEMPO = 0;
float METROS_METROS = 0;

//////////////////////////////////////////////////////
//////////////////// CONFIGURACION  //////////////////
//////////////////////////////////////////////////////
Preferences preferences;
TFT_eSPI tft = TFT_eSPI();
RTC_DS3231 rtc;
HardwareSerial mySerial(2);
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////





//////////////////////////////////////////////////////
/////////////////////// SETUP  ///////////////////////
//////////////////////////////////////////////////////
void setup() {
  delay(300);
  Serial.begin(115200);
  mySerial.begin(9600, SERIAL_8N1, 16, 17);
  xTaskCreatePinnedToCore(loop0, "loop0", 10000, NULL, 1, NULL, 0);
  preferences.begin("my-app", false);
  pinMode(PINES.PinNightMode, OUTPUT);
  pinMode(PINES.PinPulse, INPUT_PULLUP);
  pinMode(PINES.PinButtonOn, INPUT_PULLUP);
  pinMode(PINES.PinButtonControl, INPUT_PULLUP);
  pinMode(PINES.PinButtonPrinter, INPUT_PULLUP);
  pinMode(PINES.PinPrinterPaperDetection, INPUT_PULLUP);
  pinMode(PINES.PinPulseLed, OUTPUT);
  pinMode(PINES.PinTimerLed, OUTPUT);
  pinMode(PINES.PinScreenLed, OUTPUT);
  pinMode(PINES.PinBuzzer, OUTPUT);

  

  String readValue = preferences.getString("mySerial", "ERROR");
  Serial.print("Valor 'mySerial' : ");
  SerialNumber = readValue;
  preferences.end();

  initiateEsp32();
  
}
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////





//////////////////////////////////////////////////////
/////////////////////// LOOP 1 ///////////////////////
//////////////////////////////////////////////////////
void loop() {
    if (!isInTimeChangeMode()) {
        handleButtonPresses();
        manageProgramLogic();
    } else {
        handleTimeChangeMode();
    }
}
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////


//////////////////////////////////////////////////////
/////////////////////// LOOP 0 ///////////////////////
//////////////////////////////////////////////////////
void loop0(void *parameter) {
  for (;;) {
    delay(1);
  }
}
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////





/*****************************************************/
/************** FUNCIONES DEL PROGRAMA ***************/
/*****************************************************/

//////////////////////////////////////////////////////////////
/////////////// Inicializacion y Configuracion ///////////////
//////////////////////////////////////////////////////////////
/**
 * @brief Inicializa el ESP32 y configura los componentes necesarios.
 * 
 * Esta funcion establece la razon del reinicio del ESP32, inicializa el reloj RTC, 
 * configura los pines y prepara la pantalla TFT para su uso.
 */
void initiateEsp32() {
  esp_reset_reason_t reason = esp_reset_reason();
  Serial.println(reason);

  if (!rtc.begin()) {
    Serial.println("No se encontro el modulo DS3231. Verifica las conexiones.");
    while (1);
  }
  ManageClockCurrentTime();
  ledcSetup(ledcChannel, ledcFrequency, ledcResolution);
  ledcAttachPin(PINES.PinBuzzer, ledcChannel);
  digitalWrite(PINES.PinScreenLed, LOW); //IDENTIFICAR EL PIN
  digitalWrite(PINES.PinTimerLed, LOW);
  digitalWrite(PINES.PinPulseLed, LOW);
  tft.begin();
  tft.fillScreen(SCREEN_LOGIC.ColorBackground);

  delay(100);

  switch (reason) {
  case ESP_RST_POWERON:
    Serial.println("Power-on reset");
    Serial.println(reason);
    PROGRAM_SETTINGS.BoolButtonOn = true;
    printerBoleto();
    PROGRAM_SETTINGS.BoolButtonOn = false;
    break;
  }

}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
/////////////// Manejo de eventos de Botones ///////////////
////////////////////////////////////////////////////////////
/**
 * @brief Gestiona las presiones de botones y dirige a las funciones especificas de manejo.
 * 
 * Esta funcion verifica si el sistema esta en el modo de cambio de tiempo. Si no lo esta, 
 * procede a comprobar el estado de los botones relacionados con el encendido, control y 
 * impresion. Luego, llama a las funciones correspondientes para manejar las acciones de cada boton.
 */
void handleButtonPresses() {
    if (!isInTimeChangeMode()) {
        if (isButtonPressed(PINES.PinButtonOn)) {
            digitalWrite(PINES.PinScreenLed, LOW);
            handleButtonOnPress();
        }
        if (isButtonPressed(PINES.PinButtonControl)) {
            digitalWrite(PINES.PinScreenLed, LOW);
            handleButtonControlPress();
        }
    }
    handleButtonPrinterPress();
}

/**
 * @brief Maneja la logica cuando se presiona el boton de encendido.
 * 
 * Esta funcion se activa cuando se presiona el boton de encendido. Realiza comprobaciones 
 * para asegurar que no se activa cuando la alarma de papel de la impresora esta activa o 
 * cuando otros modos de control estan en uso. Inicializa el estado del taximetro y muestra
 * la patente en la pantalla.
 */
void handleButtonOnPress() {
    static unsigned long lastPressTime = 0;
    unsigned long currentPressTime = millis();
    
    if (PROGRAM_SETTINGS.isPrinterPaperErrorActive) {
        return;
    }
    
    // Debounce: Solo actúa si han pasado al menos 500 ms desde la última presión del botón
    if (digitalRead(PINES.PinButtonOn) == LOW && (currentPressTime - lastPressTime > 1000)) {
        lastPressTime = currentPressTime; // Actualiza el tiempo de la última presión
        
        if (PROGRAM_SETTINGS.BoolButtonOn == true || PROGRAM_SETTINGS.BoolButtonControlOn == true) {
            
            if(printerFinish == false) {
              isPaused = !isPaused;
            } else {
                tft.fillScreen(SCREEN_LOGIC.ColorBackground);
                ESP.restart();
            }
          
            digitalWrite(PINES.PinTimerLed, LOW); // Asegura que el LED del timer esté apagado si se pausa
            
            if (isPaused) {
                Serial.println("Taxímetro pausado.");
                strprintLCD("Tiempo Pausado", 157, 40, 1, SCREEN_LOGIC.ColorWords);
            } else {
                Serial.println("Taxímetro reanudado.");
                strprintLCD("              ", 157, 40, 1, SCREEN_LOGIC.ColorWords);
                // Reinicia el tiempo de inicio para no contar el tiempo pausado
                PROGRAM_SETTINGS.StartTime = millis();
            }
        } else if (PROGRAM_SETTINGS.BoolButtonOn == false || PROGRAM_SETTINGS.BoolButtonControlOn == false ) {
            // Iniciar el taxímetro si no está en pausa ni control activo
            PROGRAM_SETTINGS.StartTime = millis();
            tft.fillScreen(SCREEN_LOGIC.ColorBackground);
            TAXIMETER_PRICES.PriceActual = TAXIMETER_PRICES.PriceBajadaBandera;
            printLCDNumber(TAXIMETER_PRICES.PriceActual);
            strprintLCD(PROGRAM_SETTINGS.Patente + "   #" + String(SerialNumber), 160, 15, 1, SCREEN_LOGIC.ColorWords);
            PROGRAM_SETTINGS.BoolButtonOn = true;
            isPaused = false;
            ManageClockCurrentTime();
            CLOCK_LOGIC.hourInitial = CLOCK_LOGIC.timeCurrent;
        }
    }
}

/**
 * @brief Maneja la logica cuando se presiona el boton de control.
 * 
 * Similar a handleButtonOnPress, pero para el boton de control. Este boton puede tener 
 * funciones especificas como cambiar modos de operacion o iniciar procedimientos de control.
 */
void handleButtonControlPress() {
    if (PROGRAM_SETTINGS.isPrinterPaperErrorActive) {
        return;
    }
    if (digitalRead(PINES.PinButtonControl) == LOW && PROGRAM_SETTINGS.BoolButtonControlOn == false && PROGRAM_SETTINGS.BoolButtonOn == false) {
        PROGRAM_SETTINGS.StartTime = millis();
        tft.fillScreen(SCREEN_LOGIC.ColorBackground);
        TAXIMETER_PRICES.PriceActual = TAXIMETER_PRICES.PriceBajadaBandera;
        printLCDNumber(TAXIMETER_PRICES.PriceActual);
        strprintLCD(PROGRAM_SETTINGS.Patente + "   #" + String(SerialNumber), 160, 15, 1, SCREEN_LOGIC.ColorWords);
        PROGRAM_SETTINGS.BoolButtonControlOn = true;
        ManageClockCurrentTime();
        CLOCK_LOGIC.hourInitial = CLOCK_LOGIC.timeCurrent;
    }
}

/**
 * @brief Maneja la logica cuando se presiona el boton de la impresora.
 * 
 * Esta funcion gestiona las acciones relacionadas con la impresora, como imprimir un boleto. 
 * Utiliza variables para rastrear el tiempo de presion del boton y el numero de veces que se
 * ha presionado, lo que puede activar diferentes funciones segun la duracion y la frecuencia 
 * de las presiones.
 */
void handleButtonPrinterPress() {
    static unsigned long lastPressTime = 0;
    static unsigned long lastReleaseTime = 0;
    static int pressCount = 0;
    unsigned long currentTime = millis();

    if (digitalRead(PINES.PinButtonPrinter) == LOW) {
        digitalWrite(PINES.PinScreenLed, LOW);
        handlePrinterButtonPressed(lastPressTime, currentTime);
    } else {
        handlePrinterButtonReleased(lastPressTime, lastReleaseTime, pressCount, currentTime);
    }

    checkAndResetPressCount(pressCount, lastReleaseTime, currentTime);
    checkRapidPresses(pressCount);
}

/**
 * @brief Acciones específicas cuando el botón de la impresora es presionado.
 * 
 * Establece lógica para determinar si se debe imprimir un boleto basándose en el estado 
 * actual del programa y en si otros botones están activos o no. Gestiona la impresión y
 * acciones subsiguientes como mostrar la fecha y hora, y actualizar la interfaz de usuario.
 *
 * @param lastPressTime Referencia a la marca de tiempo del último momento en que se presionó el botón.
 *                      Esta marca se actualiza al tiempo actual si es la primera vez que se presiona.
 * @param currentTime Tiempo actual para comparar con marcas de tiempo anteriores y gestionar
 *                    la lógica de tiempo de presión del botón.
 */
void handlePrinterButtonPressed(unsigned long &lastPressTime, unsigned long currentTime) {
    static int printCount = 0; // Contador para la cantidad de veces que se ha impreso el boleto

    if (PROGRAM_SETTINGS.BoolButtonOn || PROGRAM_SETTINGS.BoolButtonControlOn) {
        printerBoleto();
        printCount++;

        if (printCount >= 1) {
            printerFinish = true;
            delay(1000);
            ShowDateTimeClock();
            strprintLCD("Carrera Finalizada", 170, 15, 1, SCREEN_LOGIC.ColorWords);
            tickerTimerLED.detach();
            digitalWrite(PINES.PinTimerLed, LOW);
            digitalWrite(PINES.PinScreenLed, LOW);
        }
    } else {
        if (PRINTER_LOGIC.printButtonPressTime == 0) {
            PRINTER_LOGIC.printButtonPressTime = currentTime;
        }

        if (lastPressTime == 0) {
            lastPressTime = currentTime;
        }
    }
}

/**
 * @brief Acciones específicas cuando el botón de la impresora es liberado.
 * 
 * Gestiona el comportamiento del botón de la impresora cuando se libera, incluyendo la
 * posibilidad de entrar en el modo de cambio de tiempo si el botón se mantiene presionado 
 * durante un tiempo prolongado.
 *
 * @param lastPressTime Referencia a la marca de tiempo del último momento en que se presionó el botón.
 * @param lastReleaseTime Referencia a la marca de tiempo del último momento en que se liberó el botón.
 * @param pressCount Referencia al contador de presiones rápidas del botón.
 * @param currentTime Tiempo actual para comparar con marcas de tiempo anteriores.
 */
void handlePrinterButtonReleased(unsigned long &lastPressTime, unsigned long &lastReleaseTime, int &pressCount, unsigned long currentTime) {
    if (PRINTER_LOGIC.printButtonPressTime > 0 && currentTime - PRINTER_LOGIC.printButtonPressTime > 3000) {
        PRINTER_LOGIC.isPrintButtonLongPressed = true;
        enterTimeSettingMode();
        PRINTER_LOGIC.printButtonPressTime = 0;
        pressCount = 0;
        lastPressTime = 0;
        lastReleaseTime = 0;
    } else if (lastPressTime > 0) {
        if (currentTime - lastPressTime < PRINTER_LOGIC.rapidPressThreshold) {
            pressCount++;
        }
        lastPressTime = 0;
        lastReleaseTime = currentTime;
    }
    PRINTER_LOGIC.printButtonPressTime = 0;
}

/**
 * @brief Verifica y reinicia el conteo de presiones rápidas del botón.
 * 
 * Esta función determina si se debe reiniciar el contador de presiones rápidas del botón 
 * de la impresora, basándose en el tiempo transcurrido desde la última presión.
 * 
 * @param pressCount Referencia a un entero que lleva el conteo de las presiones del botón. 
 *                   Esta variable se reinicia a cero si el tiempo desde la última presión
 *                   supera el umbral definido.
 * @param lastReleaseTime Referencia a un entero sin signo de largo que registra el tiempo 
 *                        en milisegundos de la última vez que se soltó el botón. Este valor 
 *                        se reinicia a cero si se supera el umbral de tiempo.
 * @param currentTime Tiempo actual en milisegundos. Utilizado para comparar con lastReleaseTime
 *                    y determinar si ha transcurrido suficiente tiempo para reiniciar pressCount.
 */
void checkAndResetPressCount(int &pressCount, unsigned long &lastReleaseTime, unsigned long currentTime) {
    if (currentTime - lastReleaseTime > PRINTER_LOGIC.resetPressCountThreshold && lastReleaseTime > 0) {
        pressCount = 0;
        lastReleaseTime = 0;
    }
}

/**
 * @brief Verifica si se alcanzo el numero requerido de presiones rapidas.
 * 
 * Comprueba si el numero de presiones rapidas del boton de la impresora ha alcanzado el umbral 
 * requerido para una funcion especifica, como imprimir un tipo especial de boleto.
 * @param pressCount Cantidad de veces que presiono el boton.
 */
void checkRapidPresses(int &pressCount) {
    if (pressCount >= PRINTER_LOGIC.requiredPressCount) {
        pressCount = 0;
        PROGRAM_SETTINGS.BoolPrintVariables = true;
        printerBoleto();
        tft.fillScreen(SCREEN_LOGIC.ColorBackground);
        ESP.restart();
    }
}

/**
 * @brief Comprueba si un boton especifico esta presionado.
 * 
 * Retorna `true` si el boton especificado esta presionado, de lo contrario retorna `false`. 
 * Utilizado para simplificar la logica de deteccion de estado de los botones.
 */
bool isButtonPressed(int buttonPin) {
    return digitalRead(buttonPin) == LOW;
}
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////





////////////////////////////////////////////////////////////
///////////////// Visualizacion del Display ////////////////
////////////////////////////////////////////////////////////
/**
 * @brief Actualiza la pantalla para mostrar la interfaz de edicion de fecha y hora.
 *
 * Prepara y muestra la interfaz de usuario para la edicion de fecha y hora en la pantalla.
 * Resalta la seccion que esta siendo editada y muestra los valores actuales de fecha y hora.
 */
void updateDisplay() {
  char yearBuffer[5], monthBuffer[4], dayBuffer[4], hourBuffer[4], minuteBuffer[4];
  sprintf(yearBuffer, "-%04d", EDIT_CLOCK_LOGIC.currentYear);
  sprintf(monthBuffer, "-%02d", EDIT_CLOCK_LOGIC.currentMonth);
  sprintf(dayBuffer, "%02d", EDIT_CLOCK_LOGIC.currentDay);
  sprintf(hourBuffer, "%02d", EDIT_CLOCK_LOGIC.currentHour);
  sprintf(minuteBuffer, ":%02d", EDIT_CLOCK_LOGIC.currentMinute);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  strprintLCD("Editando Fecha y Hora", 155, 10, 1, SCREEN_LOGIC.ColorWords);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.setFreeFont(FF24);
  tft.setTextColor(TFT_WHITE);
  int x = 90, yDate = 100, yTime = 150, space = 60;
  tft.setTextColor(EDIT_CLOCK_LOGIC.editingSection == 0 ? TFT_YELLOW : TFT_WHITE);
  tft.drawString(dayBuffer, x-25, yDate, GFXFF);
  tft.setTextColor(EDIT_CLOCK_LOGIC.editingSection == 1 ? TFT_YELLOW : TFT_WHITE);
  tft.drawString(monthBuffer, x + space-25, yDate, GFXFF);
  tft.setTextColor(EDIT_CLOCK_LOGIC.editingSection == 2 ? TFT_YELLOW : TFT_WHITE);
  tft.drawString(yearBuffer, x + space+67, yDate, GFXFF);
  tft.setTextColor(EDIT_CLOCK_LOGIC.editingSection == 3 ? TFT_YELLOW : TFT_WHITE);
  tft.drawString(hourBuffer, 120, yTime, GFXFF);
  tft.setTextColor(EDIT_CLOCK_LOGIC.editingSection == 4 ? TFT_YELLOW : TFT_WHITE);
  tft.drawString(minuteBuffer, 120 + space, yTime, GFXFF);
}

/**
 * @brief Muestra la fecha y hora actual en la pantalla.
 *
 * Esta funcion verifica si la hora actual ha cambiado desde la ultima actualizacion.
 * Si es asi, actualiza la pantalla con la nueva hora. Utiliza un formato especifico para mostrar la hora.
 */
void ShowDateTimeClock() {
    static unsigned long lastUpdate = 0; // Almacena la última vez que se actualizó la pantalla
    const unsigned long interval = 59000; // Intervalo de tiempo deseado (30 segundos)

    ManageClockCurrentTime();

    if (CLOCK_LOGIC.timeCurrent == CLOCK_LOGIC.timePrevious) {
        CLOCK_LOGIC.timePrevious = CLOCK_LOGIC.timeCurrent;
    } else {
        unsigned long currentTime = millis();
        if (currentTime - lastUpdate >= interval) {
            digitalWrite(PINES.PinScreenLed, HIGH);
            lastUpdate = currentTime;
        }
            tft.setRotation(1);
            tft.fillRect(0, 40, 360, 360, SCREEN_LOGIC.ColorBackground);
            tft.setTextDatum(MC_DATUM);
            tft.setTextSize(2);
            tft.setFreeFont(FF24);
            tft.setTextColor(SCREEN_LOGIC.ColorWords, SCREEN_LOGIC.ColorBackground);
            tft.drawString(CLOCK_LOGIC.timeCurrent, 160, 120, GFXFF);
            
            CLOCK_LOGIC.timePrevious = CLOCK_LOGIC.timeCurrent;

    }
}

/**
 * @brief Imprime un mensaje de texto en la pantalla en una posicion y tamano especificos.
 *
 * Dibuja una cadena de texto en la pantalla LCD en la ubicacion especificada, con un tamano y color determinados.
 * Utiliza la configuracion actual de fuente y color de fondo.
 *
 * @param message El mensaje de texto a mostrar.
 * @param x Coordenada X para el inicio del texto.
 * @param y Coordenada Y para el inicio del texto.
 * @param size Tamano del texto.
 * @param ColorText Color del texto.
 */
void strprintLCD(const String &message, int x, int y, int size, uint16_t ColorText) {
    tft.setRotation(1);
    tft.fillRect(x-180, y-10, 360, 20, SCREEN_LOGIC.ColorBackground);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(FM9);
    tft.setTextColor(ColorText, SCREEN_LOGIC.ColorBackground);
    tft.setTextSize(size);
    tft.drawString(String(message), x, y, GFXFF);
}

/**
 * @brief Muestra un numero en la pantalla en una posicion especifica.
 *
 * Esta funcion esta disenada para mostrar el precio o numeros similares en la pantalla.
 * Utiliza un formato especifico y posicion para garantizar la consistencia visual.
 *
 * @param number El numero a mostrar.
 */
void printLCDNumber(int number) {
    tft.setRotation(1);
    tft.fillRect(158-180, 120-10, 240, 40, SCREEN_LOGIC.ColorBackground);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(FF20);
    tft.setTextColor(SCREEN_LOGIC.ColorNumbersPrice, SCREEN_LOGIC.ColorBackground);
    tft.setTextSize(2);
    tft.drawString(String(number), 158, 120, GFXFF);
}
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////





////////////////////////////////////////////////////////////
/////////////// Gestion del tiempo y alarmas ///////////////
////////////////////////////////////////////////////////////
/**
 * @brief Gestiona la logica principal del programa en funcion del estado de los botones y modos.
 *
 * Esta funcion determina si el sistema esta en modo de cambio de tiempo o en funcionamiento normal.
 * En el modo normal, gestiona los pulsos, el temporizador y muestra la hora actual. En el modo de cambio
 * de tiempo, maneja las interacciones para ajustar la fecha y la hora.
 */
void manageProgramLogic() {
    if ((PROGRAM_SETTINGS.BoolButtonOn == true || PROGRAM_SETTINGS.BoolButtonControlOn == true) && printerFinish == false) {
        managePulsesAndTimer();
    } else {
        ShowDateTimeClock();
        handleNightMode();
    }
    updateBlinkingPulseLed();
    ManagePrinterPaperDetection();
    checkUnlockCondition();
}


/**
 * @brief Maneja la logica para cambiar la fecha y la hora del sistema.
 *
 * Esta funcion se activa cuando el sistema esta en el modo de cambio de tiempo. Permite al usuario
 * ajustar la fecha y la hora mediante los botones. Si se mantiene presionado un boton durante un tiempo
 * suficiente, la nueva fecha y hora se guardan y se reinicia el sistema.
 */
void handleTimeChangeMode() {
    if (digitalRead(PINES.PinButtonPrinter) == LOW) {
        if (EDIT_CLOCK_LOGIC.bothButtonsPressedTime == 0) {
            EDIT_CLOCK_LOGIC.bothButtonsPressedTime = millis();
        }
        else if (millis() - EDIT_CLOCK_LOGIC.bothButtonsPressedTime > 3000) {
            DateTime newTime = DateTime(EDIT_CLOCK_LOGIC.currentYear, EDIT_CLOCK_LOGIC.currentMonth, EDIT_CLOCK_LOGIC.currentDay, EDIT_CLOCK_LOGIC.currentHour, EDIT_CLOCK_LOGIC.currentMinute, 0);
            rtc.adjust(newTime);
            EDIT_CLOCK_LOGIC.ischangeTime = false;
            ESP.restart();
        }
    } else {
        EDIT_CLOCK_LOGIC.bothButtonsPressedTime = 0;
    }
    if (digitalRead(PINES.PinButtonControl) == LOW) {
        EDIT_CLOCK_LOGIC.editingSection = (EDIT_CLOCK_LOGIC.editingSection + 1) % 5;
        updateDisplay();
        delay(300);
    }
    if (digitalRead(PINES.PinButtonOn) == LOW) {
        adjustCurrentTimeSection();
        updateDisplay();
        delay(100);
    }
}

/**
 * @brief Ajusta la seccion actual de la fecha y hora que se esta editando.
 *
 * Cambia el valor de la seccion de fecha y hora seleccionada (dia, mes, ano, hora o minuto)
 * en el modo de edicion de fecha y hora.
 */
void adjustCurrentTimeSection() {
    switch (EDIT_CLOCK_LOGIC.editingSection) {
        case 0: EDIT_CLOCK_LOGIC.currentDay = (EDIT_CLOCK_LOGIC.currentDay % 31) + 1; break;
        case 1: EDIT_CLOCK_LOGIC.currentMonth = (EDIT_CLOCK_LOGIC.currentMonth % 12) + 1; break;
        case 2: EDIT_CLOCK_LOGIC.currentYear = (EDIT_CLOCK_LOGIC.currentYear - 2024 + 1) % 11 + 2024; break;
        case 3: EDIT_CLOCK_LOGIC.currentHour = (EDIT_CLOCK_LOGIC.currentHour + 1) % 24; break;
        case 4: EDIT_CLOCK_LOGIC.currentMinute = (EDIT_CLOCK_LOGIC.currentMinute + 1) % 60; break;
    }
}

/**
 * @brief Verifica si el sistema esta en el modo de cambio de tiempo.
 *
 * @return Verdadero si el sistema esta en modo de cambio de tiempo, falso en caso contrario.
 */
bool isInTimeChangeMode() {
    return EDIT_CLOCK_LOGIC.ischangeTime;
}

/**
 * @brief Actualiza la hora actual en la logica del reloj del programa.
 *
 * Obtiene la hora actual del RTC y actualiza las variables correspondientes en la logica del reloj.
 */
void ManageClockCurrentTime() {
    DateTime now = rtc.now();
    CLOCK_LOGIC.timeCurrent = String(now.hour()) + ":" + (now.minute() < 10 ? "0" : "") + String(now.minute());
    EDIT_CLOCK_LOGIC.currentYear = now.year();
    EDIT_CLOCK_LOGIC.currentMonth = now.month();
    EDIT_CLOCK_LOGIC.currentDay = now.day();
    EDIT_CLOCK_LOGIC.currentHour = now.hour();
    EDIT_CLOCK_LOGIC.currentMinute = now.minute();
}

/**
 * @brief Actualiza la fecha actual en la logica del reloj del programa.
 *
 * Obtiene la fecha actual del RTC y actualiza las variables correspondientes en la logica del reloj.
 */
void ManageClockCurrentDate() {
    DateTime now = rtc.now();
    CLOCK_LOGIC.dateFormat = + (now.day() < 10 ? "0" : "") + String(now.day()) + "/" + (now.month() < 10 ? "0" : "") + String(now.month()) + "/" + String(now.year());
}

/**
 * @brief Maneja el modo nocturno, activando o desactivando el LED segun la hora actual.
 *
 * Activa el LED del modo nocturno entre las 21:00 y las 6:00 horas, y lo desactiva fuera de este horario.
 */
void handleNightMode() {
    if(CLOCK_LOGIC.timeCurrent.substring(0,2).toInt() >= 21 || CLOCK_LOGIC.timeCurrent.substring(0,2).toInt() <= 6) {
        digitalWrite(PINES.PinNightMode, HIGH);
    } else {
        digitalWrite(PINES.PinNightMode, LOW);
    }
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
/////////////// Manejo de Pulsos y Velocidad ///////////////
////////////////////////////////////////////////////////////
/**
 * @brief Gestiona los pulsos y el temporizador para calcular la velocidad y otros parametros.
 *
 * Esta funcion se encarga de controlar los pulsos recibidos del sensor y actualizar los parametros
 * del vehiculo, como la velocidad y la distancia recorrida. Tambien gestiona la logica especial
 * para situaciones como la inactividad prolongada.
 */

void managePulsesAndTimer() {
  if(FIRST_LOGIC.isFirstLogic == true) {
      managePulse();
      manageTimerLed();
    if(TIMER_LOGIC.CountSecondsStop < 60) {
      METROS_TIEMPO = TIMER_LOGIC.CountSecondsStop * 3.3333;
      if(PULSE_LOGIC.CountCompleteWheelRotation >= 1) {
        METROS_METROS = (PULSE_LOGIC.CountCompleteWheelRotation * METERS_LOGIC.MeterByCompleteWheelRotation);
      }
      if(METROS_METROS + METROS_TIEMPO >= 200) {
          FIRST_LOGIC.is60SecondsAccumulated = true;
          FIRST_LOGIC.isFirstLogic = false;
          TIMER_LOGIC.CountSecondsStop = 0;
          Serial.println("Salimos de la logica porque pasaron 200 metros movimiento + tiempo combinados.");
      }
    } else {
      FIRST_LOGIC.is60SecondsAccumulated = true;
      TIMER_LOGIC.CountSecondsStop = 0;
      FIRST_LOGIC.isFirstLogic = false;
      Serial.println("Salimos de la logica porque pasaron 60 segundos sin movimiento.");
    }
  } else {
    if (TIMER_LOGIC.CountSecondsStop % 60 == 0 && TIMER_LOGIC.CountSecondsStop != 0 && TIMER_LOGIC.BoolCountSeconds == true && FIRST_LOGIC.isFirstLogic == false) {
        TAXIMETER_PRICES.PriceActual += TAXIMETER_PRICES.PriceCaidaParcialSegundos;
        printLCDNumber(TAXIMETER_PRICES.PriceActual);
        TIMER_LOGIC.BoolCountSeconds = false;
    }
    managePulse();
    manageTimerLed();
  }
}

/**
 * @brief Controla el LED del temporizador y maneja las condiciones de alarma.
 *
 * Esta funcion alterna el estado del LED del temporizador y gestiona las alarmas de aceleracion y velocidad.
 * Se activa en respuesta a ciertas condiciones, como la deteccion de una rueda completa o la activacion
 * de una alarma.
 */
void manageTimerLed() {
  if((PROGRAM_SETTINGS.isAcelerationAlarmActive == true || PROGRAM_SETTINGS.isSpeedAlarmActive == true) && !TIMER_LOGIC.BoolTickerTimerLed && (micros() - PULSE_LOGIC.StartTimeDetectCompleteWheelRotation > 1000000)) {
    digitalWrite(PINES.PinPulseLed, LOW);
    digitalWrite(PINES.PinTimerLed, LOW);
    toggleBuzzerToneErrorAceleration(false);
    toggleBuzzerToneErrorSpeed(false);
    calculateStatisticsVehicule(true);
  }
  if (!TIMER_LOGIC.BoolTickerTimerLed && (micros() - PULSE_LOGIC.StartTimeDetectCompleteWheelRotation > 1000000) && PROGRAM_SETTINGS.isAcelerationAlarmActive == false && PROGRAM_SETTINGS.isSpeedAlarmActive == false) {
    digitalWrite(PINES.PinPulseLed, LOW);
    PULSE_LOGIC.DetectCompleteWheelRotation = false;
    tickerTimerLED.attach(0.5, toggleTimerLED);
    SPEED_LOGIC.SpeedActualMxS = 0;
    TIMER_LOGIC.BoolTickerTimerLed = true;
  }
  if(PULSE_LOGIC.DetectCompleteWheelRotation == true) {
    if(PROGRAM_SETTINGS.isSpeedAlarmActive == false && PROGRAM_SETTINGS.isAcelerationAlarmActive == false) {
      digitalWrite(PINES.PinTimerLed, LOW);
      tickerTimerLED.detach();
      TIMER_LOGIC.BoolTickerTimerLed = false;
    }
  }
}

/**
 * @brief Gestiona la logica de deteccion de pulsos del sensor.
 *
 * Detecta los pulsos del sensor y calcula las estadisticas del vehiculo, como la velocidad y la aceleracion,
 * en base a estos pulsos. Tambien gestiona las acciones a realizar cuando se detecta un pulso, como
 * el parpadeo de un LED.
 */
void managePulse() {
  static bool firstPulseDetected = false; // Variable para rastrear el primer pulso

  if(digitalRead(PINES.PinTimerLed) == HIGH) { // Reiniciar si el LED del temporizador está alto
    firstPulseDetected = false;
    return;
  } else {
    PULSE_LOGIC.StateCurrentPulse = digitalRead(PINES.PinPulse);
    if (PULSE_LOGIC.StateCurrentPulse == HIGH && PULSE_LOGIC.StatePreviousPulse == LOW) {
      PULSE_LOGIC.StatePreviousPulse = PULSE_LOGIC.StateCurrentPulse;
      return;
    }
    if (PULSE_LOGIC.StateCurrentPulse == LOW && PULSE_LOGIC.StatePreviousPulse == HIGH) {
      PULSE_LOGIC.StatePreviousPulse = PULSE_LOGIC.StateCurrentPulse;
      
      if (!firstPulseDetected) {
        firstPulseDetected = true; // Detectar el primer pulso
        return;
      }
      
      PULSE_LOGIC.CountPulses++;
      PROGRAM_SETTINGS.PulseCountSinceStart++;
      if (PROGRAM_SETTINGS.PulseCountSinceStart == 1) {
        ACELERATION_LOGIC.highAccelStartTime = millis();
      }
      if (PROGRAM_SETTINGS.PulseCountSinceStart >= 1) {
        unsigned long timeSinceFirstPulse = millis() - ACELERATION_LOGIC.highAccelStartTime;
        if (SPEED_LOGIC.SpeedActualMxS * 3.6 > 15 && timeSinceFirstPulse <= 1000) {
          toggleBuzzerToneErrorAceleration(true);
          PROGRAM_SETTINGS.PulseCountSinceStart = 0;
        }
      }
      if(PULSE_LOGIC.DividePulse == PULSE_LOGIC.CountPulses) {
        PULSE_LOGIC.StartTimeDetectCompleteWheelRotation = micros();
        PULSE_LOGIC.DetectCompleteWheelRotation = true;
        if(PROGRAM_SETTINGS.isSpeedAlarmActive == false && PROGRAM_SETTINGS.isAcelerationAlarmActive == false) {
          startBlinkingPulseLed();
        }
        PULSE_LOGIC.CountPulses = 0;
        calculateStatisticsVehicule(false);
        if(PROGRAM_SETTINGS.isSpeedAlarmActive == false && PROGRAM_SETTINGS.isAcelerationAlarmActive == false) {
          PULSE_LOGIC.CountCompleteWheelRotation++;
          isCaidaPartialMeters();
        }
        ManageSpeedAlert();
      }
    } else {
      PULSE_LOGIC.StatePreviousPulse = PULSE_LOGIC.StateCurrentPulse;
      PULSE_LOGIC.PreviousTimeDetectCompleteWheelRotation = PULSE_LOGIC.StartTimeDetectCompleteWheelRotation;
      SPEED_LOGIC.SpeedPreviousMxS = SPEED_LOGIC.SpeedActualMxS;
      ManageAcelerationAlert();
    }
  }
}




// Define la longitud de la ventana móvil
const int WINDOW_SIZE = 10;
std::deque<float> accelerationWindow;

// Umbral para detectar anomalías
const float THRESHOLD = 5.0;
const unsigned long ANOMALY_DURATION_THRESHOLD = 2000; // 2 segundos en milisegundos

// Variables para gestionar el tiempo de las anomalías
bool isAnomaly = false;
unsigned long anomalyStartTime = 0;

// Función para calcular la media de la ventana móvil
float calculateWindowAverage(const std::deque<float>& window) {
  float sum = 0.0;
  for (float value : window) {
    sum += value;
  }
  return sum / window.size();
}

/**
 * @brief Calcula y actualiza las estadisticas del vehiculo.
 *
 * Realiza calculos para determinar la velocidad actual y la aceleracion del vehiculo.
 * Puede restablecer estos valores a cero si se especifica.
 * 
 * @param zero Si es verdadero, restablece las estadisticas a cero.
 */
void calculateStatisticsVehicule(bool zero) {
  if (PROGRAM_SETTINGS.isCalculating) {
    return;
  }
  PROGRAM_SETTINGS.isCalculating = true;

  static float lastValidAceleration = 0.0;

  if(zero == true) {
    SPEED_LOGIC.SpeedActualMxS = 0;
    ACELERATION_LOGIC.AcelerationActualMxS2 = 0;
    lastValidAceleration = 0;
    accelerationWindow.clear();
    isAnomaly = false;
    anomalyStartTime = 0;
    char buffer[50];
    sprintf(buffer, "%6.2f km/hr   %5.2f mt/s2", 0, 0);
    if (MOSTRAR_VELOCIDAD_EN_PANTALLA == true) {
      strprintLCD(buffer, 150, 220, 1, SCREEN_LOGIC.ColorWords);
    }
  } else {
    if(PROGRAM_SETTINGS.isAcelerationAlarmActive == false && PROGRAM_SETTINGS.isSpeedAlarmActive == false) {
      METERS_LOGIC.MetersTraveled += METERS_LOGIC.MeterByCompleteWheelRotation;
      METERS_LOGIC.MetersTraveled_RAW += METERS_LOGIC.MeterByCompleteWheelRotation;
      METERS_LOGIC.MetersTraveled200Mts += METERS_LOGIC.MeterByCompleteWheelRotation;
    }

    PULSE_LOGIC.DifferenceTimeDetectCompleteWheelRotation = PULSE_LOGIC.StartTimeDetectCompleteWheelRotation - PULSE_LOGIC.PreviousTimeDetectCompleteWheelRotation;
    SPEED_LOGIC.SpeedActualMxS = (METERS_LOGIC.MeterByCompleteWheelRotation / (PULSE_LOGIC.DifferenceTimeDetectCompleteWheelRotation / 1000000.0));

    // Calcular la aceleración actual
    float currentAceleration = (SPEED_LOGIC.SpeedActualMxS - SPEED_LOGIC.SpeedPreviousMxS) / (PULSE_LOGIC.DifferenceTimeDetectCompleteWheelRotation / 1000000.0);

    // Añadir la aceleración actual a la ventana
    accelerationWindow.push_back(currentAceleration);
    if (accelerationWindow.size() > WINDOW_SIZE) {
      accelerationWindow.pop_front();
    }

    // Calcular la media de la ventana
    float windowAverage = calculateWindowAverage(accelerationWindow);

    // Detectar anomalías si la aceleración es significativamente mayor que la media
    if (abs(currentAceleration - windowAverage) > THRESHOLD * abs(windowAverage)) {
      if (!isAnomaly) {
        // Primera detección de anomalía
        isAnomaly = true;
        anomalyStartTime = millis();
      } else if (millis() - anomalyStartTime >= ANOMALY_DURATION_THRESHOLD) {
        // Anomalía persistente durante el umbral de tiempo
        ACELERATION_LOGIC.AcelerationActualMxS2 = currentAceleration;
        lastValidAceleration = currentAceleration;
      }
    } else {
      // No es una anomalía, actualizar la aceleración válida
      isAnomaly = false;
      ACELERATION_LOGIC.AcelerationActualMxS2 = currentAceleration;
      lastValidAceleration = currentAceleration;
    }

    char buffer[50];
    sprintf(buffer, "%6.2f km/hr   %5.2f mt/s2", SPEED_LOGIC.SpeedActualMxS * 3.6, ACELERATION_LOGIC.AcelerationActualMxS2);
    if (MOSTRAR_VELOCIDAD_EN_PANTALLA == true) {
      strprintLCD(buffer, 150, 220, 1, SCREEN_LOGIC.ColorWords);
    }
  }
  PROGRAM_SETTINGS.isCalculating = false;
}


/**
 * @brief Gestiona la logica para la caida parcial de metros.
 *
 * Actualiza el precio total y otros parametros cuando se recorre una distancia especifica.
 * Se utiliza para calcular tarifas basadas en la distancia recorrida.
 */
void isCaidaPartialMeters() {
  if(PULSE_LOGIC.CountCompleteWheelRotation > 0 && METERS_LOGIC.MetersTraveled200Mts - 200 >= 0) {
    TAXIMETER_PRICES.PriceActual = TAXIMETER_PRICES.PriceActual + TAXIMETER_PRICES.PriceCaidaParcialMetros;
    printLCDNumber(TAXIMETER_PRICES.PriceActual);
    METERS_LOGIC.MetersTraveled200Mts = 0;
    METERS_LOGIC.MetersTraveledInt = METERS_LOGIC.MetersTraveledInt + 200;
    TAXIMETER_PRICES.TotalMetersMoney = TAXIMETER_PRICES.TotalMetersMoney + TAXIMETER_PRICES.PriceCaidaParcialMetros;
  }
  if(PULSE_LOGIC.CountCompleteWheelRotation > 0 && FIRST_LOGIC.is60SecondsAccumulated == true) {
    TAXIMETER_PRICES.PriceActual = TAXIMETER_PRICES.PriceActual + TAXIMETER_PRICES.PriceCaidaParcialMetros;
    printLCDNumber(TAXIMETER_PRICES.PriceActual);
    METERS_LOGIC.MetersTraveled200Mts = 0;
    METERS_LOGIC.MetersTraveledInt = METERS_LOGIC.MetersTraveledInt + 200;
    TAXIMETER_PRICES.TotalMetersMoney = TAXIMETER_PRICES.TotalMetersMoney + TAXIMETER_PRICES.PriceCaidaParcialMetros;
    FIRST_LOGIC.is60SecondsAccumulated = false;
    //TIMER_LOGIC.CountSecondsStop = 0;
  }
}

/**
 * @brief Controla las alertas de velocidad del vehiculo.
 *
 * Activa o desactiva las alarmas de velocidad en funcion de los limites establecidos.
 * Si la velocidad es menor a 3km/hr entonces detectamos que el vehiculo se encuentra detenido. 
 * Si la velocidad supera los 120km/hr entonces detectamos que el vehiculo se encuentra en un estado de velocidad maxima, por lo que emite alarma.
 */
void ManageSpeedAlert() {
  if(SPEED_LOGIC.SpeedActualMxS * 3.6 > 120) {
    toggleBuzzerToneErrorSpeed(true);
  }
  if (SPEED_LOGIC.SpeedActualMxS * 3.6 <= 3) {
    toggleBuzzerToneErrorSpeed(false);
  }
}

/**
 * @brief Gestiona las alertas de aceleracion del vehiculo.
 *
 * Controla las alertas de aceleracion en funcion de los limites predefinidos y la aceleracion actual.
 */
void ManageAcelerationAlert() {
  bool isHighAccel = false;
  for (int i = 0; i < sizeof(limites) / sizeof(limites[0]); i++) {
    if (SPEED_LOGIC.SpeedActualMxS * 3.6 >= limites[i].velocidadMin && 
        SPEED_LOGIC.SpeedActualMxS * 3.6 <= limites[i].velocidadMax && 
        ACELERATION_LOGIC.AcelerationActualMxS2 >= limites[i].aceleracionLimite) {
      isHighAccel = true;
      if (ACELERATION_LOGIC.highAccelStartTime == 0) {
        ACELERATION_LOGIC.highAccelStartTime = millis();
      }
      break;
    }
  }
  if (isHighAccel && millis() - ACELERATION_LOGIC.highAccelStartTime >= 500) {
    toggleBuzzerToneErrorAceleration(true);
  } else if (!isHighAccel) {
    ACELERATION_LOGIC.highAccelStartTime = 0;
  }
}
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////





////////////////////////////////////////////////////////////
//////////// Gestion de LEDs y Alarmas Sonoras /////////////
////////////////////////////////////////////////////////////
/**
 * @brief Alterna el estado del LED del temporizador.
 *
 * Cambia el estado del LED del temporizador en funcion de las alarmas activas. Si no hay alarmas,
 * el LED se alterna para indicar el estado del temporizador.
 */
void toggleTimerLED() {
  if((PROGRAM_SETTINGS.isAcelerationAlarmActive == false || PROGRAM_SETTINGS.isSpeedAlarmActive == false) && isPaused == false) {
    int state = digitalRead(PINES.PinTimerLed);
    digitalWrite(PINES.PinTimerLed, !state);
    if (digitalRead(PINES.PinTimerLed) == HIGH) {
      PROGRAM_SETTINGS.PulseCountSinceStart = 0;
      digitalWrite(PINES.PinPulseLed, LOW);
      TIMER_LOGIC.CountSecondsStop++;
      TIMER_LOGIC.CountSecondsStop_RAW++;
      TIMER_LOGIC.BoolCountSeconds = true;
    }
  }
}

/**
 * @brief Inicia el parpadeo del LED de pulso.
 *
 * Activa el LED de pulso y comienza a registrar el tiempo para controlar la duracion del parpadeo.
 */
void startBlinkingPulseLed() {
  digitalWrite(PINES.PinTimerLed, LOW);
  digitalWrite(PINES.PinPulseLed, HIGH);
  PULSE_LOGIC.BlinkStartTime = micros();
  PULSE_LOGIC.BoolIsBlinkingLed = true;
}

/**
 * @brief Actualiza el estado del parpadeo del LED de pulso.
 *
 * Apaga el LED de pulso despues de un tiempo especifico para crear un efecto de parpadeo.
 */
void updateBlinkingPulseLed() {
  if (PULSE_LOGIC.BoolIsBlinkingLed && (micros() - PULSE_LOGIC.BlinkStartTime >= 1000)) {
    digitalWrite(PINES.PinPulseLed, LOW);
    PULSE_LOGIC.BoolIsBlinkingLed = false;
  }
}

/**
 * @brief Controla la alarma sonora y visual para el error de papel en la impresora.
 *
 * Activa o desactiva la alarma sonora y visual cuando se detecta un error de papel en la impresora.
 * 
 * @param Tone Indica si se debe activar (true) o desactivar (false) la alarma.
 */
void toggleBuzzerToneErrorPrinterPaper(bool Tone) {
    if (Tone) {
        tickerBlink.attach(0.3, blinkLeds);
        tickerAlarm.attach(0.5, soundAlarm);
        strprintLCD("ERROR: PAPEL", 150, 15, 1, SCREEN_LOGIC.ColorWords);
        PROGRAM_SETTINGS.isPrinterPaperErrorActive = true;
    } else {
        tickerBlink.detach();
        tickerAlarm.detach();
        ledcWriteTone(ledcChannel, 0);
        digitalWrite(PINES.PinPulseLed, LOW);
        digitalWrite(PINES.PinTimerLed, LOW);
        strprintLCD("ERROR: PAPEL", 150, 15, 1, SCREEN_LOGIC.ColorBackground);
        PROGRAM_SETTINGS.isPrinterPaperErrorActive = false;
        strprintLCD(PROGRAM_SETTINGS.Patente + "   #" + String(SerialNumber), 160, 15, 1, SCREEN_LOGIC.ColorWords);
    }
}

/**
 * @brief Controla la alarma sonora y visual para el error de aceleracion.
 *
 * Activa o desactiva la alarma sonora y visual cuando se detecta una aceleracion excesiva.
 * 
 * @param Tone Indica si se debe activar (true) o desactivar (false) la alarma.
 */
void toggleBuzzerToneErrorAceleration(bool Tone) {
  if (Tone && !PROGRAM_SETTINGS.isAcelerationAlarmActive) {
    tickerBlink.attach(0.3, blinkLeds);
    tickerAlarm.attach(0.5, soundAlarm);
    strprintLCD("ERROR: ACELERACION", 150, 15, 1, SCREEN_LOGIC.ColorWords);
    PROGRAM_SETTINGS.isAcelerationAlarmActive = true;
  } else if (!Tone && PROGRAM_SETTINGS.isAcelerationAlarmActive) {
    tickerBlink.detach();
    tickerAlarm.detach();
    ledcWriteTone(ledcChannel, 0);
    digitalWrite(PINES.PinPulseLed, LOW);
    digitalWrite(PINES.PinTimerLed, LOW);
    strprintLCD("ERROR: ACELERACION", 150, 15, 1, SCREEN_LOGIC.ColorBackground);
    PROGRAM_SETTINGS.isAcelerationAlarmActive = false;
    strprintLCD(PROGRAM_SETTINGS.Patente + "   #" + String(SerialNumber), 160, 15, 1, SCREEN_LOGIC.ColorWords);
  }
}

/**
 * @brief Controla la alarma sonora y visual para el error de velocidad.
 *
 * Activa o desactiva la alarma sonora y visual cuando se detecta una velocidad excesiva.
 * 
 * @param Tone Indica si se debe activar (true) o desactivar (false) la alarma.
 */
void toggleBuzzerToneErrorSpeed(bool Tone) {
    if (Tone && !PROGRAM_SETTINGS.isSpeedAlarmActive) {
        tickerBlink.attach(0.3, blinkLeds);
        tickerAlarm.attach(0.5, soundAlarm);
        strprintLCD("ERROR: VELOCIDAD", 150, 15, 1, SCREEN_LOGIC.ColorWords);
        PROGRAM_SETTINGS.isSpeedAlarmActive = true;
    } else if (!Tone && PROGRAM_SETTINGS.isSpeedAlarmActive) {
        tickerBlink.detach();
        tickerAlarm.detach();
        ledcWriteTone(ledcChannel, 0);
        digitalWrite(PINES.PinPulseLed, LOW);
        digitalWrite(PINES.PinTimerLed, LOW);
        strprintLCD("ERROR: VELOCIDAD", 150, 15, 1, SCREEN_LOGIC.ColorBackground);
        PROGRAM_SETTINGS.isSpeedAlarmActive = false;
        strprintLCD(PROGRAM_SETTINGS.Patente + "   #" + String(SerialNumber), 160, 15, 1, SCREEN_LOGIC.ColorWords);
    }
}

/**
 * @brief Genera un tono de alarma.
 *
 * Alterna entre generar y detener un tono de alarma, creando un efecto de sonido intermitente.
 */
void soundAlarm() {
  static bool alarmState = false;
  if (alarmState) {
    ledcWriteTone(ledcChannel, 0);
  } else {
    ledcWriteTone(ledcChannel, 4500);
  }
  alarmState = !alarmState;
}

/**
 * @brief Hace parpadear los LEDs de temporizador y pulso.
 *
 * Alterna el estado de los LEDs de temporizador y pulso para crear un efecto de parpadeo.
 */
void blinkLeds() {
  digitalWrite(PINES.PinTimerLed, !digitalRead(PINES.PinTimerLed));
  digitalWrite(PINES.PinPulseLed, !digitalRead(PINES.PinPulseLed));
}

/**
 * @brief Verifica y gestiona la condicion para desactivar las alarmas.
 *
 * Si la velocidad del vehiculo es baja durante un tiempo suficiente (3km/hr), desactiva las alarmas de aceleracion y velocidad.
 */
void checkUnlockCondition() {
  static unsigned long startTime = 0;
  bool isSpeedLow = SPEED_LOGIC.SpeedActualMxS * 3.6 <= 3;
  if (isSpeedLow) {
    if (startTime == 0) {
      startTime = millis();
    } else if (millis() - startTime >= 1000) {
      toggleBuzzerToneErrorAceleration(false);
      toggleBuzzerToneErrorSpeed(false);
      startTime = 0;
    }
  } else {
    startTime = 0;
  }
}
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////





////////////////////////////////////////////////////////////
////////// Funciones Especificas de la Impresora ///////////
////////////////////////////////////////////////////////////
/**
 * @brief Formatea una cadena para cumplir con una longitud deseada.
 *
 * Anade ceros al inicio de la cadena hasta alcanzar la longitud especificada.
 * 
 * @param str Cadena a formatear.
 * @param desiredLength Longitud deseada de la cadena resultante.
 * @return Cadena formateada con la longitud deseada.
 */
String formatString(String str, int desiredLength) {
    while (str.length() < desiredLength) {
        str = "0" + str;
    }
    return str;
}

/**
 * @brief Gestiona la impresion de boletos.
 *
 * Imprime un boleto con la informacion actual del taximetro, incluyendo fecha, hora, tarifas y 
 * otros datos relevantes. Se comporta de manera diferente dependiendo del estado de los botones 
 * y de la alarma de papel de la impresora.
 */
void printerBoleto() {
    if (PROGRAM_SETTINGS.isPrinterPaperErrorActive) {
        return;
    }
    ManageClockCurrentDate();
    ManageClockCurrentTime();
    CLOCK_LOGIC.hourEnd = CLOCK_LOGIC.timeCurrent;
    String patente = String(PROGRAM_SETTINGS.Patente);
    String serial_number = String(SerialNumber);
    String fecha = String(CLOCK_LOGIC.dateFormat);
    String horaInicio = String(CLOCK_LOGIC.hourInitial);
    
    if (horaInicio.length() == 0) {
        horaInicio = String(CLOCK_LOGIC.hourEnd);
    }

    String horaFin = String(CLOCK_LOGIC.hourEnd);
    String bajadaBanderaPrecio = formatString(String(TAXIMETER_PRICES.PriceBajadaBandera), 4);
    String cada200MetrosPrecio = formatString(String(TAXIMETER_PRICES.PriceCaidaParcialMetros), 4);
    String cada60SegundosPrecio = formatString(String(TAXIMETER_PRICES.PriceCaidaParcialSegundos), 4);
    String totalBajadaBandera = formatString(String(TAXIMETER_PRICES.PriceBajadaBandera), 6);
    String totalMetros = formatString(String(METERS_LOGIC.MetersTraveledInt), 6);
    String totalPrecioMetros = formatString(String(TAXIMETER_PRICES.TotalMetersMoney), 6);
    String totalMinutos = formatString(String(TIMER_LOGIC.CountSecondsStop / 60), 6);
    String totalPrecioMinutos = formatString(String((TIMER_LOGIC.CountSecondsStop / 60) * TAXIMETER_PRICES.PriceCaidaParcialSegundos), 6);
    String totalCobrado = formatString(String(TAXIMETER_PRICES.PriceActual), 6);
    
    if (totalCobrado.toFloat() == 0) {
        totalCobrado = formatString(String(TAXIMETER_PRICES.PriceBajadaBandera), 6);
    }

    String metrosControl = formatString(String(METERS_LOGIC.MetersTraveled_RAW), 6);
    String minutosControl = formatString(String(TIMER_LOGIC.CountSecondsStop_RAW / 60), 6);
    String segundosControl = formatString(String(TIMER_LOGIC.CountSecondsStop_RAW % 60), 2);
    String propaganda1 = String(PROGRAM_SETTINGS.propaganda_1);
    String propaganda2 = String(PROGRAM_SETTINGS.propaganda_2);
    String propaganda3 = String(PROGRAM_SETTINGS.propaganda_3);
    String propaganda4 = String(PROGRAM_SETTINGS.propaganda_4);

    if (PROGRAM_SETTINGS.BoolButtonOn == true) {
        imprimirBoleto(
            mySerial, 
            patente,
            serial_number,
            fecha,
            horaInicio,
            horaFin,
            bajadaBanderaPrecio,
            cada200MetrosPrecio,
            cada60SegundosPrecio,
            totalBajadaBandera,
            totalMetros,
            totalPrecioMetros,
            totalMinutos,
            totalPrecioMinutos,
            totalCobrado,
            metrosControl,
            minutosControl,
            segundosControl,
            propaganda1,
            propaganda2,
            propaganda3,
            propaganda4
        );
    }

    if (PROGRAM_SETTINGS.BoolButtonControlOn == true) {
        imprimirBoletoControl(
            mySerial, 
            patente,
            serial_number,
            fecha,
            horaInicio,
            horaFin,
            bajadaBanderaPrecio,
            cada200MetrosPrecio,
            cada60SegundosPrecio,
            totalBajadaBandera,
            totalMetros,
            totalPrecioMetros,
            totalMinutos,
            totalPrecioMinutos,
            totalCobrado,
            metrosControl,
            minutosControl,
            segundosControl
        );
    }

    if (PROGRAM_SETTINGS.BoolPrintVariables == true) {
        DateTime now = rtc.now();
        CLOCK_LOGIC.timeCurrent = String(now.hour()) + ":" + (now.minute() < 10 ? "0" : "") + String(now.minute());
        imprimirBoletoVariables(
            mySerial, 
            String(fecha),
            String(CLOCK_LOGIC.timeCurrent),
            String(MODELO_TAXIMETRO),
            String(PATENTE),
            String(CANTIDAD_PULSOS),
            String(METERS_LOGIC.MeterByCompleteWheelRotation, 3),
            String(RESOLUCION),
            String(TARIFA_INICIAL),
            String(TARIFA_CAIDA_PARCIAL_METROS),
            String(TARIFA_CAIDA_PARCIAL_MINUTO)
        );
    }
}


/**
 * @brief Verifica el estado del papel en la impresora.
 *
 * Comprueba si hay papel en la impresora y activa una alarma si se detecta que falta papel. 
 * Realiza la comprobacion a intervalos regulares.
 */
void ManagePrinterPaperDetection() {
  if (PROGRAM_SETTINGS.isAcelerationAlarmActive == false && PROGRAM_SETTINGS.isSpeedAlarmActive == false) {
    static unsigned long lastCheckTime = 0;
    static bool detectedLow = false;
    unsigned long currentTime = millis();
    if (currentTime - lastCheckTime >= 1000) {
        lastCheckTime = currentTime;
        if (detectedLow) {
            if (!PROGRAM_SETTINGS.isPrinterPaperErrorActive) {
                PROGRAM_SETTINGS.isPrinterPaperErrorActive = true;
                toggleBuzzerToneErrorPrinterPaper(true);
            }
        } else {
            if (PROGRAM_SETTINGS.isPrinterPaperErrorActive) {
                PROGRAM_SETTINGS.isPrinterPaperErrorActive = false;
                toggleBuzzerToneErrorPrinterPaper(false);
            }
        }
        detectedLow = false;
    }
    if (digitalRead(PINES.PinPrinterPaperDetection) == LOW) {
        detectedLow = true;
    }
  }
}

/**
 * @brief Activa el modo de configuracion de tiempo.
 *
 * Prepara la pantalla para permitir al usuario ajustar la configuracion de la fecha y la hora.
 */
void enterTimeSettingMode() {
    tft.fillScreen(TFT_BLACK);
    updateDisplay();
    EDIT_CLOCK_LOGIC.ischangeTime = true;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

/*****************************************************/
/*****************************************************/
/*****************************************************/
