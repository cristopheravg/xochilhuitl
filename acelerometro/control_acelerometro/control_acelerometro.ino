#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>



// ======== CONFIG WIFI ========
const char* ssid = "IZZI-6B12";
const char* password = "3Q5Y6QXWZWVO";

//const char* ssid = "IZZI-5F93";
//const char* password = "FCAE34B95F93";


// IP de tu PC (donde correrá Python)
const char* udpAddress = "192.168.0.88"; 
const int udpPort = 1234;

WiFiUDP udp;


// ======== PUERTO RECEPTOR ========
const int udpPortRX = 1235;
WiFiUDP udpRX;


// ======== MPU6050 ========
const int MPU = 0x68;

// ======== VARIABLES ========
int16_t AcX, AcY, AcZ;

float angleX, angleY;
float angleX_f = 0, angleY_f = 0;

// calibración
float offsetX = 0, offsetY = 0;

// filtro
float alpha = 0.1;


// ======== ALERTAS ========
// Define tus pines de salida
#define PIN_LED_ALERTA  23
#define PIN_BUZZER      18


// Inicializacion variables alarma
unsigned long previousMillis = 0; // Última vez que se cambió el estado
const long interval = 1000;       // Intervalo en milisegundos (1 segundo)
bool ledState = LOW;              // Estado actual del LED





// ======== SETUP ========
void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN,OUTPUT);

  // I2C
  Wire.begin(21, 22);

  // despertar MPU6050
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");



  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.println(WiFi.localIP());

  udpRX.begin(udpPortRX);



  // ======== CALIBRACIÓN ========
  Serial.println("Calibrando... deja el sensor quieto");

  for (int i = 0; i < 100; i++) {
    leerMPU();

    float ax = atan2(AcY, AcZ) * 180 / PI;
    float ay = atan2(-AcX, sqrt(AcY * AcY + AcZ * AcZ)) * 180 / PI;

    offsetX += ax;
    offsetY += ay;

    delay(20);
  }

  offsetX /= 100;
  offsetY /= 100;

  Serial.println("Calibracion lista");

}

// ======== LOOP ========
void loop() {
  leerMPU();

  // calcular ángulos
  angleX = atan2(AcY, AcZ) * 180 / PI;
  angleY = atan2(-AcX, sqrt(AcY * AcY + AcZ * AcZ)) * 180 / PI;

  // quitar offset
  angleX -= offsetX;
  angleY -= offsetY;

  // filtro
  angleX_f = alpha * angleX + (1 - alpha) * angleX_f;
  angleY_f = alpha * angleY + (1 - alpha) * angleY_f;

  // zona muerta
  if (abs(angleX_f) < 2) angleX_f = 0;
  if (abs(angleY_f) < 2) angleY_f = 0;

  // enviar datos
  String data = String(angleX_f) + "," + String(angleY_f);

  //Serial.println(data);

  udp.beginPacket(udpAddress, udpPort);
  udp.print(data);
  udp.endPacket();


  int packetSize = udpRX.parsePacket();
  if (packetSize) {
      char buf[8];
      udpRX.read(buf, sizeof(buf));
      int codigo = atoi(buf);

      switch (codigo) {
          case 0:
              digitalWrite(LED_BUILTIN, LOW);
              Serial.println("LLEGO ALARMA CODIGO 0");
              //digitalWrite(PIN_LED_ALERTA, LOW);
              //digitalWrite(PIN_BUZZER, LOW);
              break;
          case 1:  // altitud baja — LED + buzzer rápido
              digitalWrite(LED_BUILTIN, HIGH);
              Serial.println("LLEGO ALARMA CODIGO 1");
              //digitalWrite(PIN_LED_ALERTA, HIGH);
              //tone(PIN_BUZZER, 1000, 100);
              break;
          case 2:  // pérdida de velocidad — buzzer lento
              digitalWrite(LED_BUILTIN, HIGH);
              Serial.println("LLEGO ALARMA CODIGO 2");
              //digitalWrite(PIN_LED_ALERTA, HIGH);
              //tone(PIN_BUZZER, 500, 300);
              break;
          case 3:  // descenso rápido — solo LED
              digitalWrite(LED_BUILTIN, HIGH);
              Serial.println("LLEGO ALARMA CODIGO 3");
              //digitalWrite(PIN_LED_ALERTA, HIGH);
              //digitalWrite(PIN_BUZZER, LOW);
              break;
      }
  }




  delay(20); // ~50 Hz
}

// ======== FUNCIÓN LECTURA ========
void leerMPU() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);

  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();
}


// ====== Función de parpadeo usando millis ======
void blinkLEDAlarma() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Guardamos el último cambio

    // Cambiamos el estado del LED
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }
}