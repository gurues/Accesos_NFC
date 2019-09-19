/*
Credenciales WIFI y BROKER MQTT
*/

#pragma once

// Credenciales WIFI tita
//const char *ssid = "MOVISTAR_D752";
//const char *password = "KckxpKRFbjCuU7TUTfuR";

// Credenciales WIFI Casa
const char *ssid = "MOVISTAR_9E06";
const char *password = "1CD0FD833D86C2705DD2";

IPAddress ip(192, 168, 1, 57);
IPAddress gateway(192,168,1,1);
IPAddress subnet (255,255,255,0);

// Credenciales MQTT http://192.168.99.100 BROKER DOCKER
// Credenciales MQTT http://192.168.1.99  BROKER RPI
#define MQTT_HOST IPAddress(192, 168, 1, 99)
#define MQTT_PORT 1883

// TOPIC MQTT
const char* topicAcceso = "casa/puerta/acceso";
const char* topicAlta = "casa/puerta/alta_NFC";
const char* topicBaja = "casa/puerta/baja_NFC";
const char* topicControl = "casa/puerta/control_NFC";
const char* topicTarjetas = "casa/puerta/tarjetas_NFC";
