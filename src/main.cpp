#include <Arduino.h>
#include <BluetoothSerial.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include "AS5600.h"

// Declaración de variables
const int E18D80NK_PIN = 26;
const int AS5600_SDA = 21;
const int AS5600_SCL = 22;
bool modeBleActivo = false;

volatile unsigned int conteoCajas = 0;
int estadoAnterior = HIGH;
int estadoActual = LOW;
unsigned ultimoTiempoLectura = 0;
const unsigned long intervaloLectura = 500;
int ultimoAnguloAS5600=0;

// Access point
const char* AP_SSID = "ESP32_AP";
const char* AP_PASS = "12345678";

// Dirección del servidor
const char* serverUrl = "http://89.117.53.122:8004/datosE4";

BluetoothSerial SerialBT;

// Configuración BLE
#define SERVICE_UIID "e84fb5de-f911-4c55-a178-d0f47f736b41"
#define CHARACTERISTIC_UUID  "4c5697a1-1d67-4722-9e7a-2190021d0f89"
#define CHARACTERISTIC2_UUID "72090ab7-7994-480b-a997-fbe3965b414b"
BLECharacteristic *pCharacteristic;
BLECharacteristic *pCharacteristic2;

// portal cautivo
WebServer server(80);
DNSServer dnsServer;
AS5600 as5600;

// Portal cautivo
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

// Prototipado
void mostrarMenu();
void leerE18D80NK();
void leerAS5600();
void conectarWiFi();
void activarModoBLE();
void iniciarPortalCautivo();
void handleRoot();
void handleNotFound();
void flushBluetoothInput();
void conectarHttp();


// Setup
void setup() {
  Serial.begin(9600);
  SerialBT.begin("ESP32_Bluetooth");
  
  pinMode(E18D80NK_PIN, INPUT);
  Wire.begin(AS5600_SDA, AS5600_SCL);
  delay(100);
  
  as5600.begin(4);
  as5600.setDirection(AS5600_CLOCK_WISE);
  delay(100);
  
  Serial.println("Sistema iniciado");
  SerialBT.println("¡Bienvenido! Conectado al ESP32 por Bluetooth");
  mostrarMenu();
}

// Loop
void loop() {
  if (!modeBleActivo) {
    if (SerialBT.available()) {
      char opcion = SerialBT.read();
      
      switch (opcion) {
        case '1':
          SerialBT.println(ultimoAnguloAS5600);
          break;
        case '2':
          estadoActual = digitalRead(E18D80NK_PIN);
          if (estadoActual == LOW) { 
            SerialBT.println("OBSTACULO");
          } else {
            SerialBT.println("LIBRE");
          }
          SerialBT.print("Cajas totales: ");
          SerialBT.println(conteoCajas);
          break;
        case '3':
          conectarWiFi();
          delay(1000);
          delay(1000);
          break;
        case '4':
          iniciarPortalCautivo();
          break;
        case '5':
          SerialBT.println("Cambiando a modo BLE. Te desconectarás.");
          delay(1000);
          activarModoBLE();
          modeBleActivo = true;
          break;
        default:
          if (opcion != '\n' && opcion != '\r') {
            SerialBT.println("Opción inválida. Elige 1 a 4.");
          }
          break;
      }
      
      if (opcion != '\n' && opcion != '\r') {
        delay(300);
        mostrarMenu();
      }
    }
  } else {
      Serial.println("Modo BLE activado");

      int cajasTotales = conteoCajas;
      String valueString = String(cajasTotales);
      
      int angulo = 0;
      if (as5600.isConnected()) {
        angulo = as5600.readAngle();
      }
      String valueString2 = String(angulo);

      pCharacteristic->setValue(valueString.c_str());
      pCharacteristic->notify();
      pCharacteristic2->setValue(valueString2.c_str());
      pCharacteristic2->notify();

      Serial.println("Valores BLE actualizado");
      Serial.print("Cajas totales: ");
      Serial.println(cajasTotales);
      Serial.print("Ángulo: ");
      Serial.println(angulo);
      delay(5000);
  }
  

  if (WiFi.getMode() == WIFI_AP) {
    dnsServer.processNextRequest();  // Procesar DNS para portal cautivo
    server.handleClient();
  }

  if (WiFi.status() == WL_CONNECTED) {
    conectarHttp();
  }

   // Actualización periódica de sensores
  if (millis() - ultimoTiempoLectura >= intervaloLectura) {
    ultimoTiempoLectura = millis();

    // Sensor E18
    estadoActual = digitalRead(E18D80NK_PIN);
    if (estadoAnterior == HIGH && estadoActual == LOW) {
      conteoCajas++;
    }
    estadoAnterior = estadoActual;

    // Sensor AS5600
    if (as5600.isConnected()) {
      ultimoAnguloAS5600 = as5600.readAngle();
    }
  }

  // --- Portal cautivo activo ---
  if (WiFi.getMode() == WIFI_AP) {
    dnsServer.processNextRequest();
    server.handleClient();
  }
}

// Funciones
void mostrarMenu() {
  SerialBT.println("\n--- MENU BLUETOOTH ---");
  SerialBT.println("1. Leer sensor AS5600");
  SerialBT.println("2. Leer sensor E18-D80NK");
  SerialBT.println("3. Conectar a WiFi");
  SerialBT.println("4. Iniciar portal cautivo");
  SerialBT.println("5. Cambiar a modo BLE");
  SerialBT.println("Elige una opción (1-5):");
}

void leerE18D80NK() {
  int estadoActual = digitalRead(E18D80NK_PIN);
  
  if (estadoAnterior == HIGH && estadoActual == LOW) {
    conteoCajas++;
    SerialBT.print("Caja detectada. Total: ");
    SerialBT.println(conteoCajas);
  }
  
  estadoAnterior = estadoActual;
  SerialBT.print("Estado sensor: ");
  SerialBT.println(estadoActual == HIGH ? "LIBRE" : "OBSTACULO");
}

void leerAS5600() {
  if (as5600.isConnected()) {
    int angulo = as5600.readAngle();
    int raw = as5600.rawAngle();
    
    SerialBT.print("Ángulo: ");
    SerialBT.print(angulo);
    SerialBT.print(" | Valor RAW: ");
    SerialBT.println(raw);
  } else {
    SerialBT.println("Error: Sensor AS5600 no detectado");
  }
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
    delay(500); 
    SerialBT.print("."); 
  } 
  
  if (WiFi.status() == WL_CONNECTED) { 
    SerialBT.println("\nConectado a WiFi con éxito!");
    SerialBT.print("IP: ");
    SerialBT.println(WiFi.localIP()); 
  } else {
    SerialBT.println("\nNo se pudo conectar. Verifica SSID/contraseña."); 
  } 
}

void activarModoBLE() {
  // Detener BT clásico
  SerialBT.end();
  Serial.println("Servicio Bluetooth clásico detenido.");

  // Iniciar servidor BLE
  Serial.println("Iniciando Servidor BLE...");
  BLEDevice::init("ESP32_Sensor_BLE__");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UIID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  
  BLEDescriptor *sensor1 = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  sensor1->setValue("Contador de Cajas");
  pCharacteristic->addDescriptor(sensor1);

  pCharacteristic2 = pService->createCharacteristic(
                      CHARACTERISTIC2_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  
  BLEDescriptor *sensor2 = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  sensor2->setValue("Angulo AS5600");
  pCharacteristic2->addDescriptor(sensor2);
  
  pService->start();
  BLEDevice::startAdvertising();
  Serial.println("Dispositivo anunciándose por BLE.");
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32 - Sistema de Sensores</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f0f0f0; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; text-align: center; }";
  html += ".sensor-data { background: #e9ecef; padding: 15px; margin: 10px 0; border-radius: 5px; }";
  html += ".refresh-btn { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 10px 0; }";
  html += ".refresh-btn:hover { background: #0056b3; }";
  html += "</style>";
  html += "<script>";
  html += "function autoRefresh() { setTimeout(function(){ location.reload(); }, 2000); }";
  html += "</script>";
  html += "</head><body onload='autoRefresh()'>";
  html += "<div class='container'>";
  html += "<h1>ESP32 - Sistema de Sensores</h1>";
  
  html += "<div class='sensor-data'>";
  html += "<h3>Contador de Cajas</h3>";
  html += "<p><strong>Total detectado: " + String(conteoCajas) + "</strong></p>";
  html += "</div>";
  
  html += "<div class='sensor-data'>";
  html += "<h3>Sensor AS5600 (Magnético)</h3>";
  if (as5600.isConnected()) {
    int angulo = as5600.readAngle();
    float grados = angulo * 0.087890625;
    html += "<p>Ángulo: <strong>" + String(angulo) + "</strong> (RAW)</p>";
    html += "<p>Grados: <strong>" + String(grados, 1) + "°</strong></p>";
    html += "<p>Estado: <span style='color: green;'>Conectado</span></p>";
  } else {
    html += "<p>Estado: <span style='color: red;'>No conectado</span></p>";
  }
  html += "</div>";
  
  html += "<div class='sensor-data'>";
  html += "<h3>Sensor E18-D80NK (Proximidad)</h3>";
  int estadoActual = digitalRead(E18D80NK_PIN);
  html += "<p>Estado: <strong>" + String(estadoActual == HIGH ? "LIBRE" : "OBSTÁCULO DETECTADO") + "</strong></p>";
  html += "<p>Color: <span style='color: " + String(estadoActual == HIGH ? "green" : "red") + ";'>●</span></p>";
  html += "</div>";
  
  html += "<button class='refresh-btn' onclick='location.reload()'>Actualizar</button>";
  html += "<p style='text-align: center; color: #666; font-size: 12px;'>Actualización automática cada 2 segundos</p>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleNotFound() {
  // Rediriger todas las peticiones a la página principal
  handleRoot();
}

void iniciarPortalCautivo() {
  SerialBT.println("Iniciando portal cautivo...");
  
  // Configurar AP con IP específica
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  
  if (WiFi.softAP(AP_SSID, AP_PASS)) {
    SerialBT.print("Portal cautivo creado: ");
    SerialBT.println(AP_SSID);
    SerialBT.print("IP: ");
    SerialBT.println(WiFi.softAPIP());
    
    // Configurar servidor DNS para portal cautivo
    dnsServer.start(DNS_PORT, "*", apIP);
    
    // Configurar rutas del servidor web
    server.on("/", handleRoot);
    server.on("/index.html", handleRoot);
    server.on("/generate_204", handleRoot);  // Android
    server.on("/fwlink", handleRoot);        // Microsoft
    server.onNotFound(handleNotFound);       // Capturar todas las demás peticiones
    
    server.begin();
    SerialBT.println("Servidor web iniciado");
    SerialBT.println("Conéctate a la red WiFi 'ESP32_AP'");
    SerialBT.println("El portal se abrirá automáticamente");
    SerialBT.println("O visita: http://192.168.4.1");
  } else {
    SerialBT.println("Error al crear el punto de acceso");
  }
}

void flushBluetoothInput() {
  while (SerialBT.available()) {
    SerialBT.read();
  }
  delay(50);
}

void conectarHttp() {
  // Solo intentar enviar datos si estamos conectados a Wi-Fi
  if (WiFi.status() == WL_CONNECTED) {

    // Crear un objeto HTTPClient
    HTTPClient http;

    // Especificar el endpoint del servidor
    http.begin(serverUrl);

    // Anadir una cabecera para indicar que enviaremos datos en formato JSON
    http.addHeader("Content-Type", "application/json");

    // Leer datos
    int cajasTotales = conteoCajas;
      String valueString = String(cajasTotales);
      
      int angulo = 0;
      if (as5600.isConnected()) {
        angulo = as5600.readAngle();
      }
      String valueString2 = String(angulo);

    // 2. Crear el cuerpo (payload) de la peticion en formato JSON
    String jsonPayload = "{\"angulo\":" + String(angulo) + ",\"conteo_cajas\":" + String(cajasTotales) + "}";

    // 3. Enviar la peticion POST y obtener el codigo de respuesta
    int httpResponseCode = http.POST(jsonPayload);

    // 4. Verificar la respuesta del servidor
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("Codigo de respuesta HTTP: ");
      Serial.println(httpResponseCode);
      Serial.print("Respuesta del servidor: ");
      Serial.println(response);
    } else {
      Serial.print("Error en la peticion HTTP. Codigo: ");
      Serial.println(httpResponseCode);
    }

    // Liberar los recursos
    http.end();
  } else {
    Serial.println("Error en la conexion WI-FI");
  }

  delay(10000);
}