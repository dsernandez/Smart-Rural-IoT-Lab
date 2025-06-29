#include <WiFi.h>
#include <HTTPClient.h>
#include "DHTesp.h"

// Configuración de pines
constexpr int SOUND_ANALOG_PIN = 34;
constexpr int RED_LED_PIN = 15;
constexpr int YELLOW_LED_PIN = 2;    
constexpr int GREEN_LED_PIN = 4;
constexpr int DHT_PIN = 26;

// Umbrales para controlar el semáforo
constexpr int LOW_THRESHOLD = 50;
constexpr int MEDIUM_THRESHOLD = 300;
constexpr int HIGH_THRESHOLD = 600;

// Datos de la red WiFi y la URL del servidor
constexpr char WIFI_SSID[] = "dragino-2187ac";
constexpr char WIFI_PASSWORD[] = "PASSWORD-WiFi";   // No se especifica por seguridad
constexpr char SERVER_URL[] = "http://10.201.54.164:1880/iot-lab/sensors";

// Static IP configuration
IPAddress staticIP(10, 130, 1, 102); // ESP32 static IP
IPAddress gateway(10, 130, 1, 239);    // IP Address of your network gateway (router)
IPAddress subnet(255, 255, 255, 0);   // Subnet mask
IPAddress primaryDNS(8, 8, 8, 8); // Primary DNS (optional)
IPAddress secondaryDNS(0, 0, 0, 0);   // Secondary DNS (optional)

// Identificadores de los sensores para enviar los datos
constexpr char TEMP_HUM_SENSOR_ID[] = "DHT11";
constexpr char SOUND_SENSOR_ID[] = "KY-038";

// Intervalos de tiempo
constexpr unsigned long TEMP_SEND_INTERVAL = 300000;  // Intervalo de envío de datos del sensor de temperatura y humedad (5 minutos)
constexpr unsigned long SOUND_SEND_INTERVAL = 300000;  // Intervalo de envío de datos del sensor de sonido (5 minutos)
constexpr unsigned long PRINT_INTERVAL = 1000;         // Intervalo de impresión de datos en el monitor serie (1 segundo)

// Variables para gestionar el tiempo
unsigned long lastTempSendMillis = 0;  // Último tiempo de envío de datos de temperatura
unsigned long lastSoundSendMillis = 0; // Último tiempo de envío de datos del sensor de sonido
unsigned long lastPrintMillis = 0;     // Último tiempo de impresión de datos en serie

// Instancia del sensor DHT
DHTesp dht;

// Prototipos de las funciones
void connectToWiFi();
void sendHttpPost(const String& data);
float analogToDecibels(int analogValue);
float getSmoothedAnalogReading();
void controlTrafficLight(int analogValue);

void setup() {
  Serial.begin(115200);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  Serial.println("Iniciando sensores...");
  connectToWiFi();
  dht.setup(DHT_PIN, DHTesp::DHT11); // Configurar el sensor DHT11
}

void loop() {
  // Leer el valor analógico suavizado del sensor de sonido
  int analogValue = getSmoothedAnalogReading();
  // Convertir el valor analógico a decibelios
  float dBValue = analogToDecibels(analogValue);
  // Leer la temperatura y la humedad desde el sensor DHT
  TempAndHumidity dhtData = dht.getTempAndHumidity();

  // Controlar el semáforo basado en el valor del sensor de sonido
  controlTrafficLight(analogValue);

  unsigned long currentMillis = millis(); // Obtener el tiempo actual en milisegundos

  // Mostrar datos en el monitor serie cada 1 segundo
  if (currentMillis - lastPrintMillis >= PRINT_INTERVAL) {
    lastPrintMillis = currentMillis; // Actualizar el tiempo de la última impresión
    // Imprimir la temperatura, humedad y decibelios en el monitor serie
    Serial.printf("Temperatura: %.2f°C | Humedad: %.2f%% | Decibelios: %.2f\n", dhtData.temperature, dhtData.humidity, dBValue);
  }

  // Enviar datos del sensor de temperatura y humedad cada 30 minutos
  if (currentMillis - lastTempSendMillis >= TEMP_SEND_INTERVAL) {
    lastTempSendMillis = currentMillis; // Actualizar el tiempo del último envío de temperatura

    // Crear string JSON con los datos del sensor de temperatura y humedad
    String jsonTemperatureHumidity = "{";
    jsonTemperatureHumidity += "\"sensor_id\": \"" + String(TEMP_HUM_SENSOR_ID) + "\", ";
    jsonTemperatureHumidity += "\"temperatura\": " + String(dhtData.temperature, 2) + ", ";
    jsonTemperatureHumidity += "\"humedad\": " + String(dhtData.humidity, 2);
    jsonTemperatureHumidity += "}";

    // Mostrar los datos JSON en el monitor serie
    Serial.println("Enviando datos JSON del sensor de temperatura y humedad: " + jsonTemperatureHumidity);
    sendHttpPost(jsonTemperatureHumidity);
  }

  // Enviar datos del sensor de sonido cada 5 minutos
  if (currentMillis - lastSoundSendMillis >= SOUND_SEND_INTERVAL) {
    lastSoundSendMillis = currentMillis; // Actualizar el tiempo del último envío de sonido

    // Crear JSON con los datos del sensor de sonido
    String jsonSound = "{";
    jsonSound += "\"sensor_id\": \"" + String(SOUND_SENSOR_ID) + "\", ";
    jsonSound += "\"decibelios\": " + String(dBValue, 2);
    jsonSound += "}";

    // Mostrar los datos JSON en el monitor serie
    Serial.println("Enviando datos JSON del sensor de sonido: " + jsonSound);
    sendHttpPost(jsonSound);
  }
}

// Función para convertir el valor analógico del sensor de sonido a decibelios
float analogToDecibels(int analogValue) {
  float voltage = (analogValue / 4095.0f) * 3.3f;  // Convertir valor analógico a voltaje
  if (voltage < 0.004f) voltage = 0.004f;  // Ajustar para evitar valores de voltaje muy bajos
  return 20.0f * log10(voltage / 0.004f);  // Calcular el valor en decibelios
}

// Función para obtener una lectura suavizada del sensor de sonido
float getSmoothedAnalogReading() {
  const int NUM_SAMPLES = 20; // Número de muestras a promediar
  int sum = 0;
  // Leer y promediar varias muestras del sensor de sonido
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sum += 4095 - analogRead(SOUND_ANALOG_PIN); // Leer valor analógico y restar a 4095
    delay(5); // Esperar un poco entre lecturas
  }
  return sum / float(NUM_SAMPLES); // Promediar las lecturas
}

// Función para controlar el semáforo basado en el valor del sensor de sonido
void controlTrafficLight(int analogValue) {
  // Determinar el estado del semáforo según el valor analógico del sensor de sonido
  int state = analogValue > HIGH_THRESHOLD ? 2 : 
              analogValue > MEDIUM_THRESHOLD ? 1 : 
              analogValue > LOW_THRESHOLD ? 0 : -1;

  // Activar el LED correspondiente del semáforo según el estado
  digitalWrite(RED_LED_PIN, state == 2);
  digitalWrite(YELLOW_LED_PIN, state == 1);
  digitalWrite(GREEN_LED_PIN, state == 0);
}

// Función para conectar el dispositivo a la red WiFi
void connectToWiFi() {
  Serial.print("Conectando a WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Intentar conectar durante 20 intentos
  for (int attempts = 0; WiFi.status() != WL_CONNECTED && attempts < 20; attempts++) {
    delay(500);
    Serial.print(".");
  }

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

  // Verificar si la conexión fue exitosa
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConexión exitosa");
    Serial.println("IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nError al conectar. Reiniciando...");
    ESP.restart(); // Reiniciar si no se pudo conectar
  }
}

// Función para enviar los datos al servidor
void sendHttpPost(const String& data) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, omitiendo envío.");
    return; // No enviar datos si no hay conexión WiFi
  }

  HTTPClient http;
  http.begin(SERVER_URL);  // Establecer la conexión HTTP con el servidor
  http.addHeader("Content-Type", "application/json");  // Especificar que se enviarán datos JSON

  // Enviar los datos al servidor
  int httpResponseCode = http.POST(data);
  if (httpResponseCode > 0) {
    Serial.println("Respuesta del servidor: " + http.getString());
  } else {
    Serial.printf("Error en la solicitud HTTP: %d\n", httpResponseCode);
  }

  http.end();
}
