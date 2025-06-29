/*
    Enviar datos a traves de CoAP (UDP)
    Placa ESP32 Wifi
    */
 
    #include <WiFi.h>
    #include <WiFiUdp.h>
    #include <coap-simple.h>

    
   float duration_us, distance_cm;
    // Datos de red WiFi
    const char* ssid = "dragino-2187ac";        // Cambia por el SSID de tu red
    const char* password = "PASSWORD-WiFi";   // No se especifica por seguridad
 
   // Datos del sensor

  #define TRIG_PIN 23 // ESP32 pin GPIO23 connected to Ultrasonic Sensor's TRIG pin
  #define ECHO_PIN 22 // ESP32 pin GPIO22 connected to Ultrasonic Sensor's ECHO pin
    
    // Dirección del servidor CoAP
    const char* coapServer = "10.201.54.164";
    const uint16_t coapPort = 5683; // Puerto estándar de CoAP

    // Static IP configuration
    IPAddress staticIP(10, 130, 1, 108); // ESP32 static IP
    IPAddress gateway(10, 130, 1, 239);    // IP Address of your network gateway (router)
    IPAddress subnet(255, 255, 255, 0);   // Subnet mask
    IPAddress primaryDNS(8, 8, 8, 8); // Primary DNS (optional)
    IPAddress secondaryDNS(0, 0, 0, 0);   // Secondary DNS (optional)
 
    WiFiUDP udp;                // Cliente UDP para usar con CoAP
    Coap coapClient(udp);       // Cliente CoAP basado en UDP
 
    // Configuración del recurso CoAP y datos
    const char* resource = "iot-lab/sensors";
    const char* sensorId = "HC-SR04";
    // float valor = 123.456; // Valor inicial de ejemplo

 
    // Intervalo de tiempo para el envío de datos (30 segundos)
    const unsigned long interval = 30000;
    unsigned long previousMillis = 0;
 
    // Conexión WiFi
    void initWifi() {
        delay(10);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            Serial.println("Conectando a WiFi...");
        }
        Serial.println("Conectado a WiFi");
        Serial.print("Conectado con IP: ");
        Serial.println(WiFi.localIP());

    // Configuring static IP
    if(!WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS)) {
      Serial.println("Failed to configure Static IP");
    } else {
      Serial.println("Static IP configured!");
    }
    }
 
    // Crear string JSON
    String createJson(const char* id, float dato) {
        String json = "{";
        json += "\"sensor_id\": \"" + String(id) + "\", ";
        json += "\"distancia\": " + String(dato, 2); // Redondear a 2 decimales
        json += "}";
        return json;
    }
 
    // CoAP client response callback
    void callback_response(CoapPacket &packet, IPAddress ip, int port);
 
 
    // CoAP client response callback
    void callback_response(CoapPacket &packet, IPAddress ip, int port) {
        Serial.println("[Coap Response got]");
 
        char p[packet.payloadlen + 1];
        memcpy(p, packet.payload, packet.payloadlen);
        p[packet.payloadlen] = NULL;
 
        Serial.println(p);
    }
 
    void setup() {
        Serial.begin(115200);
    // configure the trigger pin to output mode
    pinMode(TRIG_PIN, OUTPUT);
    // configure the echo pin to input mode
    pinMode(ECHO_PIN, INPUT);
        // Conectar a WiFi
        initWifi();
 
        // Inicializar CoAP y registrar callback
        Serial.println("Setup Response Callback");
        coapClient.response(callback_response);
 
        coapClient.start();
    }
 
    void loop() {
        unsigned long currentMillis = millis();
        
    // generate 10-microsecond pulse to TRIG pin
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    duration_us = pulseIn(ECHO_PIN, HIGH);

  // calculate the distance
  distance_cm = 0.017 * duration_us;
 
  // Verificar si ha pasado el intervalo configurado
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
 
     // Generar datos simulados
     //valor = random(10, 200) / 10.0; // Valor de ejemplo entre 1.0 y 20.0
 
     // Crear JSON y enviarlo
     String jsonData = createJson(sensorId, distance_cm);
 
     int msgId = coapClient.put(coapServer, coapPort, resource, jsonData.c_str());
     Serial.print("msgId: ");
     Serial.println(msgId);
    }
      coapClient.loop(); // Mantener las comunicaciones CoAP
}