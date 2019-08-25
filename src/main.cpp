/*                            *****************************************************************
 *  ***************************   CONTROL DE ACCESOS TARJETAS NFC / MQTT / NODERED / INFLUX   *********************************************************************
 *                            *****************************************************************
 *  
 *  Lea el archivo Readme_Accesos para entender el funcionamiento de este proyecto así cmo el material 
 *  necesario para su implementación.
 * 
 *  Desarrollado por Oscar Ruiz Muela gurues@2019.
 * 
 */



#include <Arduino.h>

#include <ESP8266WiFi.h>        // WIFI ESP8266
#include <AsyncMqttClient.h>    // MQTT Libreria asincrona basada en eventos
#include <SPI.h>                // Bus SPI
#include <Adafruit_PN532.h>     // LECTOR TRAJETAS NFC
#include <ArduinoOTA.h>         // Actualización por OTA
#include <ESP8266mDNS.h>        // Actualización por OTA
#include <WiFiUdp.h>            // Actualización por OTA
#include <Ticker.h>

// Pines SPI 
#define SCK   D0
#define MOSI  D6
#define SS    D7
#define MISO  D5
#define RQ    D2

Ticker open_manual;

//Objeto Lector tarjetas NFC
Adafruit_PN532 nfc(SCK, MISO, MOSI, SS);

// Descomentar para mostrar debug monitor serie
#define DEBUG_ACCESO

// Definición de pines y ctes
#define led         D4            // Led de control placa Wemos
#define cerradura   D1            // Salida control rele puerta
//#define nfcretries 0x5F           // Número máximo de intentos para leer una tarjeta nfc

// Definición de Variables
volatile bool NFC_Present = false;    // Tarjeta NFC presente (variable interrupción)
unsigned long msApertura = 500;      // 1,5 seg de apertura de la cerradura
int Estado_Cerradura = LOW;        // Tarjeta NFC presente (variable estado )
bool manual = false;

#define ARRAYSIZE 11 // Son 10 tarjetas y la posición 11 = "" para borrar.
String idPermitido[ARRAYSIZE]={"108-18-101-3","16-31-183-195","150-156-49-249","92-127-211-3"}; // Tarjetas habilitadas para acceder
int ARRAYUSE= 4; // Puntero usado del array idPermitido[0,1,2,3,4,5,6,7,8,9,10], la pos 10 para borrar

// Configuración WIFI
const char *ssid = "MOVISTAR_9E06";
const char *password = "1CD0FD833D86C2705DD2";

// Configuración MQTT
#define MQTT_HOST IPAddress(192, 168, 1, 99)
#define MQTT_PORT 1883
AsyncMqttClient mqttClient;
const char* topicAcceso = "casa/puerta/acceso";
const char* topicAlta = "casa/puerta/alta_NFC";
const char* topicBaja = "casa/puerta/baja_NFC";
const char* topicControl = "casa/puerta/control_NFC";

  
/////// DEFINICIÓN DE FUNCIONES   /////////////////////////////////////////////

// Apertura de la cerradura. Esta función será llamada cada vez que se haya concedido el acceso a una tarjeta.
void abrirPuerta(){   
  #ifdef DEBUG_ACCESO
    Serial.println("abrirPuerta_UID_Card ............");
  #endif
  digitalWrite (cerradura, HIGH);
  delay(msApertura);
  digitalWrite (cerradura, LOW);
}

void abrirPuertaManual(){   
  #ifdef DEBUG_ACCESO
    Serial.println("abrirPuerta_Manual ............");
  #endif
  digitalWrite (cerradura, Estado_Cerradura);
  if (Estado_Cerradura == LOW){
    open_manual.detach(); // se para el Ticker apertura manual
  }
  Estado_Cerradura = LOW;
}

// Conectando a WiFi network
void setup_wifi() {

    delay(10);
  #ifdef DEBUG_ACCESO
    Serial.println();
    Serial.print("Conectado a ");
    Serial.println(ssid);
  #endif
    IPAddress ip(192, 168, 1, 150);
    IPAddress gateway(192,168,1,1);
    IPAddress subnet (255,255,255,0);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    WiFi.config(ip,gateway,subnet);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      #ifdef DEBUG_ACCESO
        Serial.print(".");
      #endif
    }
    randomSeed(micros());
  #ifdef DEBUG_ACCESO
    Serial.println("");
    Serial.println("WiFi conectado");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("");
  #endif

}

// Funciones del Broker MQTT

//Función para conectarse al Broker MQTT
void connectToMqtt() {
  
  #ifdef DEBUG_ACCESO
    Serial.println("********** connectToMqtt ************");
    Serial.println("Conectando CONTROL-ACCESOS al Broker MQTT...");
  #endif

    mqttClient.connect();
  
}

// Evento producido cuando se conecta al Broker
void onMqttConnect(bool sessionPresent) {

  #ifdef DEBUG_ACCESO
    Serial.println("********** onMqttConnect ************");
    Serial.println("Connected to MQTT.");
    Serial.print("Session present: ");
    Serial.println(sessionPresent);
  #endif

    mqttClient.subscribe(topicAcceso, 0);
    mqttClient.subscribe(topicAlta, 2);
    mqttClient.subscribe(topicBaja, 2);
    mqttClient.subscribe(topicControl, 2);
      
  #ifdef DEBUG_ACCESO
    Serial.print("Suscrito a topic's: Acesso, Alta, Baja y Control");
  #endif

}

//Evento cuando se desconecta del Broker
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {

  #ifdef DEBUG_ACCESO
    Serial.println("********** onMqttDisconnect ************");
    Serial.println("Control-Acesso Desconectado del MQTT.....");
  #endif

    if (WiFi.isConnected()) {
      connectToMqtt(); 
    }
    
}

//Gestión de Mensajes Suscritos MQTT ***************************************************************************************************
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t length, size_t index, size_t total) {

  String myString=(char*)payload;
    
  #ifdef DEBUG_ACCESO
    Serial.println("********** onMqttMessage ************");
    Serial.print("Message llegado [");
    Serial.print(topic);
    Serial.print("]= ");
    for (size_t i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println("");
    Serial.println("length = " + String(length));
    Serial.print("  qos: ");  Serial.println(properties.qos);
    Serial.print("  dup: ");  Serial.println(properties.dup);
    Serial.print("  retain: ");  Serial.println(properties.retain);
  #endif

    // Si re recibe un topic de Control
    if ((String)topic==(String)topicControl){
      // Se reinicia Wemos
      if ((char)payload[0] == 'R') {
        ESP.restart();
      } 
      // Muestra uid de las tarjetas con acceso Puerto serie
      if ((char)payload[0] == 'I') {
        for (int X = 0; X < ARRAYSIZE; X++) {
          Serial.println(idPermitido[X]);
        }
        Serial.print("ARRAYSIZE= ");
        Serial.println(ARRAYSIZE);
        Serial.print("ARRAYUSE= ");
        Serial.println(ARRAYUSE);
      }
      // Se abre la puerta
      if ((char)payload[0] == '1') {
        //abrirPuerta();
        Estado_Cerradura = HIGH;
        open_manual.attach_ms(msApertura, abrirPuertaManual); // Activo callback open manual
      } 
    }
    
    // Si re recibe un topic de Alta tarjeta NFC
    if ((String)topic==(String)topicAlta){
      String alta_uid = myString.substring(0, length);
      bool incluir = true;
      //#ifdef DEBUG_ACCESO
        Serial.print("Alta Tarjeta NFC UID: ");
        Serial.println(alta_uid);
      //#endif
      if ((ARRAYSIZE-2)>ARRAYUSE){
        for(int i = 0; i <= ARRAYUSE; i++){
            Serial.println("idPermitido: " + idPermitido[i]);
            Serial.println("Tamaño alta_uid: " + String(alta_uid.length()));
            Serial.println("Tamaño idPermitido: " + String((idPermitido[i]).length()));
            if(alta_uid == idPermitido[i]){
              incluir = false;
              break;
            }
        }
        if (incluir){
            ARRAYUSE++;
            idPermitido[ARRAYUSE-1]=alta_uid;
          // #ifdef DEBUG_ACCESO
              Serial.print("Se ha dado de Alta Tarjeta NFC UID: ");
              Serial.println(alta_uid);
          // #endif
        }
      }
    } 

    // Si se recibe un topic de Baja tarjeta NFC
    if ((String)topic==(String)topicBaja){
      String baja_uid= myString.substring(0, length);
      //#ifdef DEBUG_ACCESO
        Serial.print("Baja Tarjeta NFC UID: ");
        Serial.println(baja_uid);
      //#endif
      for(int i = 0; i <= ARRAYUSE; i++){
        if(baja_uid == idPermitido[i]){
          for(int n = i; n < ARRAYUSE+1; n++){
            idPermitido[n]=idPermitido[n+1];
          }
          ARRAYUSE--;
          Serial.print("ARRAYSIZE= ");
          Serial.println(ARRAYSIZE);
          Serial.print("ARRAYUSE= ");
          Serial.println(ARRAYUSE);
          break;
        }
      }
    }

}

// Muestra el ID de la tarjeta de forma hexadecimal
String PrintHex(const byte * uid, const long uidLength){
  String r = "";
  r+= uid[0];
  for (uint8_t i=1; i < uidLength; i++){
      r += "-";
      r += uid[i]&0xff; 
  }
  return r;
}

// Gestiona la interrupción de tarjeta NFC presente
void ICACHE_RAM_ATTR IRQ_ISR(){

  NFC_Present = true;

}

void Card_access(){
  

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  
  Serial.begin(115200);

  //Configuración de tarjetas con acceso permitido 
  // guardar SPIFFS *********************************************************************************
  
  Card_access();

  //Configuración de Pines
  pinMode ( RQ, INPUT );
  pinMode ( led, OUTPUT );
  digitalWrite ( led, HIGH );
  pinMode ( cerradura, OUTPUT );
  digitalWrite ( cerradura, LOW );

  nfc.begin();                          // Inicio NFC Hardware
  digitalWrite ( led, LOW );
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    #ifdef DEBUG_ACCESO
      Serial.println("NO ENCONTRADA TARJETA PN53x");
    #endif
    while (1); // halt
  }
  nfc.SAMConfig(); // configura tarjeta para leer RFID tags
  digitalWrite ( led, HIGH );
#ifdef DEBUG_ACCESO
  Serial.println("");
  Serial.println("");
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  Serial.println("");
  Serial.println("LECTOR NFC OK");
#endif

  //Configuración WIFI
  setup_wifi();

  //Configuración MQTT
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  if (WiFi.isConnected()) {
    connectToMqtt();
  }
#ifdef DEBUG_ACCESO 
  Serial.println("Configurado [MQTT]");
#endif

  //Actualización código por OTA
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("Control_Acceso");
  //ArduinoOTA.setPassword("2047");
  ArduinoOTA.begin();

  attachInterrupt(digitalPinToInterrupt(RQ), IRQ_ISR, FALLING);
  
#ifdef DEBUG_ACCESO 
  Serial.println("");
  Serial.println("");
  Serial.println("----- Control de Acceso NFC OPERATIVO -----");
#endif 

}

void loop () {

  ArduinoOTA.handle(); // Actualización código por OTA
  
  //Variables de loop()
  String uid;
  uint8_t puerta;
  uint8_t N_uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                          // Length of the UID (4 (Mifare Classic) or 7 (Mifare Ultralight) 
                                              // bytes depending on ISO14443A card type)
  #ifdef DEBUG_ACCESO
    Serial.println("loop");
  #endif

  if(NFC_Present == true) { 
    noInterrupts(); // desactivo interrupciones
    digitalWrite ( led, LOW ); // Detectada tarjeta
    #ifdef DEBUG_ACCESO
      Serial.println("Detectada Tarjeta NFC ........");
    #endif
    puerta = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &N_uid[0], &uidLength);
    #ifdef DEBUG_ACCESO
      Serial.print("puerta= ");Serial.println(puerta);
    #endif
    if (puerta){
      #ifdef DEBUG_ACCESO
        Serial.print("Encontrada ISO14443A card UID: ");
        Serial.println(PrintHex(N_uid,uidLength));
      #endif
      // espera hasta que se quite la tarjeta
      while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &N_uid[0], &uidLength)) {}
      uid = PrintHex(N_uid,uidLength);
      //Comprueba si card UID tiene acceso
      bool noEncontrado = true;
      for(int i = 0; i < ARRAYUSE; i++){
        if(uid == idPermitido[i]){
          abrirPuerta();
          noEncontrado = false;
          break;
        }
      }
      #ifdef DEBUG_ACCESO
        if(noEncontrado){ // Tarjeta sin acceso
          Serial.print("Acceso Denegado a ISO14443A card UID: ");
          Serial.println(uid);
        }
        else{
          Serial.print("Acceso Concedido a ISO14443A card UID: ");
          Serial.println(uid);
        }
      #endif
      // MQTT se publica el acceso
      mqttClient.publish(topicAcceso, 0, true, ((uid).c_str())); // Publico uid CARD leida
    }
    digitalWrite ( led, HIGH ); // Fin presencia tarjeta 
    NFC_Present = false; // Reiniciovariable gestión interrupción
    interrupts(); // Activo interrupciones
  }
  
  nfc.enableRead(PN532_MIFARE_ISO14443A, N_uid, &uidLength);
    
}
