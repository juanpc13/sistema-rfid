#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <SPIFFS.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include "FS.h"
#endif

const char *ssid = "TURBONETT_1DFD27";
const char *password = "57E04D255E";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

boolean solenoid = false;
boolean solenoidPin = 4;
unsigned long solenoidTime = 0;

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){  
  String text = "";
  StaticJsonDocument<256> doc;
  if(type == WS_EVT_CONNECT){
    Serial.println("Client connected");
  } else if(type == WS_EVT_DISCONNECT){
    Serial.println("Client disconnected");
  } else if(type == WS_EVT_DATA){
    Serial.println("Client Recive Data");
    deserializeJson(doc, data);
    JsonVariant varSolenoid = doc["solenoid"];
    if (!varSolenoid.isNull()) {
      solenoidTime = millis() + 1000*2;
    }
  }
}

void setup() {
  //Inicando el pin del Solenoid apagado
  pinMode(solenoidPin, OUTPUT);digitalWrite(solenoidPin, LOW);
  //Puerto Serial
  Serial.begin(115200);Serial.println();
  //Iniciando punto de acceso
  //WiFi.softAP(ssid, password);
  //Conectar Wifi
  WiFi.mode(WIFI_STA);WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");delay(100);
  }
  //Mostrar IP
  Serial.print("IP : ");Serial.println(WiFi.localIP());
  //Iniciando la memoria SPIFFS  
  SPIFFS.begin();
  //Servidor de Archivos de la memoria SPIFFS
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  //Agregando los eventos del WebSocket al Servidor
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  //Iniciar Servidor
  server.begin();  
}

void loop() {
  //Atender al Solenoid
  digitalWrite(solenoidPin, solenoid);
  solenoid = (millis() - solenoidTime >= 0);
  delay(250);  
}