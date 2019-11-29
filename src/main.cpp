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
//Solenoid
uint8_t solenoidPin = 2;
unsigned long solenoidTime = 0;
unsigned long registerTime = 0;


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
    if (!varSolenoid.isNull() && varSolenoid.as<bool>()) {
      solenoidTime = millis();
    }
    JsonVariant varRegistrar = doc["registrar"];
    if (!varRegistrar.isNull() && varRegistrar.as<bool>()) {
      registerTime = millis();
    }
  }
}

void solenoidTimmer(){
  //Timmer del solenoid 2 segundos  
  if(millis() - solenoidTime <= 2000 && millis() >= 2000){    
    digitalWrite(solenoidPin, HIGH);
  }else{
    digitalWrite(solenoidPin, LOW);
  }
}

int modeTimmer(){
  //Timmer del de registro son 5 segundos
  //Mode 1 = Leer, 2 = Registrar
  if(millis() - registerTime <= 5000 && millis() >= 5000){
    return 2;
  }else{
    return 1;
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
  uint8_t mode = modeTimmer();
  if(mode == 1){//Leer tarjetas
    Serial.println("Modo Leer");
  }else if (mode == 2){//Registrar Tarjetas
    Serial.println("Modo Registrar");
  }
  //Se ejecuta cada 2 segundos
  solenoidTimmer();
  //Delay de Debug
  delay(500);
}