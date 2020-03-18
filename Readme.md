# CONTROL DE ACCESOS TARJETAS NFC / MQTT / NODERED / INFLUX

## 

### Mediante el lector de tarjetas PN532 conectado a través del bus SPI a una placa WEMOS D1 MINI y mediante interrupciones se realiza el control de accesos de una puerta. El sistema mediante MQTT se conecta a NODERED que supervisa y puede controlar los accesos. NODERED también gestiona el almacenamiento de los acceso en la base de datos INFLUXDB. Tanto el servidor MQTT (Mosquitto), NODERED y la BD INFLUXDB están instalados en una RASPBERRYPI 3, dentro de la misma red local que el  hardware del Control de Accesos

### La alimentación del hardware del Control de Accesos se realiza a través del portero automático alimentado a 12VAC. Los 12VAC son rectificados con un puente de diodos y la tensión de continua obtenidas, unos 17VCC, son reducidos a 5VCC mediante un reductor DC-DC. El el circuito de Control de Accesos se instalará en el interior    del portero automático. Esta instalación puede provoca problemas de CONEXIÓN wifi

### En paralelo con el contacto NA del Relé Shield para Wemos, conectaremos un varistor para proteger los contactos del mismo frente a la energia almacenada en la carga insductiva a controlar (bobina de apertura puerta). Si el varistor el contacto del rele se destruye en 1 año

### El control de acceso se realiza por medio de interrupciones y para solucionar cualquier problema con las mismas y WEMOS ESP8266 la función que se ejecuta cuando se produce la interrupción (ISR), debe estar en la memoria IRAM, en lugar de la memoria Flash. Si la (ISR) está en la Flash funciona la mayor parte del tiempo, pero no es confiable, ya que provoca bloqueos de la (ISR) de vez en cuando. Agregar ICACHE_RAM_ATTR en la función (ISR) soluciona este problema, coloca la (ISR) en la memoria IRAM y no se producen más bloqueos aleatorios
  
### La librería Adafruit_PN532 usada es la modifica por Oliv4945, que permite interrupciones con conexión SPI del  lector PN532. Descargala desde <https://github.com/Oliv4945/Adafruit-PN532#interrupt_spi>

### Mediante la modificación de los archivos configuracion.h y credenciales.h se puede ajustar este proyecto a otros escenarios

### Descomentando #define DEBUG_ACCESO en el archivo configuracion.h se permite el DEBUG por el puerto serie

### La actualización del software en el WEMOS DI MINI se puede realizar mediante OTA

### En el arcivo Flow_nodered está la programación de flujos en nodered

### -----------------------------------------------------------------------------------------------------------

### Hardware necesario

- 1 Condensador de 100 25V nanoFaradios
- 1 Varistor
- 1 Wemos D1 Mini
<https://es.aliexpress.com/item/ESP8266-NodeMcu-Lua-WIFI-Junta-D1-versi-n-mini/32879989375.html?spm=a2g0s.9042311.0.0.274263c0maHTwS>
- 1 Lector tarjetas PN532 NFC V3 <https://es.aliexpress.com/item/1-Unidades-PN532-NFC-RFID-m-dulo-inal-mbrico-V3-registrado-Bluetooth-lector-y-escritor-de/32863757340.html?spm=a2g0s.9042311.0.0.274263c0X0mGsc>
- 1 Puente de diodos <https://es.aliexpress.com/item/Free-shipping-20PCS-2w10-2A-1000V-diode-bridge-rectifier-2W10/32828648841.html?spm=a2g0s.9042311.0.0.12a563c07wHrsD>
- 1 Módulo de potencia regulador reductor convertidor ajustable actualizado DC-DC LM 2596 <https://es.aliexpress.com/item/33028153010.html?spm=a2g0s.9042311.0.0.274263c0sy4OQD>
- 1 Relé Shield para Wemos <https://es.aliexpress.com/item/1PCS-NEW-Relay-Shield-for-Arduino-WeMos-D1-Mini-ESP8266-Development-Board-WeMos-D1-Relay-Module/32737849680.html?spm=a2g0s.13010208.99999999.260.2d063c00OeQ5s9>
- 1 Placa Shield Wemos (Triple escudo) <https://es.aliexpress.com/item/Triple-escudo-para-WeMos-D1-Mini-Dua-de-perforaci-n-para-Arduino-Compatible/32842137583.html?spm=a2g0s.9042311.0.0.274263c0maHTwS>
- 10 Llaveros NFC's <https://es.aliexpress.com/item/10-piezas-de-la-etiqueta-RFID-de-13-56-MHz-Mif1-S50-llaveros-etiqueta-NFC-RFID/32960150464.html?spm=a2g0s.13010208.99999999.260.78623c00iNnDJ2>

### Coste total estimado del proyecto: entre 12 y 15 euros

### Desarrollado por Oscar Ruiz Muela gurues@2019-2020
