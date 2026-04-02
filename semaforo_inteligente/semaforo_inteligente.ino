/*
 ############################################################
 ########### INSTITUCIÓN EDUCATIVA XOCHILHUITL ##############
 ###########         NIVEL SECUNDARIA            ############ 
 ############################################################

 PROYECTO: Semáforo con control automático y Bluetooth

 DESCRIPCIÓN:
 - Secuencia automática de semáforo (verde, amarillo, rojo) sin bloquear.
 - Posibilidad de control manual de LEDs vía Bluetooth.
 - Se puede volver a la secuencia automática en cualquier momento.

 AUTOR: Cristopher Avila Gallegos
 FECHA: 26/03/2026
*/

#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

// ===============================
// DEFINICIÓN DE PINES
// ===============================
const int verde = 18;
const int amarillo = 19;
const int rojo = 21;

// ===============================
// TIEMPOS (milisegundos)
// ===============================
const unsigned long tiempo_verde = 10000;
const unsigned long tiempo_rojo = 10000;
const unsigned long tiempo_amarillo_fijo = 2000;
const unsigned long tiempo_parpadeo = 250; // para parpadeo amarillo
const int parpadeos = 4; // número de parpadeos

// ===============================
// VARIABLES DE CONTROL
// ===============================
unsigned long ultimoCambio = 0;
int estado = 0;  // 0=verde, 1=amarillo fijo, 2=amarillo parpadeo, 3=rojo
int parpadeo_contador = 0;
bool parpadeo_on = false;

bool modoAutomatico = true; // true = secuencia automática, false = control manual

void setup() {
  Serial.begin(115200);
  SerialBT.begin("iSEMAFORO");
  Serial.println("Bluetooth iniciado, listo para emparejar");

  // Configuración de pines
  pinMode(verde, OUTPUT);
  pinMode(amarillo, OUTPUT);
  pinMode(rojo, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // Apagar todos los LEDs (modo current sink)
  digitalWrite(verde, HIGH);
  digitalWrite(amarillo, HIGH);
  digitalWrite(rojo, HIGH);

  ultimoCambio = millis();
}

// ===============================
// LOOP PRINCIPAL
// ===============================
void loop() {
  unsigned long ahora = millis();

  // -------------------------------
  // Lectura de Bluetooth
  // -------------------------------
  if (SerialBT.available()) {
    char dato = SerialBT.read();
    Serial.write(dato); // mostrar en monitor

    switch (dato) {
      case 'A': // Encender verde manual
        modoAutomatico = false;
        digitalWrite(verde, LOW);
        digitalWrite(amarillo, HIGH);
        digitalWrite(rojo, HIGH);
        Serial.println("Verde manual");
        break;

      case 'B': // Encender amarillo manual
        modoAutomatico = false;
        digitalWrite(verde, HIGH);
        digitalWrite(amarillo, LOW);
        digitalWrite(rojo, HIGH);
        Serial.println("Amarillo manual");
        break;

      case 'C': // Encender rojo manual
        modoAutomatico = false;
        digitalWrite(verde, HIGH);
        digitalWrite(amarillo, HIGH);
        digitalWrite(rojo, LOW);
        Serial.println("Rojo manual");
        break;

      case 'D': // Volver a modo automático
        modoAutomatico = true;
        estado = 0; // reiniciar secuencia automática
        ultimoCambio = ahora;
        parpadeo_contador = 0;
        parpadeo_on = false;
        Serial.println("Modo automático");
        break;

      default:
        break;
    }
  }

  // -------------------------------
  // Secuencia automática (no bloqueante)
  // -------------------------------
  if (modoAutomatico) {
    switch (estado) {
      case 0: // Verde
        digitalWrite(verde, LOW);
        digitalWrite(amarillo, HIGH);
        digitalWrite(rojo, HIGH);
        if (ahora - ultimoCambio >= tiempo_verde) {
          estado = 1;
          ultimoCambio = ahora;
        }
        break;

      case 1: // Amarillo fijo
        digitalWrite(amarillo, LOW);
        digitalWrite(verde, HIGH);
        digitalWrite(rojo, HIGH);
        if (ahora - ultimoCambio >= tiempo_amarillo_fijo) {
          estado = 2;
          ultimoCambio = ahora;
          parpadeo_contador = 0;
          parpadeo_on = false;
        }
        break;

      case 2: // Amarillo parpadeo
        if (ahora - ultimoCambio >= tiempo_parpadeo) {
          ultimoCambio = ahora;
          parpadeo_on = !parpadeo_on;
          digitalWrite(amarillo, parpadeo_on ? LOW : HIGH);

          if (!parpadeo_on) parpadeo_contador++;

          if (parpadeo_contador >= parpadeos) {
            digitalWrite(amarillo, HIGH); // apagar
            estado = 3;
            ultimoCambio = ahora;
          }
        }
        break;

      case 3: //Rojo
        digitalWrite(rojo, LOW);
        digitalWrite(verde, HIGH);
        digitalWrite(amarillo, HIGH);
        if (ahora - ultimoCambio >= tiempo_rojo) {
          estado = 0; // volver a rojo
          ultimoCambio = ahora;
        }
        break;
    }
  }

  // Aquí puedes agregar otras tareas que no bloqueen la ejecución
  // por ejemplo: lectura de sensores, botones, comunicación adicional, etc.
}