
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
  v1.1 - Control mejorado del servo (sin vibración)
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

// Histéresis
const int distanciaAbrir = 30;
const int distanciaCerrar = 40;

// Variables de texto OLED
int16_t x1, y1;
uint16_t w, h;

// ==========================================================
// SETUP
// ==========================================================
void setup() {

  // Pines auxiliares
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);

  // Sensor ultrasónico
  pinMode(pinTrigger, OUTPUT);
  pinMode(pinEcho, INPUT);

  Serial.begin(9600);

  // Inicialización pantalla
  display.begin(SSD1306_SWITCHCAPVCC);
  display.setRotation(2);
  display.clearDisplay();
  delay(200);

  // Estado inicial (cerrado)
  analogWrite(A3, 255);
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
  actualizarPantalla();

  // ----------- MONITOR SERIAL -----------
  Serial.print("Distancia: ");
  Serial.println(distanciaMedida);

  delay(100);
}

// ==========================================================
// FUNCIONES
// ==========================================================

void abrirPluma() {

  // Indicadores
  analogWrite(A2, 255);
  analogWrite(A3, 0);

  // Movimiento servo
  servomotor.attach(6);
  servomotor.write(0);
  delay(700);
  servomotor.detach();
}

void cerrarPluma() {

  // Indicadores
  analogWrite(A2, 0);
  analogWrite(A3, 255);

  // Movimiento servo
  servomotor.attach(6);
  servomotor.write(90);
  delay(700);
  servomotor.detach();
}

void actualizarPantalla() {

  display.clearDisplay();

  // Título
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.getTextBounds("Xochilhuitl", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 0);
  display.println("Xochilhuitl");

  // Estado
  display.setTextSize(2);

  if (plumaAbierta) {
    display.getTextBounds("ABIERTO", 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 30);
    display.println("ABIERTO");
  } else {
    display.getTextBounds("CERRADO", 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 30);
    display.println("CERRADO");
  }

  // Distancia
  display.setTextSize(1);
  String texto = "Dist: " + String(distanciaMedida, 1) + " cm";

  display.getTextBounds(texto, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 54);
  display.println(texto);

  display.display();
}

