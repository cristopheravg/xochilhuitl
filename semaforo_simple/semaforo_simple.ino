/*
 ############################################################
 ########### INSTITUCIÓN EDUCATIVA XOCHILHUITL ##############
 ###########         NIVEL SECUNDARIA            ############ 
 ############################################################

  ==========================================================
  PROYECTO: Semáforo básico con LEDs
  ==========================================================

  DESCRIPCIÓN:
  Sistema que simula el funcionamiento de un semáforo utilizando
  tres LEDs (verde, amarillo y rojo). El sistema sigue un ciclo
  automático de encendido y apagado con tiempos definidos.

  AUTOR:  Cristopher Avila Gallegos
  FECHA DE CREACIÓN:  26/03/2026
  MATERIA:  Robótica I, II, III

  ----------------------------------------------------------
  [FUNCIONAMIENTO]:
  - El LED verde se enciende durante 10 segundos (avance).
  - El LED amarillo se enciende 2 segundos (precaución).
  - El LED amarillo parpadea para indicar cambio de estado.
  - El LED rojo se enciende 10 segundos (alto).
  - El ciclo se repite indefinidamente.

  ⚠️ IMPORTANTE:
  - El sistema está conectado en modo CURRENT SINK:
    LOW  = LED ENCENDIDO
    HIGH = LED APAGADO

  ----------------------------------------------------------
  [HARDWARE]:
  - ESP32 o Arduino
  - 3 LEDs (verde, amarillo, rojo)
  - 3 resistencias (220Ω – 330Ω recomendadas)
  - Protoboard y cables

  ----------------------------------------------------------
  [PINES]:

  - LED VERDE     → Pin 12
  - LED ROJO      → Pin 14
  - LED AMARILLO  → Pin 26

  ----------------------------------------------------------
  [NOTAS]:
  - Los LEDs deben conectarse en configuración current sink
    (ánodo a Vcc mediante resistencia, cátodo al pin).
  - Al iniciar el microcontrolador, los pines pueden flotar,
    por lo que se inicializan en HIGH para evitar encendidos
    no deseados.
  - El uso de delay() bloquea el programa, pero es útil para
    fines educativos.

  ----------------------------------------------------------
  [VERSIÓN]:
  v1.0 - Implementación básica del semáforo
  ==========================================================
*/

// ===============================
// DEFINICIÓN DE PINES
// ===============================
const int verde = 12;
const int rojo = 14;
const int amarillo = 26;

// ===============================
// TIEMPOS (milisegundos)
// ===============================
const int tiempo_verde = 10000;
const int tiempo_rojo = 10000;

// ==========================================================
// SETUP
// ==========================================================
void setup() {

  // Apagar todos los LEDs (modo current sink)
  digitalWrite(verde, HIGH);
  digitalWrite(rojo, HIGH);
  digitalWrite(amarillo, HIGH);

  // Configuración de pines como salida
  pinMode(verde, OUTPUT);
  pinMode(rojo, OUTPUT);
  pinMode(amarillo, OUTPUT);

  // Asegurar estado inicial apagado
  digitalWrite(verde, HIGH);
  digitalWrite(rojo, HIGH);
  digitalWrite(amarillo, HIGH);
}

// ==========================================================
// LOOP PRINCIPAL
// ==========================================================
void loop() {

  // ===========================
  // 🟢 VERDE (AVANCE)
  // ===========================
  digitalWrite(verde, LOW);
  delay(tiempo_verde);
  digitalWrite(verde, HIGH);

  // ===========================
  // 🟡 AMARILLO (PRECAUCIÓN)
  // ===========================

  // Encendido fijo
  digitalWrite(amarillo, LOW);
  delay(2000);

  // -------- PARPADEO --------
  for (int i = 0; i < 4; i++) {
    digitalWrite(amarillo, HIGH);
    delay(250);
    digitalWrite(amarillo, LOW);
    delay(250);
  }

  digitalWrite(amarillo, HIGH); // Apagar

  // ===========================
  // 🔴 ROJO (ALTO)
  // ===========================
  digitalWrite(rojo, LOW);
  delay(tiempo_rojo);
  digitalWrite(rojo, HIGH);
}