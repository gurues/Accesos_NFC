/*
Credenciales WIFI y BROKER MQTT
*/

#pragma once

// Credenciales WIFI
const char *ssid = "xxxxxxxxxxxxxxxxxxxx";
const char *password = "xxxxxxxxxxxxxxxxxxxxxx";

IPAddress ip(xxx, xx, x, xxx);  // Modificar platformio.ini si se cambia la ip
IPAddress gateway(xxx,xxx,x,x);
IPAddress subnet (255,255,255,0);

// Credenciales MQTT PI
#define MQTT_HOST IPAddress(xxx, xxx, x, xxx)
#define MQTT_PORT 1883

// TOPIC MQTT
const char* topicAcceso = "casa/puerta/acceso";
const char* topicAlta = "casa/puerta/alta_NFC";
const char* topicBaja = "casa/puerta/baja_NFC";
const char* topicControl = "casa/puerta/control_NFC";
const char* topicTarjetas = "casa/puerta/tarjetas_NFC";
