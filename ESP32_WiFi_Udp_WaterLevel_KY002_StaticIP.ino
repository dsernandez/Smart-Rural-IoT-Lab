/*
Enviar datos a través de UDP
Placa ESP32 Wifi
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// Datos de red WiFi
const char* ssid = "dragino-2187ac";        // Cambia por el SSID de tu red
const char* password = "PASSWORD-WiFi";   // No se especifica por seguridad

// Static IP configuration
IPAddress staticIP(10, 130, 1, 101); // ESP32 static IP
IPAddress gateway(10, 130, 1, 239);    // IP Address of your network gateway (router)
IPAddress subnet(255, 255, 255, 0);   // Subnet mask
IPAddress primaryDNS(8, 8, 8, 8); // Primary DNS (optional)
IPAddress secondaryDNS(0, 0, 0, 0);   // Secondary DNS (optional)



// Configuración del servidor UDP
const char* udpServerIP = "10.201.54.164";  // Dirección IP del servidor UDP
const int udpServerPort = 1700;            // Puerto del servidor UDP

WiFiUDP udpClient; // Objeto para enviar datos UDP


// Datos del sensor KY-002
int hitPin = 4;                             //Variable de entrada de datos a través del KY-002
int hitValor;                             //Variable entera para guardar el status del sensor KY-002

const char* sensorId = "KY-002";

// Intervalo de tiempo para el envío de datos (30 segundos)
const unsigned long interval = 30000;

// Inicializar variable para cálculo de intervalo
unsigned long previousMillis = 0;


// Datos del sensor Water Level
int waterSensorPin = 33;  // Pin donde está conectado el sensor de agua

const char *sensorId_WL = "WaterLevel";
float valor = 123.456; // Valor inicial de ejemplo

// Intervalo de tiempo para el envío de datos (5 segundos)
const unsigned long interval_WL = 5000;

// Inicializar variable para cálculo de intervalo
unsigned long previousMillis_WL = 0;



// Conexión WiFi
void initWifi()
{
    delay(10);
    WiFi.begin(ssid, password);
    // Esperar conexión WiFi
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Conectando a WiFi...");
    }
    /*
    Serial.println("Conectado a WiFi");
    Serial.print("Conectado con IP: ");
    Serial.println(WiFi.localIP());
    */
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

// Función para crear un string JSON para el sensor KY-002
String createJson(const char* id, float dato) {
  String json = "{";
  json += "\"sensor_id\": \"" + String(id) + "\", ";
  json += "\"vibracion\": " + String(dato, 2); // Redondear a 2 decimales
  json += "}";
  return json;
}

// Función para crear un string  con formato del sensor Water Level
String createJson_WL(const char *id, float dato1, float dato2)
{
  String json = "{";
  json += "\"sensor_id\": \"" + String(id) + "\", ";
  json += "\"nivel_agua\": " + String(dato1, 2) + ", "; // Redondear a 2 decimales
  json += "\"porcentaje_agua\": " + String(dato2, 2); // Redondear a 2 decimales
  json += "}";
    return json;
}

// Función para enviar datos por UDP
void sendUdpData(const String &jsonData)
{
    udpClient.beginPacket(udpServerIP, udpServerPort); // Inicia un paquete hacia el servidor UDP
    udpClient.print(jsonData);                         // Agrega el contenido del JSON al paquete
    if (udpClient.endPacket())
    { // Envía el paquete y verifica si fue exitoso
        Serial.println("Datos enviados por UDP: " + jsonData);
    }
    else
    {
        Serial.println("Error al enviar los datos por UDP.");
    }
}

float calcular_porcentaje_WL(int sensor_value) {
  // Valores del sensor
  const float v_min = 0;// Valor devuelto por el sensor con el tanque vacío (medido previamente)
  const float v_mitad = 700;// Valor devuelto por el sensor con el tanque al 50% (medido previamente)
  const float v_max = 900;// Valor devuelto por el sensor con el tanque lleno (medido previamente)
  float porcent = 0;

  if(sensor_value <= v_min){
    porcent = 0;// Tanque vacío
  }
  else if(sensor_value <= v_mitad){
    // Segmento 0% - 50%
    porcent = (sensor_value / v_mitad) * 50;
  }
  else if(sensor_value <= v_max){
    // Segmento 50% - 100%
    porcent = ((sensor_value - v_mitad) / (v_max - v_mitad)) * 50 + 50;
  }
  else {
    porcent = 100;// Tanque lleno
  }
  
  return porcent;
}

void setup() {
  pinMode(hitPin,INPUT);                 //Se configura el KY-002 como entrada
  Serial.begin(115200);
   // Conectar a WiFi
  initWifi();

  // Configurar el cliente UDP
  udpClient.begin(WiFi.localIP(), udpServerPort); // Inicia el cliente UDP en el puerto local
  Serial.println("Cliente UDP inicializado");
}

void loop() {

  //Water Level Sensor
  unsigned long currentMillis_WL = millis();

  // Verificar si ha pasado el intervalo configurado para el sensor Water Level
  if (currentMillis_WL - previousMillis_WL >= interval_WL)
  {
    previousMillis_WL = currentMillis_WL;
    /*
    // Generar datos simulados
    valor = random(10, 200) / 10.0; // Valor de ejemplo entre 1.0 y 20.0
    */
    //Leer datos sensor y calcular porcentaje
    int valorSensor = analogRead(waterSensorPin);
    float porcentajeAgua = calcular_porcentaje_WL(valorSensor);
    Serial.println("*************************");
    Serial.print("Valor Analógico: ");
    Serial.print(valorSensor);
    Serial.print(" - Nivel de Agua: ");
    Serial.print(porcentajeAgua);
    Serial.println("%");
    Serial.println("*************************");

    // Crear JSON y enviarlo por UDP
    String jsonData = createJson_WL(sensorId_WL, valorSensor, porcentajeAgua);

    Serial.println("Enviando datos WaterLevel JSON por UDP:");
    Serial.println(jsonData);

    sendUdpData(jsonData);
  }

  //KY-002 Sensor Vibración
  hitValor = digitalRead(hitPin);               // Se lee el valor digital del ky-002 y se asigna a valor
  /*
  if(hitValor == LOW){                     // Como es su estado en reposo es NA su valor inicial es 0(LOW) 
    Serial.print("Inactivo: ");         // Se imprime un texto en el monitor serial, Inactivo...
    Serial.println (hitValor);          // ... y con valor 0
  }
  else {                              // Sí el valor es igual a 1(HIGH) se ejecuta la instrucción  
   Serial.print("Golpe Avisa: ");       // Se imprime Golpe Avisa
   Serial.println (hitValor);           // ... y con valor 1
   // Crear JSON y enviarlo

    // Crear JSON y enviarlo por UDP
    String jsonData = createJson(sensorId, hitValor);
    
    Serial.println("Enviando datos JSON por UDP:");
    Serial.println(jsonData);
    
    sendUdpData(jsonData);
    delay(5000);  // 5 segundos hasta proxima lectura
  }
  */
  if(hitValor != LOW){                     // Como es su estado en reposo es NA su valor inicial es 0(LOW) 
                                        // Sí el valor es igual a 1(HIGH) se ejecuta la instrucción  
   Serial.print("Golpe Avisa: ");       // Se imprime Golpe Avisa
   Serial.println (hitValor);           // ... y con valor 1
   // Crear JSON y enviarlo

    // Crear JSON y enviarlo por UDP
    String jsonData = createJson(sensorId, hitValor);
    
    Serial.println("Enviando datos JSON por UDP:");
    Serial.println(jsonData);
    
    sendUdpData(jsonData);
    delay(5000);  // 5 segundos hasta proxima lectura
  }
}