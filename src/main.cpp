#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <MFRC522.h>
#ifdef ESP32
  #include "time.h"
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <SPIFFS.h>
  #include <ESPmDNS.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include "FS.h"
  #define FILE_APPEND "a"
  #define FILE_READ "r"
  #define FILE_FILE_WRITE "w"
  #include <ESP8266mDNS.h>
#endif
const char *ssid = "TURBONETT_1DFD27";
const char *password = "57E04D255E";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
//Solenoid
uint8_t solenoidPin = 4;
unsigned long solenoidTime = 0;
unsigned long registerTime = 0;
//RFID Module
#define SS_PIN 21
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);
byte nuidPICC[4];
//System
#define csvFile "/data.csv"
#define csvLogs "/logs.csv"
#define ledPin 2
#define btnReg 0
//NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -3600*6;
const int   daylightOffset_sec = 3600;


void sendAllJson(const String& key, const String& value){
  // Crear salida en formato JSON
  String text = "";//respuesta json del websocket
  StaticJsonDocument<256> doc;
  JsonObject root = doc.to<JsonObject>();
  root[key] = value;
  serializeJson(doc, text);
  //Enviar por el WebSocket
  ws.textAll(text.c_str());
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

String today(){
#ifdef ESP32
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "-";
  }
  char timeStringBuff[50]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "%d/%m/%Y %H:%M:%S", &timeinfo);
  String asString(timeStringBuff);
  return asString;
#elif defined(ESP8266)
  return "No disponible";
#endif
}

boolean saveLog(const String& today, const String& hexCode, const String& message){
  //Si no exites el archivo, Crear el csv
  if(!SPIFFS.exists(csvLogs)){
    Serial.println(F("Crear archivo CSV"));
    File file = SPIFFS.open(csvLogs, FILE_APPEND);
    const char *headersCSV = "fecha,hex,message";
    if(!file.print(headersCSV)){
      Serial.println(F("No se ha podido escribir"));
      file.close();
      return false;
    }
    file.close();
  }
  //Guardar el registro
  String text;
  text.concat('\n');text.concat(today);text.concat(',');text.concat(hexCode);text.concat(',');text.concat(message);
  File file = SPIFFS.open(csvLogs, FILE_APPEND);
  if(!file.print(text)){
    Serial.println(F("No se ha podido escribir en logs"));
    return false;
  }
  file.close();
  sendAllJson("update", "true");
  return true;
}

boolean saveCard(const String& usuario, const String& hexCode){
  //Si no exites el archivo, Crear los headers del csv
  if(!SPIFFS.exists(csvFile)){
    Serial.println(F("Crear archivo CSV"));
    File file = SPIFFS.open(csvFile, FILE_APPEND);
    const char *headersCSV = "usuario,hex";
    if(!file.print(headersCSV)){
      Serial.println(F("No se ha podido escribir los headers"));
      file.close();
      return false;
    }
    file.close();
  }
  //Guardar el usuario con su tarjeta
  String text;
  text.concat('\n');text.concat(usuario);text.concat(',');text.concat(hexCode);
  File file = SPIFFS.open(csvFile, FILE_APPEND);
  if(!file.print(text)){
    Serial.println(F("No se ha podido escribir la tarjeta"));
    return false;
  }
  file.close();
  return true;
}

boolean cardExits(const String &hexCode){
  File file = SPIFFS.open(csvFile, FILE_READ);
  String hexText = "";
  boolean isHex = false;
  boolean isHeader = true;
  while (file.available()) {
    char c = file.read();
    if(c == ','){
      isHex = true;
    }else if(c == '\n'){
      isHeader = false;
      isHex = false;
      if(hexCode == hexText){
        file.close();
        return true;
      }
      hexText = "";
    }
    if(isHex && c != ',' && isHeader == false){
      hexText.concat(c);
    }
  }
  if(hexCode == hexText){
    file.close();
    return true;
  }
  file.close();
  return false;
}

boolean isCard(){
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return false;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return false;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && piccType != MFRC522::PICC_TYPE_MIFARE_1K && piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    //return false;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] || rfid.uid.uidByte[1] != nuidPICC[1] || rfid.uid.uidByte[2] != nuidPICC[2] || rfid.uid.uidByte[3] != nuidPICC[3]) {
    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }
    return true;
  }else{
    Serial.println(F("Card read previously."));
  }
  // Halt PICC
  rfid.PICC_HaltA();
  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
  return false;
}

String cardToHexString(){
  String hexString = "";
  for (byte i = 0; i < 4; i++) {
    hexString.concat(String(nuidPICC[i], HEX));
    if(i < 3){
      hexString.concat(':');
    }
    nuidPICC[i] = 0;
  }
  return hexString;
}

void manualRegister(){
  //Registro Manual
  if(digitalRead(btnReg) == 0){
    digitalWrite(ledPin, HIGH);
    while(digitalRead(btnReg) == 0 && !isCard()){
      delay(100);
    }
    String hexToString = cardToHexString();
    if(hexToString != "0:0:0:0"){
      digitalWrite(ledPin, LOW);
      saveCard("invitado", hexToString);
      saveLog(today(), "hardware", F("Se ha registrado invitado manualmente"));
      delay(100);
      digitalWrite(ledPin, HIGH);
    }
    while(digitalRead(btnReg) == 0){
      delay(100);
    }
    digitalWrite(ledPin, LOW);
  }
}

void resetAllSystem(){
  SPIFFS.remove(csvLogs);
  SPIFFS.remove(csvFile);
  ESP.restart();
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  String text = "";
  StaticJsonDocument<256> doc;
  if(type == WS_EVT_CONNECT){
    Serial.println(F("Client connected"));
  } else if(type == WS_EVT_DISCONNECT){
    Serial.println(F("Client disconnected"));
  } else if(type == WS_EVT_DATA){
    Serial.println(F("Client Recive Data"));
    deserializeJson(doc, data);
    JsonVariant varSolenoid = doc["solenoid"];
    if (!varSolenoid.isNull() && varSolenoid.as<bool>()) {
      saveLog(today(), client->remoteIP().toString().c_str(), F("Se ha accionado la cerradura desde la web"));
      solenoidTime = millis();
    }
    JsonVariant varRegistrar = doc["registrar"];
    if (!varRegistrar.isNull() && varRegistrar.as<bool>()) {
      registerTime = millis();
    }
    JsonVariant varUsuario = doc["usuario"];
    JsonVariant varHex = doc["hex"];
    if (!varUsuario.isNull() && !varHex.isNull()) {
      saveCard(varUsuario.as<String>(), varHex.as<String>());
      saveLog(today(), varHex.as<String>(), F("Nueva tarjeta registrada"));
      sendAllJson("message", "success-Se ha registrada tarjeta");
    }
    JsonVariant varResetAll = doc["resetAll"];
    if (!varResetAll.isNull() && varResetAll.as<bool>()) {
      resetAllSystem();
    }
  }
}

void setup() {
  //Inicando el pin del Solenoid apagado
  pinMode(solenoidPin, OUTPUT);digitalWrite(solenoidPin, LOW);
  //Debug Hardware
  pinMode(btnReg, INPUT);
  pinMode(ledPin, OUTPUT);digitalWrite(ledPin, LOW);
  //SPI bus
  SPI.begin();
  //Modulo MFRC522
  rfid.PCD_Init();
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
  //DNS para servicio de hostname
  if (!MDNS.begin("sistema-rfid")) {
    Serial.println(F("Error setting up MDNS responder!"));
  }
  MDNS.addService("http", "tcp", 80);
  //Iniciando la memoria SPIFFS
  SPIFFS.begin();
  //Servidor de Archivos de la memoria SPIFFS
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  //Agregando los eventos del WebSocket al Servidor
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  //Iniciar Servidor
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.begin();
  //NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  uint8_t mode = modeTimmer();
  if(mode == 1){//Leer tarjetas
    Serial.println(F("Modo Leer"));
    if(isCard()){
      String hexToString = cardToHexString();
      Serial.println(hexToString);
      if(cardExits(hexToString)){
        saveLog(today(), hexToString, F("Tarjeta ha accedido"));
        solenoidTime = millis();
      }else{
        saveLog(today(), hexToString, F("Tarjeta no Registrada"));
        Serial.println(F("Tarjeta no Registrada"));
      }
    }
  }else if (mode == 2){//Registrar Tarjetas
    Serial.println(F("Modo Registrar"));
    if(isCard()){
      String hexToString = cardToHexString();
      if(cardExits(hexToString)){
        sendAllJson("message", "error-Tarjeta ya ha sido registrada");
      }else{
        sendAllJson("hex", hexToString);
      }
    }
  }
  //Se ejecuta cada 2 segundos
  solenoidTimmer();
  //Delay de Debug
  delay(500);

  manualRegister();

}