/*
Configuración del Lector/REgistro tarjetas NFC
*/

#pragma once

#include <ESP8266WiFi.h>        // WIFI ESP8266
#include <AsyncMqttClient.h>    // MQTT Libreria asincrona basada en eventos
#include <SPI.h>                // Bus SPI
#include <Adafruit_PN532.h>     // LECTOR TRAJETAS NFC
#include <ArduinoOTA.h>         // Actualización por OTA
#include <ESP8266mDNS.h>        // Actualización por OTA
#include <WiFiUdp.h>            // Actualización por OTA
#include <Ticker.h>             // Temporizador funciones asincronas

#include <credenciales.h>      // Credencales WIFI y MQTT BROKER

// Pines SPI LECTOR NFC
#define SCK   D0
#define MOSI  D6
#define SS    D7
#define MISO  D5
#define RQ    D2

// Descomentar para mostrar debug monitor serie
#define DEBUG_ACCESO

// Definición de pines y ctes
#define led         D4            // Led de control placa Wemos
#define cerradura   D1            // Salida control rele puerta
#define ARRAYSIZE   11            // Son 10 tarjetas y la posición 11 = "" para borrar.

// TOPIC MQTT
const char* topicAcceso = "casa/puerta/acceso";
const char* topicAlta = "casa/puerta/alta_NFC";
const char* topicBaja = "casa/puerta/baja_NFC";
const char* topicControl = "casa/puerta/control_NFC";