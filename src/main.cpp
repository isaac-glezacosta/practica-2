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

// Access Point
const char* AP_SSID = "ESP32_AP";
const char* AP_PASS = "12345678";

// Prototipado
void mostrarMenu();
void inicializarE18D80NK();
void leerE18D80NK();
void inicializarAS5600();
int leerAS5600();
int leerAS5600Raw();
void conectarWiFi();
void iniciarPortalCautivo();
void flushBluetoothInput();

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
        leerE18D80NK();
        break;
      }

      case '3':
        conectarWiFi();
        break;

      case '4':
        iniciarPortalCautivo();
        break;

      default:
        if (opcion != '\n' && opcion != '\r') {
          SerialBT.print("Opcion '");
          SerialBT.print(opcion);
          SerialBT.println("' invalida. Elige 1 a 4.");
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
  as5600.begin(4);
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

void conectarWiFi() {
  String ssid = "";
  String password = "";

  // Limpiar buffer antes de empezar
  flushBluetoothInput();
  
  SerialBT.println("Ingresa SSID de la red WiFi:"); 
  while (ssid == "") {
    if (SerialBT.available()) {
      ssid = SerialBT.readString();
      ssid.trim();
    }
    delay(100);
  }
  SerialBT.print("SSID recibido: ");
  SerialBT.println(ssid);
  
  SerialBT.println("Ingresa la contraseña de la red:");
  while (password == "") {
    if (SerialBT.available()) {
      password = SerialBT.readString();
      password.trim();
    }
    delay(100);
  }
  SerialBT.print("Contraseña recibida: ");
  SerialBT.println(password);
  
  // Validar que ambos campos no estén vacíos
  if (ssid.length() == 0 || password.length() == 0) {
    SerialBT.println("Error: SSID y contraseña no pueden estar vacíos.");
    return;
  }
  
  SerialBT.println("Conectando a WiFi...");
  
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500); SerialBT.print("."); 
  } if (WiFi.status() == WL_CONNECTED) { 
    SerialBT.println("\nConectado a WiFi con éxito!");
    SerialBT.print("IP: ");
    SerialBT.println(WiFi.localIP()); 
  } else {
    SerialBT.println("\nNo se pudo conectar. Verifica SSID/contraseña."); 
  } 
}

void iniciarPortalCautivo() {
  SerialBT.println("Iniciando punto de acceso...");

  Serial.print("Creando red Wi-Fi co el nombre: ");
  Serial.println(AP_SSID);

  if (WiFi.softAP(AP_SSID, AP_PASS)) {
    Serial.println("Red creada exitosamente");
    Serial.print("La dirección IP del Access Point es: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Error al crear la red Wi-Fi");
  }
}

void flushBluetoothInput() {
  while (SerialBT.available()) {
    SerialBT.read();
  }
  delay(50);
}