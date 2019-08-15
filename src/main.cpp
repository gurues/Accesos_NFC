/*                            *****************************************************************
 *  ***************************   CONTROL DE ACCESOS TARJETAS NFC / MQTT / NODERED / INFLUX   *********************************************************************
 *                            *****************************************************************
 *  Mediante el lector de tarjetas PN532 conectado a través de bus I2C con resistencias pull-up de 2 Kohm a una placa WEMOS D1 MINI se 
 *  realiza el control de accesos de una puerta. El sistema mediante MQTT se conecta a NODERED que gestiona los accesos de las tarjetas
 *  y la activación del relé de salida. NODERED también gestiona el almacenamiento de los acceso en la base de datos INFLUXDB. Tanto el
 *  servidor MQTT (Mosquitto), NODEREED como INFLUXDB están instalados en una RASPBERRYPI 3 dentro la misma red local que el  hardware 
 *  del Control de Accesos.
 *  
 *  La alimentación del hardware del Control de Accesos se realiza a través de un puente de diodos y un regulador de voltaje para 
 *  rectificar y regular los 12 VAC que llegan al portero automático. La instalación del hardware del Control de Accesos se instalará
 *  en el interior del portero automático.
 *  
 *  Hardware necesario:
 *  
 *  - 2 Resistencia de 2 Kohmios.
 *  - 1 Wemos D1 Mini.
 *  https://es.aliexpress.com/item/ESP8266-NodeMcu-Lua-WIFI-Junta-D1-versi-n-mini/32879989375.html?spm=a2g0s.9042311.0.0.274263c0maHTwS
 *  - 1 Lector tarjetas PN532 NFC V3.
 *  https://es.aliexpress.com/item/1-Unidades-PN532-NFC-RFID-m-dulo-inal-mbrico-V3-registrado-Bluetooth-lector-y-escritor-de/32863757340.html?spm=a2g0s.9042311.0.0.274263c0X0mGsc
 *  - 1 Puente de diodos.
 *  https://es.aliexpress.com/item/Free-shipping-20PCS-2w10-2A-1000V-diode-bridge-rectifier-2W10/32828648841.html?spm=a2g0s.9042311.0.0.12a563c07wHrsD
 *  - 1 Regulador de Voltaje 7805. 
 *  https://es.aliexpress.com/item/10pcs-LM7805-L7805-7805-Voltage-Regulator-IC-5V-1-5A-TO-220-make-in-china/32796581038.html?spm=a2g0s.9042311.0.0.12a563c07wHrsD
 *  - 1 Relé Shield para Wemos.
 *  https://es.aliexpress.com/item/1PCS-NEW-Relay-Shield-for-Arduino-WeMos-D1-Mini-ESP8266-Development-Board-WeMos-D1-Relay-Module/32737849680.html?spm=a2g0s.13010208.99999999.260.2d063c00OeQ5s9
 *  - 1 Placa Shield Wemos (Triple escudo).
 *  https://es.aliexpress.com/item/Triple-escudo-para-WeMos-D1-Mini-Dua-de-perforaci-n-para-Arduino-Compatible/32842137583.html?spm=a2g0s.9042311.0.0.274263c0maHTwS
 *  - 10 Llaveros NFC's.
 *  https://es.aliexpress.com/item/10-piezas-de-la-etiqueta-RFID-de-13-56-MHz-Mif1-S50-llaveros-etiqueta-NFC-RFID/32960150464.html?spm=a2g0s.13010208.99999999.260.78623c00iNnDJ2 
 *  
 *  Coste total estimado del proyecto: entre 12 y 15 euros.
 *  
 *  La actualización del software en el WEMOS DI MINI se puede realizar mediante OTA
 * 
 *  Desarrollado por Oscar Ruiz Muela gurues@2019.
 * 
 */



#include <Arduino.h>
#include <ESP8266WiFi.h>        // WIFI ESP8266
#include <PubSubClient.h>       // MQTT
#include <SPI.h>                // Bus SPI
#include <Adafruit_PN532.h>     // LECTOR TRAJETAS NFC
#include <ArduinoOTA.h>         // Actualización por OTA
#include <ESP8266mDNS.h>        // Actualización por OTA
#include <WiFiUdp.h>            // Actualización por OTA

// Pines SPI 
#define SCK   D5
#define MOSI  D7
#define SS    D3
#define MISO  D6
#define RQ    D2

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
const int msApertura = 1500;         // 1,5 seg de apertura de la cerradura

#define ARRAYSIZE 11 // Son 10 tarjetas y la posición 11 = "" para borrar.
String idPermitido[ARRAYSIZE]={"16-31-183-195","15-50-233-67"}; // Tarjetas habilitadas para acceder "16-31-183-195","15-50-233-67"
int ARRAYUSE= 1; // Puntero usado del array idPermitido[0,1,2,3,4,5,6,7,8,9,10], la pos 10 para borrar

// Configuración WIFI
const char *ssid = "MOVISTAR_9E06";
const char *password = "1CD0FD833D86C2705DD2";
WiFiClient  clienteWifi;

// Configuración MQTT
PubSubClient clientMqtt(clienteWifi);
const char* servidorMqtt = "192.168.1.100";
const char* topicAcceso = "/casa/puerta/acceso";
const char* topicAlta = "/casa/puerta/alta_NFC";
const char* topicBaja = "/casa/puerta/baja_NFC";
const char* topicControl = "/casa/puerta/control_NFC";


/////// DEFINICIÓN DE FUNCIONES   /////////////////////////////////////////////

// Conectando a WiFi network
void setup_wifi() {

    delay(10);
#ifdef DEBUG_ACCESO
    Serial.println();
    Serial.print("Conectado a ");
    Serial.println(ssid);
#endif
    IPAddress ip(192, 168, 1, 50);
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

// Apertura de la cerradura. Esta función será llamada cada vez que se haya concedido el acceso a una tarjeta.
void abrirPuerta(){   
    digitalWrite ( cerradura, HIGH );
    delay(msApertura);
    digitalWrite ( cerradura, LOW );
}

//Gestión de Mensajes Suscritos MQTT
void callback(char* topic, byte* payload, unsigned int length) {

  String myString=(char*)payload;
  Serial.println("Long= "+String(length));

#ifdef DEBUG_ACCESO
  Serial.print("Message llegado [");
  Serial.print(topic);
  Serial.print("]= ");
#endif
  for (unsigned int i = 0; i < length; i++) {
    //#ifdef DEBUG_ACCESO
    Serial.println((char)payload[i]);
    //#endif
  }


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
      abrirPuerta();
    } 
  }
  
  // Si re recibe un topic de Alta tarjeta NFC
  if ((String)topic==(String)topicAlta){
     String alta_uid= myString.substring(0, length);
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
          idPermitido[ARRAYUSE]=alta_uid;
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

void reconnectMqtt() {
  // Repetimos hasta conectar
  while (!clientMqtt.connected()) {
#ifdef DEBUG_ACCESO
    Serial.println("[MQTT]Esperando conexión con MQTT...");
#endif
    // Intentamos conectar
    if (clientMqtt.connect("Control_Accesos")) {
      clientMqtt.subscribe(topicAlta);
      clientMqtt.subscribe(topicBaja);
      clientMqtt.subscribe(topicControl);
#ifdef DEBUG_ACCESO
      Serial.println("[MQTT]Conectado");
#endif
    } 
    else {
#ifdef DEBUG_ACCESO
      Serial.print("[MQTT]Fallo, rc=");
      Serial.print(clientMqtt.state());
      Serial.println(" se intentará o travez tras 5 segundos");
#endif
      // Esperamos 5 segundos
      delay(5000);
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
void IRQ_ISR()
{
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

  //Configuración WIFI y MQTT
  setup_wifi();
  clientMqtt.setServer(servidorMqtt, 1883);
  clientMqtt.setCallback(callback);
#ifdef DEBUG_ACCESO 
  Serial.println("Configurado [MQTT]");
#endif

  //Actualización código por OTA
  ArduinoOTA.setHostname("Control_Acceso");
  ArduinoOTA.setPassword("2047");
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
#ifdef DEBUG_ACCESO
  Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
#endif
  });
  ArduinoOTA.begin();

  attachInterrupt(digitalPinToInterrupt(RQ), IRQ_ISR, FALLING);
  
#ifdef DEBUG_ACCESO 
  Serial.println("");
  Serial.println("");
  Serial.println("----- Control de Acceso NFC OPERATIVO -----");
#endif 

}

void loop () {

  String uid;
  uint8_t puerta;
  uint8_t N_uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                          // Length of the UID (4 (Mifare Classic) or 7 (Mifare Ultralight) 
                                              // bytes depending on ISO14443A card type)
                                                
  ArduinoOTA.handle(); // Actualización código por OTA
  
  // Comprobamos conexión con broker MQTT
  if (!clientMqtt.connected()) {
    reconnectMqtt();
  }
  clientMqtt.loop();

  if(NFC_Present == true) { 
    noInterrupts(); // desactivo interrupciones
    digitalWrite ( led, LOW ); // Destectada tarjeta
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
      bool noEncontrado = true;
      for(int i = 0; i < ARRAYUSE; i++){
        if(uid == idPermitido[i]){
          abrirPuerta();
          noEncontrado = false;
          break;
        }
      }
      if(noEncontrado){ // Tarjeta sin acceso
        #ifdef DEBUG_ACCESO
          Serial.print("Acceso Denegado a ISO14443A card UID: ");
          Serial.println(uid);
        #endif
      }
      // MQTT se publica el acceso
      char buf[15];
      uid.toCharArray(buf, 15);
      clientMqtt.publish(topicAcceso, buf);
    }
    else{
      uid = "0";
      #ifdef DEBUG_ACCESO
        Serial.println("No se ha encontrado ISO14443A card");
      #endif 
    } 
    digitalWrite ( led, HIGH ); // Fin presencia tarjeta 
    NFC_Present = false; // Activo variable gestión interrupción
    interrupts(); // Activo interrupciones
  }
  
  nfc.enableRead(PN532_MIFARE_ISO14443A, N_uid, &uidLength);
  
}
