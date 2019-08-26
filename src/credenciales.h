/*
Credenciales WIFI y BROKER MQTT
*/

#pragma once

// Credenciales WIFI
const char *ssid = "MOVISTAR_9E06";
const char *password = "1CD0FD833D86C2705DD2";

// Credenciales MQTT
#define MQTT_HOST IPAddress(192, 168, 1, 99)
#define MQTT_PORT 1883

//Credenciales OTA
#define OTA_Port 8266
const char *OTA_Hostname = "Control_Acceso";
const char *OTA_Password = "2047";