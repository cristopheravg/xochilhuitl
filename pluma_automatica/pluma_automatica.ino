/*
 ############################################################
 ########### INSTITUCIÓN EDUCATIVA XOCHILHUITL ##############
 ###########         NIVEL SECUNDARIA            ############ 
 ############################################################


  ============================================
  PROYECTO: Pluma automática con sensor ultrasónico
  ============================================

  DESCRIPCIÓN:
  Este programa controla una pluma automática que se abre
  cuando detecta un objeto a menos de 30 cm y se cierra
  después de 5 segundos sin detección. Espera 2 segundos 
  una vez que el sistema se accionó para volver
  a accionar pluma.


  AUTOR:  Cristopher Avila Gallegos
  FECHA DE CREACIÓN:  19/03/2026
  MATERIA:  Robótica I,II,III;

  [FUNCIONAMIENTO]:
  El sensor ultrasónico mide la distancia continuamente.
  Si detecta un objeto cercano, el servomotor abre la pluma.
  Si el objeto se aleja, después de un tiempo la pluma se cierra.
  Se incluye un tiempo de bloqueo para evitar aperturas falsas por ruido.

  [HARDWARE]:
  - Arduino Uno
  - Sensor HC-SR04
  - Servomotor SG90
  - Material vario maqueta

  [PINES]:
  - Trigger → A1
  - Echo    → A0
  - Servo   → 3

  ============================================
*/

#include <Servo.h>

Servo servomotor;

// Pines ultrasónico
const int pinTrigger = A1;
const int pinEcho = A0;

// Variables de medición
float duracionPulsoUltrasonico, distanciaMedida;

// Control de tiempo
unsigned long ultimoTiempoDetectado = 0;
const int tiempoEspera = 5000;

// Cooldown
unsigned long tiempoUltimoCierre = 0;
const int tiempoBloqueo = 2000; // 2 segundos


// Estado de la pluma
bool plumaAbierta = false;

// Histéresis
const int distanciaAbrir = 30;
const int distanciaCerrar = 40;

void setup() {
  pinMode(pinTrigger, OUTPUT);
  pinMode(pinEcho, INPUT);
  Serial.begin(9600);

  delay(2000);
  cerrarPluma();
}

void loop() {

  // --- Medición ultrasónica ---
  digitalWrite(pinTrigger, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrigger, LOW);

  duracionPulsoUltrasonico = pulseIn(pinEcho, HIGH);
  distanciaMedida = (duracionPulsoUltrasonico * 0.0343) / 2;

  // --- Apertura con protección de cooldown ---
  if (distanciaMedida < distanciaAbrir && (millis() - tiempoUltimoCierre > tiempoBloqueo)) {

    if (!plumaAbierta) {
      Serial.println("Se abre pluma");
      abrirPluma();
      plumaAbierta = true;
    }

    ultimoTiempoDetectado = millis();
  }

  // --- Cierre controlado ---
  if (plumaAbierta && distanciaMedida > distanciaCerrar && (millis() - ultimoTiempoDetectado >= tiempoEspera)) {

    Serial.println("Se cierra pluma");
    cerrarPluma();
    plumaAbierta = false;

    // Guardamos momento de cierre (clave del cooldown)
    tiempoUltimoCierre = millis();
  }

  // --- Monitor serial ---
  Serial.print("Distancia: ");
  Serial.println(distanciaMedida);

  delay(50);

}

// Funciones
void abrirPluma() {
  servomotor.attach(3);
  servomotor.write(0);
  delay(700);
  servomotor.detach();
}

void cerrarPluma() {
  servomotor.attach(3);
  servomotor.write(90);
  delay(700);
  servomotor.detach();
}