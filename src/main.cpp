#include <Arduino.h>
#include <BluetoothSerial.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Wire.h>
#include "AS5600.h"

// Objeto para la comunicación Bluetooth Serial
BluetoothSerial SerialBT;
AS5600 as5600;

// Pines para sensores básicos
const int E18D80NK_PIN = 34;
const int AS5600_PIN = 35;

// Sensor E18D80NK
volatile unsigned long conteoCajas = 0;  
int estadoAnterior = HIGH;

// Sensor E18D80NK

// Wi-Fi
const char *ssid = "ESP32_Cautivo";
const char *password = "12345678";

// Prototipado
void mostrarMenu();
void inicializarE18D80NK();
void leerE18D80NK();
void inicializarAS5600();
int leerAS5600();
int leerAS5600Raw();

// Setup
void setup() {
  Serial.begin(9600);
  SerialBT.begin("ESP32_Menu");

  Serial.println("ESP32 listo");
  SerialBT.println("¡Bienvenido! Conectado al ESP32.");

  // Pines básicos
  pinMode(E18D80NK_PIN, INPUT);
  pinMode(AS5600_PIN, INPUT);

  // Inicializar sensores
  inicializarAS5600();
  inicializarE18D80NK();

  mostrarMenu();
}

// Loop
void loop() {
  if (SerialBT.available()) {
    char opcion = SerialBT.read();

    switch (opcion) {
      case '1': {
        int angulo = leerAS5600();
        int raw = leerAS5600Raw();
        SerialBT.print("AS5600 - Angulo: ");
        SerialBT.println(angulo);
        SerialBT.print("AS5600 - Raw: ");
        SerialBT.println(raw);
        break;
      }

      case '2': {
        inicializarE18D80NK();
        leerE18D80NK();
      }

      case '3':
        // Wi-Fi
        break;

      case '4':
        // Portal Cautivo
        SerialBT.println("Portal cautivo activado.");
        break;

      default:
        if (opcion != '\n' && opcion != '\r') {
          SerialBT.print("Opcion '");
          SerialBT.print(opcion);
          SerialBT.println("' invalida. Elige 1 a 6.");
        }
        break;
    }

    if (opcion != '\n' && opcion != '\r') {
      delay(300);
      mostrarMenu();
    }
  }
}

// Funciones
void mostrarMenu() {
  SerialBT.println("\n--- MENU BLUETOOTH ---");
  SerialBT.println("1. Lectura sensor AS5600");
  SerialBT.println("2. Lectura sensor E18-D80NK");
  SerialBT.println("3. Configurar WiFi");
  SerialBT.println("4. Portal cautivo");
  SerialBT.println("Elige una opcion (1 a 4):");
}

void inicializarE18D80NK() {
  pinMode(E18D80NK_PIN, INPUT);
  conteoCajas = 0;
  estadoAnterior = HIGH;
  Serial.println("Sistema de conteo E18-D80NK iniciado...");
}

void leerE18D80NK() {
  int estadoActual = digitalRead(E18D80NK_PIN);

  if (estadoAnterior == HIGH && estadoActual == LOW) {
    conteoCajas++;
    Serial.print("Caja detectada. Total: ");
    Serial.println(conteoCajas);
    delay(1000);
  }

  estadoAnterior = estadoActual;
}

void inicializarAS5600() {
  Wire.begin();
  as5600.begin(4); // pin de dirección
  as5600.setDirection(AS5600_CLOCK_WISE);

  if (as5600.isConnected()) {
    Serial.println("AS5600 conectado correctamente.");
  } else {
    Serial.println("Error: no se detecta el AS5600.");
  }
}

int leerAS5600() {
  return as5600.readAngle();
}

int leerAS5600Raw() {
  return as5600.rawAngle();
}