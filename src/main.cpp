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
#include <Ticker.h>             // Temporizador funciones asincronas

#include <credenciales.h>      // Credencales WIFI y MQTT BROKER
#include <configuracion.h>     // Configuración del proyecto

//Objeto Ticker para la apertura manual
Ticker open_manual;

//Objeto Lector tarjetas NFC
Adafruit_PN532 nfc(SCK, MISO, MOSI, SS);

// Objeto Broker MQTT Asincrono
AsyncMqttClient mqttClient;

// Definición de Variables
volatile bool NFC_Present = false;    // Tarjeta NFC presente (variable interrupción)
bool Estado_Cerradura = false;        // Variable global que representa el estado de la cerradura
  
/////// DEFINICIÓN DE FUNCIONES   /////////////////////////////////////////////
/*
// Apertura de la cerradura. Esta función será llamada cada vez que se haya concedido el acceso a una tarjeta.
void abrirPuerta(){   
  #ifdef DEBUG_ACCESO
    Serial.println("abrirPuerta_UID_Card ............");
  #endif
  digitalWrite (led, HIGH);
  digitalWrite (cerradura, HIGH);
  delay(msApertura);
  digitalWrite (led, LOW);
  digitalWrite (cerradura, LOW);
}
*/
// Apertura de la cerradura. Esta función será llamada cada vez que se abra la puerta de forma manual
// a través del nodered
void abrirPuertaManual(){   
  #ifdef DEBUG_ACCESO
    Serial.println("abrirPuerta_Manual ............");
  #endif
  digitalWrite (cerradura, Estado_Cerradura);
  if (Estado_Cerradura == LOW){
    open_manual.detach(); // se para el Ticker apertura manual
    mqttClient.publish(topicAcceso, 0, false, "APERTURA MANUAL"); // Publico uid CARD leida
  }
  Estado_Cerradura = LOW;
}

// Conectando a la WiFi network
void setup_wifi() {

    delay(10);
  #ifdef DEBUG_ACCESO
    Serial.println();
    Serial.print("Conectado a ");
    Serial.println(ssid);
  #endif
    
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

    // Subscrito a los topic Acceso, Alta, Baja y Control
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

//Gestión de la llegada de Mensajes Subscritos MQTT ********************************************************************************************
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

    // Si se recibe un topic de Control
    if ((String)topic==(String)topicControl){
      // Se reinicia Wemos
      if ((char)payload[0] == 'R') {
        ESP.restart();
      } 
      // Muestra uid de las tarjetas con acceso Puerto serie
      if ((char)payload[0] == 'I') {
        for (int X = 0; X < ARRAYUSE; X++) {
          #ifdef DEBUG_ACCESO
            Serial.println(idPermitido[X]);
          #endif
          mqttClient.publish(topicTarjetas, 0, false, (idPermitido[X]).c_str()); // Publico tarjetas con permiso
        }
        #ifdef DEBUG_ACCESO
          Serial.print("ARRAYSIZE= ");
          Serial.println(ARRAYSIZE);
          Serial.print("ARRAYUSE= ");
          Serial.println(ARRAYUSE);
        #endif
      }
      // Se abre la puerta de forma manual desde nodered
      if ((char)payload[0] == '1') {
        Estado_Cerradura = HIGH;
        open_manual.attach_ms(msApertura, abrirPuertaManual); // Activo callback open manual
      } 
    }
    
    // Si re recibe un topic de Alta tarjeta NFC
    if ((String)topic==(String)topicAlta){
      String alta_uid = myString.substring(0, length);
      bool incluir = true;
      #ifdef DEBUG_ACCESO
        Serial.print("Alta Tarjeta NFC UID: ");
        Serial.println(alta_uid);
      #endif
      if ((ARRAYSIZE-2)>ARRAYUSE){
        for(int i = 0; i <= ARRAYUSE; i++){
          #ifdef DEBUG_ACCESO
            Serial.println("idPermitido: " + idPermitido[i]);
            Serial.println("Tamaño alta_uid: " + String(alta_uid.length()));
            Serial.println("Tamaño idPermitido: " + String((idPermitido[i]).length()));
          #endif
          if(alta_uid == idPermitido[i]){
            incluir = false;
            break;
          }
        }
        if (incluir){
          ARRAYUSE++;
          idPermitido[ARRAYUSE-1]=alta_uid;
          #ifdef DEBUG_ACCESO
            Serial.print("Se ha dado de Alta Tarjeta NFC UID: ");
            Serial.println(alta_uid);
          #endif
        }
      }
    } 

    // Si se recibe un topic de Baja tarjeta NFC
    if ((String)topic==(String)topicBaja){
      String baja_uid= myString.substring(0, length);
      #ifdef DEBUG_ACCESO
        Serial.print("Baja Tarjeta NFC UID: ");
        Serial.println(baja_uid);
      #endif
      for(int i = 0; i <= ARRAYUSE; i++){
        if(baja_uid == idPermitido[i]){
          for(int n = i; n < ARRAYUSE+1; n++){
            idPermitido[n]=idPermitido[n+1];
          }
          ARRAYUSE--;
          #ifdef DEBUG_ACCESO
            Serial.print("ARRAYSIZE= ");
            Serial.println(ARRAYSIZE);
            Serial.print("ARRAYUSE= ");
            Serial.println(ARRAYUSE);
          #endif
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {

  #ifdef DEBUG_ACCESO
    Serial.println("----------- setup() -----------------");  
    Serial.begin(115200);
  #endif

  //Configuración de Pines
  pinMode ( RQ, INPUT );
  pinMode ( led, OUTPUT );
  digitalWrite ( led, HIGH );
  pinMode ( cerradura, OUTPUT );
  digitalWrite ( cerradura, LOW );

  nfc.begin();  // Inicio NFC Hardware
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

  // Inicializo OTA
  ArduinoOTA.setHostname("Accesos_NFC"); // Hostname OTA
  ArduinoOTA.begin();

  //Activo interrupciones lector NFC
  nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
  delay(500);
  attachInterrupt(digitalPinToInterrupt(RQ), IRQ_ISR, FALLING);
  
#ifdef DEBUG_ACCESO 
  Serial.println("");
  Serial.println("");
  Serial.println("----- Control de Acceso NFC OPERATIVO -----");
#endif 

}

void loop () {

  #ifdef DEBUG_ACCESO
   // Serial.println("----------- loop() -----------------");
  #endif
 
  ArduinoOTA.handle(); // Actualización código por OTA

  //Variables de loop()
  String uid;
  uint8_t puerta;
  uint8_t N_uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                          // Length of the UID (4 (Mifare Classic) or 7 (Mifare Ultralight) 
                                              // bytes depending on ISO14443A card type)
  
  // Evaluación de la tarjeta leida
  if(NFC_Present == true) { 
    noInterrupts(); // desactivo interrupciones
    puerta = nfc.readDetectedPassiveTargetID(&N_uid[0], &uidLength);
    if (puerta){
      digitalWrite ( led, LOW ); // Detectada tarjeta
      time_Nfc = millis();
      #ifdef DEBUG_ACCESO
        Serial.print("Encontrada ISO14443A card UID: ");
        Serial.println(PrintHex(N_uid,uidLength));
        bool noEncontrado = true;
      #endif
      uid = PrintHex(N_uid,uidLength);
      //Comprueba si card UID tiene acceso
      for(int i = 0; i < ARRAYUSE; i++){
        if(uid == idPermitido[i]){
          digitalWrite (cerradura, HIGH); // Abrimos puerta, tarjeta con acceso
          Estado_Cerradura = true; 
          #ifdef DEBUG_ACCESO
            Serial.println("Bobina Puerta ON ............");
            noEncontrado = false;
          #endif
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
      // MQTT se publica la UID de la tarjeta leida
      mqttClient.publish(topicAcceso, 0, false, ((uid).c_str())); // Publico uid CARD leida
    }
    else {
      // Tiempo con Bobina Apertura Puerta ON
      if ((abs(millis() - time_Nfc) > msApertura) && (Estado_Cerradura)){ 
        #ifdef DEBUG_ACCESO
          Serial.println("Bobina Puerta OFF ............");
        #endif
        Estado_Cerradura = false; 
        digitalWrite (cerradura, LOW);
      }
      //Activación de interrupciones y permisos para nueva lectura de Tarjeta
      if ((abs(millis() - time_Nfc) > timeEntreCards)&&(NFC_Present)){
        time_Nfc = millis();
        digitalWrite ( led, HIGH ); // Fin lectura tarjeta 
        Estado_Cerradura = false; 
        digitalWrite (cerradura, LOW);
        NFC_Present = false;
        nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A); // Activación lectura tarjeta
        interrupts(); // Activo interrupciones
      }
    }
  }

}
