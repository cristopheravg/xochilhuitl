/*
 ############################################################
 ########### INSTITUCIÓN EDUCATIVA XOCHILHUITL ##############
 ###########         NIVEL SECUNDARIA            ############ 
 ############################################################

  ==========================================================
  PROYECTO: Pluma automática con sensor ultrasónico
  ==========================================================

  DESCRIPCIÓN:
  Sistema automático que controla una pluma vehicular mediante
  un sensor ultrasónico. Detecta objetos cercanos para abrir
  la pluma y la cierra después de un tiempo sin detección.
  La interfaz visual muestra el estado mediante un gráfico
  representativo en pantalla OLED.

  AUTOR:  Cristopher Avila Gallegos
  FECHA DE CREACIÓN:  19/03/2026
  MATERIA:  Robótica I, II, III

  ----------------------------------------------------------
  [FUNCIONAMIENTO]:
  - El sensor HC-SR04 mide la distancia continuamente.
  - Si detecta un objeto a menos de 30 cm → ABRE.
  - Si no detecta objeto → espera 5 segundos y CIERRA.
  - Se usa histéresis para evitar oscilaciones.
  - Se implementa un cooldown para evitar activaciones falsas.
  - El servo se activa solo cuando se mueve (attach/detach)
    para evitar vibraciones.
  - La pantalla OLED muestra un dibujo del estado de la pluma.

  ----------------------------------------------------------
  [HARDWARE]:
  - Arduino Uno
  - Sensor ultrasónico HC-SR04
  - Servomotor SG90
  - Pantalla OLED SSD1306 SPI
  - LEDs o indicadores (opcionales en A2 y A3)
  - Fuente de 5V recomendada para el servo

  ----------------------------------------------------------
  [PINES]:

  // Sensor ultrasónico
  - Trigger → A1
  - Echo    → A0

  // Servomotor
  - Señal   → Pin 6

  // Pantalla OLED SPI
  - MOSI    → Pin 11
  - SCK     → Pin 13
  - CS      → Pin 10
  - DC      → Pin 9
  - RESET   → Pin 12

  // Salidas auxiliares
  - A2 → Indicador apertura
  - A3 → Indicador cierre

  ----------------------------------------------------------
  [NOTAS]:
  - El servo debe alimentarse con fuente externa si es posible.
  - Todas las tierras (GND) deben estar en común.
  - SPI hardware mejora la velocidad del display.
  - Evitar forzar mecánicamente el servo en sus extremos.

  ----------------------------------------------------------
  [VERSIÓN]:
  v1.2 - Interfaz gráfica mejorada con iconos
  ==========================================================
*/

#include <Servo.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==================== CONFIGURACIÓN OLED ====================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_DC     9
#define OLED_CS     10
#define OLED_RESET  12

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  &SPI, OLED_DC, OLED_RESET, OLED_CS);

// ==================== OBJETOS ====================
Servo servomotor;

// ==================== PINES ====================
const int pinTrigger = A1;
const int pinEcho = A0;

// ==================== VARIABLES ====================
float duracionPulsoUltrasonico;
float distanciaMedida;

// Control de tiempo
unsigned long ultimoTiempoDetectado = 0;
const int tiempoEspera = 5000;

// Cooldown
unsigned long tiempoUltimoCierre = 0;
const int tiempoBloqueo = 2000;

// Estado
bool plumaAbierta = false;
bool estadoAnterior = false;

// Histéresis
const int distanciaAbrir = 30;
const int distanciaCerrar = 40;

// ==========================================================
// SETUP
// ==========================================================
void setup() {

  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);

  pinMode(pinTrigger, OUTPUT);
  pinMode(pinEcho, INPUT);

  Serial.begin(9600);

  display.begin(SSD1306_SWITCHCAPVCC);
  display.setRotation(2);
  display.clearDisplay();
  display.display();

  // Estado inicial (cerrado)
  analogWrite(A3, 255);

  actualizarPantalla();
}

// ==========================================================
// LOOP PRINCIPAL
// ==========================================================
void loop() {

  // ----------- MEDICIÓN ULTRASÓNICA -----------
  digitalWrite(pinTrigger, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrigger, LOW);

  duracionPulsoUltrasonico = pulseIn(pinEcho, HIGH);
  distanciaMedida = (duracionPulsoUltrasonico * 0.0343) / 2;

  // ----------- LÓGICA DE CONTROL -----------

  // Apertura
  if (distanciaMedida < distanciaAbrir &&
      (millis() - tiempoUltimoCierre > tiempoBloqueo)) {

    if (!plumaAbierta) {
      Serial.println("Se abre pluma");
      abrirPluma();
      plumaAbierta = true;
    }

    ultimoTiempoDetectado = millis();
  }

  // Cierre
  if (plumaAbierta &&
      distanciaMedida > distanciaCerrar &&
      (millis() - ultimoTiempoDetectado >= tiempoEspera)) {

    Serial.println("Se cierra pluma");
    cerrarPluma();
    plumaAbierta = false;
    tiempoUltimoCierre = millis();
  }

  // ----------- PANTALLA -----------
  if (plumaAbierta != estadoAnterior) {
    actualizarPantalla();
    estadoAnterior = plumaAbierta;
  }

  // ----------- MONITOR SERIAL -----------
  Serial.print("Distancia: ");
  Serial.println(distanciaMedida);

  delay(120);
}

// ==========================================================
// FUNCIONES
// ==========================================================

void abrirPluma() {

  analogWrite(A2, 255);
  analogWrite(A3, 0);

  servomotor.attach(6);
  servomotor.write(0);
  delay(700);
  servomotor.detach();
}

void cerrarPluma() {

  analogWrite(A2, 0);
  analogWrite(A3, 255);

  servomotor.attach(6);
  servomotor.write(90);
  delay(700);
  servomotor.detach();
}

// ==================== OLED ====================


void actualizarPantalla() {

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // ===== TÍTULO =====
  display.setTextSize(1);
  display.setCursor(25, 5);
  display.println("CONTROL ACCESO");

  // Línea separadora
  display.drawLine(0, 18, 128, 18, SSD1306_WHITE);

  // ===== ESTADO PRINCIPAL =====
  display.setTextSize(2);

  if (plumaAbierta) {
    display.setCursor(20, 30);
    display.println("ABIERTO");
  } else {
    display.setCursor(15, 30);
    display.println("CERRADO");
  }

  display.display();
}
