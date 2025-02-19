#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "htmlpage.hpp"
#include <PubSubClient.h>
#include "DHT.h"
#include <PZEM004Tv30.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// Definir el GPIO para activar el modo AP
#define CONFIG_PIN 4
#define DHT1 15 //15
#define DHT2 2
#define DHT3 0
#define DHT4 4

#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE2 DHT11   // DHT 11-2
#define DHTTYPE3 DHT11   // DHT 11-3
#define DHTTYPE4 DHT11   // DHT 11-4

#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define PZEM_SERIAL Serial2

//El largo de este vector como minimo tiene que ser el tiempo de envio en segundos x2, porque leo los detectores cada 500ms
#define VECTOR_DETECTOR 250 
#define VECTOR 4

// HTML almacenado en un string (ya lo definimos antes)
extern const char htmlPage[];
extern WebServer server;
extern Preferences preferences;
extern Config config;  // Variable global para almacenar la configuración
PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);



DHT dht1(DHT1, DHTTYPE);
DHT dht2(DHT2, DHTTYPE2);
DHT dht3(DHT3, DHTTYPE3);
DHT dht4(DHT4, DHTTYPE4);

OneWire oneWire1(DHT1);
OneWire oneWire2(DHT2);
OneWire oneWire3(DHT3);
OneWire oneWire4(DHT4);

DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);
DallasTemperature sensors3(&oneWire3);
DallasTemperature sensors4(&oneWire4);

const char* redes[3][2] = {
  {"e-Invitados", "Edenor2022$"},
  {"EMSETEC4G", "IOT2024EMSETEC"},
  {"HUAWEI_311_1B94", "Maxpower1995"}
};

//Incluir la direccion del servidor MQTT
const char* mqtt_server = "143.198.66.52";// Broker EDENOR
//const char* mqtt_server = "143.244.186.22"; // Broker Prueba
const char* mqtt_user = "emsetec";
const char* mqtt_pass = "84705200";

//Variables para enviar datos por MQTT
const unsigned long TIEMPO_DE_ENVIO = 60000;  
const unsigned long RESET = 3600000*2;  // 1 hora en milisegundos
unsigned long ultimoEnvio = 0;  // Tiempo de la última ejecución
unsigned long tiempoActual = 0;
unsigned long tiempoReset = 0;

//Variables de Funcionamiento
float factorVoltaje = 1;

void setup() {
  Serial.begin(115200);
  pinMode(CONFIG_PIN, INPUT_PULLUP);
  loadConfiguration();
  printConfiguration();
  
  // Si el pin de configuración está en alto, entra en modo AP
  if (digitalRead(CONFIG_PIN) == LOW) {
    startAP();
    server.on("/", handleRoot);
    server.on("/save", handleSave);
    server.begin();
    Serial.println("Servidor web iniciado en modo AP.");
  } else {
    Serial.println("Modo normal. No se inicia configuración.");
    // Aquí podrías cargar la configuración y conectar a WiFi, etc.
    //loadConfiguration();
    //printConfiguration();
  }

  if (config.sensor == "DHT11") {
    dht1.begin();
    dht2.begin();
    dht3.begin();
    dht4.begin();
    Serial.println("Se usan: DHT11");
  } else if (config.sensor == "DS18B20") {
    sensors1.begin();
    sensors2.begin();
    sensors3.begin();
    sensors4.begin();   
    Serial.println("Se usan: DS18B20"); 
  } else {
    Serial.println("No se mide Temperatura");
  }

  if (config.conexion == "Trifasica") {
    factorVoltaje = sqrt(3);
    Serial.println("Se mide tension Trifásica");
  } else if (config.conexion == "Monofasica") {
    factorVoltaje = 1; 
    Serial.println("Se mide tension Monofásica");
  } else {
    Serial.println("No se mide Tensión");
  }

}


void loop() {
    if (WiFi.getMode() == WIFI_AP) {
      server.handleClient();
    }
    // Aquí el resto de la lógica del dispositivo...
  }