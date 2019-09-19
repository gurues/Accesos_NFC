/*
Configuración del Lector/REgistro tarjetas NFC
*/

#pragma once

// Descomentar para mostrar debug monitor serie
#define DEBUG_ACCESO

// Pines SPI LECTOR NFC
#define SCK   D0
#define MOSI  D6
#define SS    D7
#define MISO  D5
#define RQ    D2

// Definición de pines
#define led         D4            // Led de control placa Wemos
#define cerradura   D1            // Salida control rele puerta

// Configuración tarjetas NFC --> idPermitido[0,1,2,3,4,5,6,7,8,9,10], la pos 10 para borrar
#define ARRAYSIZE   11    // Nº de tarjetas con acceso, son 10 tarjetas y la posición 11 = "" para borrar.
String idPermitido[ARRAYSIZE]={"108-18-101-3","16-31-183-195","150-156-49-249","92-127-211-3","204-149-106-3","15-50-233-67"}; // Tarjetas con acceso
int ARRAYUSE= 6; // Nº de tarjetas actuales con acceso (Puntero usado del array)

//Tiempo actuación apertura cerradura
const int msApertura = 500;         // 0,5 seg de apertura de la cerradura