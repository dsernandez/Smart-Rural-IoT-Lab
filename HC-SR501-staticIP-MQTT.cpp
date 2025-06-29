
/****************************************
 *            SENSOR PIR                *  
 ****************************************
El sensor PIR (sensor infrarrojo pasivo), que mide la luz infrarroja (IR) reflejada en los objetos situados en su campo de visión. Principalmente se usa como detector de movimiento. Sus principales aplicaciones son para sistemas de seguridad, iluminación o climatización. Modelo aproximado HC-SR501

Orden de los pines 1,  2, 3 => Pin 1 cercano al chip rectangular grande
Pin 1 = GND
Pin 2 = Data/Signal = D2
Pin 3 = VCC 3,3V
Pin D5 = LED positivo
GND = LED negativo

Potenciómetros:
===============
- Potenciómetro de sensibilidad: Ajusta rango sensibilidad de detección del sensor. De 3 a 7 metros, girando en sentido del reloj se aumenta. (más cercano al integrado rectangular grande)

- Potenciómetro de tiempo de retención: Ajusta el tiempo que el sensor permanece activo una vez ha detectado movimiento, girando también en sentido de las agujas del reloj.

Datos Técnicos
==============
   - Voltaje de operación: 4.5VDC - 20VDC
   - Consumo de corriente en reposo: <50uA
   - Voltaje de salida: 3.3V (alto) / 0V (bajo)
   - Rango de detección: 3 a 7 metros, ajustable mediante Trimmer (Sx)
   - Angulo de detección: <100º (cono)
   - Tiempo de retardo: 5-200 S (puede ser ajustado (Tx), por defecto 5S +-3%)
   - Tiempo de bloqueo: 2.5 S (por defecto)
   - Temperatura de trabajo: -20ºC hasta 80ºC
   - Dimensión: 3.2 cm x 2.4 cm x 1.8 cm (aprox.)
   - Redisparo configurable mediante jumper de soldadura

*/
// Librerías
//==========
#include <Arduino.h>
#include <WiFi.h> // Librería para usar WiFi
#include <PubSubClient.h>

//-------------------------------------------------------------------------------------------
// CONFIGURACIONES
//-------------------------------------------------------------------------------------------
// Configuración DE RED 
//=====================
const char *ssid = "dragino-2187ac";
const char *password = "PASSWORD-WiFi";   // No se especifica por seguridad

// Static IP configuration
IPAddress staticIP(10, 130, 1, 105); // ESP32 static IP
IPAddress gateway(10, 130, 1, 239);    // IP Address of your network gateway (router)
IPAddress subnet(255, 255, 255, 0);   // Subnet mask
IPAddress primaryDNS(8, 8, 8, 8); // Primary DNS (optional)
IPAddress secondaryDNS(0, 0, 0, 0);   // Secondary DNS (optional)

// SERVIDOR MQTT
//==============
const char *mqttServer = "10.201.54.164";
const int mqttPort = 1883;
const char *mqttUser = "";
const char *mqttPassword = "";
const char *mqttTopic = "/arduino/pir/data";


// OBJETOS Y CONSTANTES
//======================
WiFiClient espClient;
PubSubClient client(espClient);

String clientID = "ESP32_" + String(random(1000, 9999)); // Cliente
const char* sensorId = "HC-SR501";

// Inicializar valor  PIR
int valor = 0;


// Tiempo del intervalo
// const unsigned long interval = 60000; // 1 minuto para pruebas
// const unsigned long interval = 600000; // 10 minutos
 const unsigned long interval = 1000; // 1 segundo

unsigned long previousMillis = 0;


//-------------------------------------------------------------------------------------------
//  FUNCIONES
//-------------------------------------------------------------------------------------------
// Función WIFI
//=============
void initWifi() {
  Serial.begin(115200);
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Intentando establecer la conexión WiFi...");
  }
  Serial.println("Conexión WiFi establecida");

  Serial.println("===========================");
  Serial.println("Información de la conexión:");
  Serial.println("===========================");
  Serial.println("Dirección IP: " + WiFi.localIP().toString());
  Serial.println("Máscara de red: " + WiFi.subnetMask().toString());
  Serial.println("Gateway: " + WiFi.gatewayIP().toString());
  Serial.println("Dirección Mac: " + WiFi.macAddress());

  // Configuring static IP
    if(!WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS)) {
      Serial.println("Failed to configure Static IP");
    } else {
      Serial.println("Static IP configured!");
    }
  
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());  // Print the ESP32 IP address to Serial Monitor
    Serial.print("Gateway (router) IP: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Subnet Mask: " );
    Serial.println(WiFi.subnetMask());
    Serial.print("Primary DNS: ");
    Serial.println(WiFi.dnsIP(0));
    Serial.print("Secondary DNS: ");
    Serial.println(WiFi.dnsIP(1));
}

// CONEXIÓN BROKER
//================
void connectToMQTT() {
  Serial.print("Conectando a MQTT...");
  while (!client.connected()) {
    if (client.connect(clientID.c_str(), mqttUser, mqttPassword)) {
      Serial.println("\nConectado a MQTT!");
    } else {
      Serial.print("ERROR, rc=");
      Serial.print(client.state());
      if (client.state() == -2) {
        Serial.println("ERROR: No se pudo resolver el hostname del servidor MQTT");
      } else if (client.state() == -1) {
        Serial.println("ERROR: Fallo en la red");
      } else if (client.state() == 1) {
        Serial.println("ERROR: credenciales incorrectas");
      } else if (client.state() == 2) {
        Serial.println("ERROR: identificador de cliente incorrecto");
      } else if (client.state() == 3) {
        Serial.println("ERROR: servidor no disponible");
      } else if (client.state() == 4) {
        Serial.println("ERROR: versión MQTT incorrecta");
      } else if (client.state() == 5) {
        Serial.println("ERROR: identificador de cliente no aceptado");
      }
      Serial.println(". Intentando nuevamente en 5 segundos...");
      delay(5000);
    }
  }
}

// Enviar MQTT
//==============
void sendMqttData(const String& jsonData) {
  if (!client.connected()) {
    Serial.println("MQTT desconectado, intentando reconectar...");
    connectToMQTT();
  }

  if (client.publish(mqttTopic, jsonData.c_str())) {
    Serial.println("Datos enviados: " + jsonData);
  } else {
    Serial.println("Error al enviar los datos.");
  }
}

// Función enviar datos MQTT en formato string JSON
//===========================================
String createJson(const char* id, int dato) {
  String json = "{";
  json += "\"sensor_id\": \"" + String(id) + "\", ";
  json += "\"presencia\": " + String(dato);
  json += "}";
  return json;
}

// PINES de la ESP32
const int PIR = 2;
const int LED = 5;

// Función usar PIR y LED
void usePIR() {
  // Leer PIR
  valor = digitalRead(PIR);
  String jsonMessage;

  unsigned long currentMillis = millis();

  if (currentMillis-previousMillis >= interval){
  previousMillis = currentMillis;  
    // Si hay movimiento, se activa LED y se publica por MQTT
    if (valor == HIGH) {
      digitalWrite(LED, HIGH);
      delay(1000); // Tiempo encendido del LED - 1 segundo
      jsonMessage = createJson(sensorId, 1); // Envía 1 si se detecta movimiento
      sendMqttData(jsonMessage); // Publicar mensaje JSON solo cuando es 1, comentar si se descomenta debajo
    }else{
      digitalWrite(LED, LOW);
      jsonMessage = createJson(sensorId, 0); // Envía 1 si se detecta movimiento
    }
    Serial.println(jsonMessage); // Mostrar el JSON creado 
    // sendMqttData(jsonMessage); // Publicar mensaje JSON con 1 o 0, descomentar para publicar 1 y 0
  }
}

//-------------------------------------------------------------------------------------------
//  FUNCIONES BASE ARDUINO
//-------------------------------------------------------------------------------------------
// Función SETUP
//==============
void setup() {
  // Configuración de pines
  pinMode(PIR, INPUT);
  pinMode(LED, OUTPUT);
  // Inicializa WiFi y MQTT
  initWifi();
  //Configurar Broker MQTT
  client.setServer(mqttServer, mqttPort);
  //Conectar a MQTT
  connectToMQTT();
}

// Función LOOP
//=============
void loop() {
  // Loop Broker
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();
  // Usar PIR
  usePIR();
}
