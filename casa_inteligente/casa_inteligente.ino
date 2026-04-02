#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_BT"); // Nombre del dispositivo Bluetooth
  Serial.println("Bluetooth iniciado, listo para emparejar");
  pinMode(LED_BUILTIN,OUTPUT);
}

void loop() {
  // Leer datos desde Bluetooth y enviarlos al monitor serial
  if (SerialBT.available()) {
    char dato = SerialBT.read();
    Serial.write(dato);

    if(dato == 97){
      Serial.println("LLEGO UNA A");
      digitalWrite(LED_BUILTIN,HIGH);
    }

    else if(dato == 98){
      Serial.println("LLEGO UNA B");
      digitalWrite(LED_BUILTIN,LOW);
    }

  }

  // Leer datos desde el monitor serial y enviarlos por Bluetooth
  if (Serial.available()) {
    char dato = Serial.read();
    SerialBT.print(dato);
  }
}