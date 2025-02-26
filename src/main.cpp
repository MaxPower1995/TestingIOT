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
#include <string>

// Definir el GPIO para activar el modo AP
#define CONFIG_PIN 4
#define DHT1 15 //15
#define DHT2 2
#define DHT3 0
#define DHT4 4

#define DETECTOR_1 5
#define DETECTOR_2 18
#define DETECTOR_3 19
#define DETECTOR_4 21

#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE2 DHT11   // DHT 11-2
#define DHTTYPE3 DHT11   // DHT 11-3
#define DHTTYPE4 DHT11   // DHT 11-4

#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define PZEM_SERIAL Serial2

#define RELE_1 25
#define RELE_2 26
#define ADC_1 32
#define ADC_2 36

#define FASE_R 12
#define FASE_S 14
#define FASE_T 27
#define LED_ROJO 33

//El largo de este vector como minimo tiene que ser el tiempo de envio en segundos x2, porque leo los detectores cada 500ms
#define VECTOR_DETECTOR 250 
#define VECTOR 4
#define ERROR 0

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
float voltaje[3] = {0,0,0};
float corriente[3] = {0,0,0};
float temperatura[4] = {0,0,0,0};
float humedad[4] = {0,0,0,0};
float adc[4] = {0,0,0,0};
float factorBobina = 1;

int maxWifiSSID = 3;
int maxWifiRetry = 10;


#define ESP_CLIENT_NAME espClient
WiFiClient ESP_CLIENT_NAME;
PubSubClient client(ESP_CLIENT_NAME);

String CLIENT_ID;
String EQUIPO;

const char* redes[3][2] = {
    {"e-Invitados", "Edenor2022$"},
    {"EMSETEC4G", "IOT2024EMSETEC"},
    {"HUAWEI_311_1B94", "Maxpower1995"}
  };

//Funciones
void wifiInit(const char* redes[][2], int num_redes, int max_intentos);
void lecturaPzems();
void enviarDatosMQTT();
void parpadearLed(void *pvParameters);
void reconnect();
void leerDHT11();
void leerDS18B20();
void configuracionPines();

void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(CONFIG_PIN, INPUT_PULLUP);
  Serial.println("#######################################");
  Serial.println("    Incicializando el Sistema...");
  Serial.println("#######################################");
  loadConfiguration();
  configuracionPines();
  //printConfiguration();

  EQUIPO = "SERIE" + String(config.serialNumber);
  CLIENT_ID = "espClient"+ String(config.serialNumber);


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

  factorBobina = config.factor;
  Serial.print("Factor de Bobina: ");
  Serial.println(factorBobina);

  // Imprime el valor del define
  Serial.print("Este es el Modulo: ");
  Serial.println(EQUIPO);
  Serial.print("Con el Cliente: ");
  Serial.println(CLIENT_ID);

  if (strcmp(mqtt_server, "143.198.66.52") == 0) {
    Serial.println("Conectado al Broker de EDENOR");
  } else {
    Serial.println("Conectado al Broker de PRUEBAS");
  }
  
  client.setServer(mqtt_server, 1884);
  //xTaskCreate(leerDetectores,"Leer Detectores",4096,NULL,1,NULL);
  xTaskCreate(parpadearLed, "Blink Task", 4096, NULL, 0, NULL);
  //resetVectorDetectores();
  //analogReadResolution(12);
  Serial.println("#######################################");
  Serial.println("        Sistema Configurado");
  Serial.println("#######################################");
}


void loop() {
  tiempoReset = millis();
  if(tiempoReset >= RESET){
    ESP.restart();
  }
  tiempoActual = millis();

    if (WiFi.getMode() == WIFI_AP) {
      server.handleClient();
    }else{
    // Aquí el resto de la lógica del dispositivo...
    if (WiFi.status() != WL_CONNECTED) {
      wifiInit(redes, maxWifiSSID, maxWifiRetry);
    }

    Serial.println("Leyendo los Pzems");
    lecturaPzems();

    enviarDatosMQTT();
    }


}

  //---------------- VOID CONECTAR WiFi -------------------
void wifiInit(const char* redes[][2], int numRedes, int max_intentos) {
  Serial.println("Iniciando conexión WiFi...");
  bool conectado = false;
  preferences.begin("wifi", false); // Iniciar NVS para guardar/leer datos

  String last_ssid = preferences.getString("last_ssid", "");
  String last_password = preferences.getString("last_password", "");
  preferences.end();

  

  // Si hay una red guardada, intenta conectarte primero a ella
  if (last_ssid != "") {
    Serial.print("Intentando con la última red guardada: ");
    Serial.print(last_ssid);

    WiFi.begin(last_ssid.c_str(), last_password.c_str());

    for (int i = 0; i < max_intentos; i++) {
      if (WiFi.status() == WL_CONNECTED) {
        conectado = true;
        break;
      }
      delay(500);
      Serial.print(".");
    }

    if (conectado) {
      Serial.println("\nConectado a la última red guardada.");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      return;
    } else {
      Serial.println("\nNo se pudo conectar a la última red guardada.");
    }
  }

  //Intenta conectarse a la red que se le cargo por formulario HTML
  Serial.print("Intentando conectar a: ");
  Serial.println(config.wifiSSID);
  WiFi.begin(config.wifiSSID, config.wifiPassword);
  for (int j = 0; j < max_intentos; j++) {
    if (WiFi.status() == WL_CONNECTED) {
      conectado = true;

      // Guardar esta red en NVS
      preferences.begin("wifi", false);
      preferences.putString("last_ssid", config.wifiSSID);
      preferences.putString("last_password", config.wifiPassword);
      preferences.end();

      Serial.println("\nConexión exitosa.");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      return;
    }
    delay(500);
    Serial.print(".");
  }

  // Intentar conectarse con las redes del vector
  for (int i = 0; i < numRedes; i++) {
    const char* ssid = redes[i][0];
    const char* password = redes[i][1];

    Serial.print("Intentando conectar a redes standars: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    for (int j = 0; j < max_intentos; j++) {
      if (WiFi.status() == WL_CONNECTED) {
        conectado = true;

        // Guardar esta red en NVS
        preferences.begin("wifi", false);
        preferences.putString("last_ssid", ssid);
        preferences.putString("last_password", password);
        preferences.end();

        Serial.println("\nConexión exitosa.");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        return;
      }
      delay(500);
      Serial.print(".");
    }

    if (!conectado) {
      Serial.println("\nNo se pudo conectar a esta red.");
    }
  }

  // Si no se conecta a ninguna red, reinicia el ESP32
  Serial.println("Error: No se pudo conectar a ninguna red, reiniciando...");
  ESP.restart();
}

void configuracionPines(){
  pinMode(FASE_R, OUTPUT); // D1 salida digital activa optoacopladores 
  digitalWrite(FASE_R, HIGH);
  pinMode(FASE_S, OUTPUT); // D6 salida digital activa optoacopladores
  digitalWrite(FASE_S, HIGH);
  pinMode(FASE_T, OUTPUT); // D5 salida digital activa optoacopladores
  digitalWrite(FASE_T, HIGH);
  pinMode(LED_ROJO, OUTPUT); // Sensor encendido
  pinMode(DETECTOR_1, OUTPUT); //  Detector 1
  digitalWrite(DETECTOR_1, LOW);
  pinMode(DETECTOR_2, OUTPUT); // Detector 2
  digitalWrite(DETECTOR_2, LOW);
  pinMode(DETECTOR_3, OUTPUT); // Detector 3
  digitalWrite(DETECTOR_3, LOW);
  pinMode(DETECTOR_4, OUTPUT); // Detector 4
  digitalWrite(DETECTOR_4, LOW);
  pinMode(RELE_1, OUTPUT); // RELE
  digitalWrite(RELE_1, LOW);
  pinMode(RELE_2, OUTPUT); // RELE 2
  digitalWrite(RELE_2, LOW);
}

void parpadearLed(void *pvParameters) {
  for (;;) {
    digitalWrite(LED_ROJO,HIGH);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    digitalWrite(LED_ROJO,LOW);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void reconnect() {
  Serial.print("Intentando conectar a MQTT...");
  if ("ESP32S2Client") {
    Serial.println("conectado");
  } else {
    Serial.print("fallo, rc=");
    Serial.print(client.state());
    ESP.restart();
    delay(5000);
  }
}

void leerDHT11(){
    temperatura[0] = dht1.readTemperature();
    humedad[0] = dht1.readHumidity();
    if(isnan(temperatura[0])){
      temperatura[0] = ERROR;
      humedad[0] = ERROR;
    }
    delay(2000);

    temperatura[1] = dht2.readTemperature();
    humedad[1] = dht2.readHumidity();
    if(isnan(temperatura[1])){
      temperatura[1] = ERROR;
      humedad[1] = ERROR;
    }
    delay(2000);

    temperatura[2] = dht3.readTemperature();
    humedad[2] = dht3.readHumidity();
    if(isnan(temperatura[2])){
      temperatura[2] = ERROR;
      humedad[2] = ERROR;
    }
    delay(2000);

    temperatura[3] = dht4.readTemperature();
    humedad[3] = dht4.readHumidity();
    if(isnan(temperatura[3])){
      temperatura[3] = ERROR;
      humedad[3] = ERROR;
    }
    delay(2000); 

    printf("Temperatura 1: %f, Humedad 1 : %f\n",temperatura[0],humedad[0]);
    printf("Temperatura 2: %f, Humedad 2 : %f\n",temperatura[1],humedad[1]);
    printf("Temperatura 3: %f, Humedad 3 : %f\n",temperatura[2],humedad[2]);
    printf("Temperatura 4: %f, Humedad 4 : %f\n",temperatura[3],humedad[3]);
  }

void leerDS18B20(){
    // Solicitar temperatura a cada sensor
    sensors1.requestTemperatures();
    sensors2.requestTemperatures();
    sensors3.requestTemperatures();
    sensors4.requestTemperatures();

      // Leer y mostrar las temperaturas
    temperatura[0] = sensors1.getTempCByIndex(0);
    temperatura[1] = sensors2.getTempCByIndex(0);
    temperatura[2] = sensors3.getTempCByIndex(0);
    temperatura[3] = sensors4.getTempCByIndex(0);

    Serial.print("Temperatura Sensor 1: ");
    Serial.println(temperatura[0]);
    Serial.print("Temperatura Sensor 2: ");
    Serial.println(temperatura[1]);
    Serial.print("Temperatura Sensor 3: ");
    Serial.println(temperatura[2]);
    Serial.print("Temperatura Sensor 4: ");
    Serial.println(temperatura[3]);
}

void lecturaPzems(){
  // ######## FASE R ########
    digitalWrite(FASE_S, HIGH);
    digitalWrite(FASE_T, HIGH);
    digitalWrite(FASE_R, HIGH);
    delay(2000);
    //pzem.resetEnergy();
    digitalWrite(FASE_R, LOW);
  
    delay(3000);
    //printf("FASE R\n");
    voltaje[0] = pzem.voltage();
    voltaje[0] = voltaje[0]*factorVoltaje;
    delay(1000);
    corriente[0] = pzem.current()*factorBobina;
  
    if(isnan(voltaje[0])){
      voltaje[0] = ERROR;
      corriente[0] = ERROR;
    }
  
    printf("Voltaje Fase R: %f \n",voltaje[0]);
    printf("Corriente Fase R: %f\n",corriente[0]);
  
  // ######## FASE S ########
    digitalWrite(FASE_S, HIGH);
    digitalWrite(FASE_T, HIGH);
    digitalWrite(FASE_R, HIGH);
    delay(2000);
    //pzem.resetEnergy();
    digitalWrite(FASE_S, LOW);
  
  
    delay(3000);
    //printf("FASE S\n");
    voltaje[1] = pzem.voltage();
    voltaje[1] = voltaje[1]*factorVoltaje;
    delay(1000);
    corriente[1] = pzem.current()*factorBobina;
  
    if(isnan(voltaje[1])){
      voltaje[1] = ERROR;
      corriente[1] = ERROR;
    }
  
    printf("Voltaje Fase S: %f\n",voltaje[1]);
    printf("Corriente Fase S: %f\n",corriente[1]);
  
  
  // ######## FASE T ########
    digitalWrite(FASE_S, HIGH);
    digitalWrite(FASE_T, HIGH);
    digitalWrite(FASE_R, HIGH);
    delay(2000);
    //pzem.resetEnergy();
    digitalWrite(FASE_T, LOW);
  
  
    delay(3000);
    //printf("FASE T\n");
    voltaje[2] = pzem.voltage();
    voltaje[2] = voltaje[2]*factorVoltaje;
    delay(1000);
    corriente[2] = pzem.current()*factorBobina;
  
    if(isnan(voltaje[2])){
      voltaje[2] = ERROR;
      corriente[2] = ERROR;
    }
  
    printf("Voltaje Fase T: %f\n",voltaje[2]);
    printf("Corriente Fase T: %f\n",corriente[2]);
  
    digitalWrite(FASE_S, HIGH);
    digitalWrite(FASE_T, HIGH);
    digitalWrite(FASE_R, HIGH);
    delay(2000);
    printf("******Fin de Leer Pzem******\n");
  }

  void enviarDatosMQTT(){
    char topicBuffer[100]; // Buffer for the topic
    char payloadBuffer[20]; // Buffer for the payload

    if (!WiFi.isConnected()) {
      Serial.println("Wi-Fi desconectado. Intentando reconectar...");
      //wifiInit();
      wifiInit(redes, maxWifiSSID, maxWifiRetry);
    }
  
    if (!client.connected()) {
      reconnect();
    }
    //Se debe colocar el numero de cliente correspondiente al modulo utilizado
    if (!client.loop())
      client.connect(CLIENT_ID.c_str(), mqtt_user, mqtt_pass);
  
    sprintf(topicBuffer, "EMSETEC/%s/FASER/VOLTAJE", EQUIPO.c_str());
    sprintf(payloadBuffer, "%.2f", voltaje[0]);
    client.publish(topicBuffer, payloadBuffer);
    delay(250);

    sprintf(topicBuffer, "EMSETEC/%s/FASES/VOLTAJE", EQUIPO.c_str());
    sprintf(payloadBuffer, "%.2f", voltaje[1]);
    client.publish(topicBuffer, payloadBuffer);
    delay(250);

    sprintf(topicBuffer, "EMSETEC/%s/FASET/VOLTAJE", EQUIPO.c_str());
    sprintf(payloadBuffer, "%.2f", voltaje[2]);
    client.publish(topicBuffer, payloadBuffer);
    delay(250);

    sprintf(topicBuffer, "EMSETEC/%s/FASER/CORRIENTE", EQUIPO.c_str());
    sprintf(payloadBuffer, "%.2f", corriente[0]);
    client.publish(topicBuffer, payloadBuffer);
    delay(250);
    
    sprintf(topicBuffer, "EMSETEC/%s/FASES/CORRIENTE", EQUIPO.c_str());
    sprintf(payloadBuffer, "%.2f", corriente[1]);
    client.publish(topicBuffer, payloadBuffer);
    delay(250);

    sprintf(topicBuffer, "EMSETEC/%s/FASET/CORRIENTE", EQUIPO.c_str());
    sprintf(payloadBuffer, "%.2f", corriente[2]);
    client.publish(topicBuffer, payloadBuffer);
    delay(250);



  
    if(temperatura[0] > 0){
      sprintf(topicBuffer, "EMSETEC/%s/SENSOR1/TEMPERATURA", EQUIPO.c_str());
      sprintf(payloadBuffer, "%.2f", temperatura[0]);
      client.publish(topicBuffer, payloadBuffer);
      delay(250);
  }
  if(temperatura[1] > 0){
      sprintf(topicBuffer, "EMSETEC/%s/SENSOR2/TEMPERATURA", EQUIPO.c_str());
      sprintf(payloadBuffer, "%.2f", temperatura[1]);
      client.publish(topicBuffer, payloadBuffer);
      delay(250);
  }
  if(temperatura[2] > 0){
      sprintf(topicBuffer, "EMSETEC/%s/SENSOR3/TEMPERATURA", EQUIPO.c_str());
      sprintf(payloadBuffer, "%.2f", temperatura[2]);
      client.publish(topicBuffer, payloadBuffer);
      delay(250);
  }
  if(temperatura[3] > 0){
      sprintf(topicBuffer, "EMSETEC/%s/SENSOR4/TEMPERATURA", EQUIPO.c_str());
      sprintf(payloadBuffer, "%.2f", temperatura[3]);
      client.publish(topicBuffer, payloadBuffer);
      delay(250);
  }

  if(humedad[0] > 0){
      sprintf(topicBuffer, "EMSETEC/%s/SENSOR1/HUMEDAD", EQUIPO.c_str());
      sprintf(payloadBuffer, "%.2f", humedad[0]);
      client.publish(topicBuffer, payloadBuffer);
      delay(250);
  }
  if(humedad[1] > 0){
      sprintf(topicBuffer, "EMSETEC/%s/SENSOR2/HUMEDAD", EQUIPO.c_str());
      sprintf(payloadBuffer, "%.2f", humedad[1]);
      client.publish(topicBuffer, payloadBuffer);
      delay(250);
  }
  if(humedad[2] > 0){
      sprintf(topicBuffer, "EMSETEC/%s/SENSOR3/HUMEDAD", EQUIPO.c_str());
      sprintf(payloadBuffer, "%.2f", humedad[2]);
      client.publish(topicBuffer, payloadBuffer);
      delay(250);
  }
  if(humedad[3] > 0){
      sprintf(topicBuffer, "EMSETEC/%s/SENSOR4/HUMEDAD", EQUIPO.c_str());
      sprintf(payloadBuffer, "%.2f", humedad[3]);
      client.publish(topicBuffer, payloadBuffer);
      delay(250);
  }
    
    //client.publish("EMSETEC/" +EQUIPO+ "/DETECTOR1/ESTADO:", String(detector1).c_str());delay(250);
    //client.publish("EMSETEC/" +EQUIPO+ "/DETECTOR2/ESTADO:", String(detector2).c_str());delay(250);
    //detector3 = (detector3 == "ON") ? "OFF" : "ON";
    //client.publish("EMSETEC/" +EQUIPO+ "/DETECTOR1/ESTADO:", String(detector1).c_str());delay(250);
    //client.publish("EMSETEC/" +EQUIPO+ "/DETECTOR2/ESTADO:", String(detector2).c_str());delay(250);
    //client.publish("EMSETEC/" +EQUIPO+ "/DETECTOR3/ESTADO:", String(detector3).c_str());delay(250);
    //client.publish("EMSETEC/" +EQUIPO+ "/DETECTOR4/ESTADO:", String(detector4).c_str());delay(250);
  
    //client.publish("EMSETEC/" +EQUIPO+ "/DETECTOR5/ESTADO:", String(detector1).c_str());delay(250);
    //client.publish("EMSETEC/" +EQUIPO+ "/DETECTOR6/ESTADO:", String(detector2).c_str());delay(250);
  
    //client.publish("EMSETEC/" +EQUIPO+ "/AD3/NIVELAGUA:", NA); // Nivel de Inundacion
    //client.publish("EMSETEC/" +EQUIPO+ "/SENSOR1/COMBUSTIBLE:", String(nivelTanque2m).c_str());delay(250); //Nivel de Tanque de Agua
    //client.publish("EMSETEC/" +EQUIPO+ "/SENSOR2/COMBUSTIBLE:", String(nivelTanque2m_sensor2).c_str());delay(250); //Nivel de Tanque de Agua
    //client.publish("EMSETEC/SERIE10/AD1/COMBUSTIBLE:", Comb);delay(250);
    //client.publish("EMSETEC/SERIE10/AD2/VOLTAJE:", Bat);delay(250);
  
  }