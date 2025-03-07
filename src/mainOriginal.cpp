//Librerias a utilizar en en el programa
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <PZEM004Tv30.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define PZEM_SERIAL Serial2

#define DHT1 15 //15
#define DHT2 2
#define DHT3 0
#define DHT4 4

#define DETECTOR_1 5
#define DETECTOR_2 18
#define DETECTOR_3 19
#define DETECTOR_4 21

#define RELE_1 25
#define RELE_2 26
#define ADC_1 32
#define ADC_2 36

#define ERROR 0

//#define factor_bobina 40 //AUSTRIA
//#define factor_bobina 2 // BOBINA AZUL

#define FASE_R 12
#define FASE_S 14
#define FASE_T 27
#define LED_ROJO 33


//El largo de este vector como minimo tiene que ser el tiempo de envio en segundos x2, porque leo los detectores cada 500ms
#define VECTOR_DETECTOR 250 
#define VECTOR 4

PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);
Preferences preferences;

//Definiciones del sensor de temperatura y humedad
#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE2 DHT11   // DHT 11-2
#define DHTTYPE3 DHT11   // DHT 11-3
#define DHTTYPE4 DHT11   // DHT 11-4
//#########################################################################################
//#########################################################################################
//#########################################################################################
//#########################################################################################
#define SENSOR_DHT
//#define SENSOR_DS18B20

//#########################################################################################
//#########################################################################################
//#########################################################################################
//#########################################################################################
#ifdef SENSOR_DHT
    // Creamos los objetos para DHT
    DHT dht1(DHT1, DHTTYPE);
    DHT dht2(DHT2, DHTTYPE2);
    DHT dht3(DHT3, DHTTYPE3);
    DHT dht4(DHT4, DHTTYPE4);
#elif defined(SENSOR_DS18B20)
    // Configuración para cada sensor DS18B20
    OneWire oneWire1(DHT1);
    OneWire oneWire2(DHT2);
    OneWire oneWire3(DHT3);
    OneWire oneWire4(DHT4);

    DallasTemperature sensors1(&oneWire1);
    DallasTemperature sensors2(&oneWire2);
    DallasTemperature sensors3(&oneWire3);
    DallasTemperature sensors4(&oneWire4);
#endif

//#########################################################################################
//#########################################################################################
//#########################################################################################
//#########################################################################################
//#########################################################################################

#define EQUIPO "SERIE42"
#define ESP_CLIENT_NAME espClient42
WiFiClient ESP_CLIENT_NAME;
PubSubClient client(ESP_CLIENT_NAME);
const char* CLIENT_ID = "espClient42";
float factorVoltaje = 1;
//float factorVoltaje = 1;

//float factor_bobina = 1;
float factor_bobina = 2;
//float factor_bobina = 40; //Austria bobinas negras abiertas
//float factor_bobina = 100;
//float factor_bobina = 400;

int maxWifiSSID = 3;
int maxWifiRetry = 10;

const char* redes[3][2] = {
    {"e-Invitados", "Edenor2022$"},
    {"EMSETEC4G", "IOT2024EMSETEC"},
    {"HUAWEI_311_1B94", "Maxpower1995"}
  };

/*  const char* redes[8][2] = {
    {"e-Invitados", "Edenor2022$"},
    {"EMSETEC4G", "IOT2024EMSETEC"},
    {"HUAWEI_ESP32", "Maxpower"},
    {"DECO-Wifi117", "0103371008"},
    {"HUAWEI_311_1B94", "Maxpower1995"},
    {"HUAWEI_311_90BD", "GRQLRRNAL0B"},
    {"InternetCLARO0838", "6BA4B904"},
    {"EMSETEC-2.4GHz", "33710081099"}
  };  */


//#########################################################################################
//#########################################################################################
//#########################################################################################
//#########################################################################################
//#########################################################################################

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



//Si se usa el arduino IDE quiza hay que borrar estas definiciones de funciones
void wifiInit(const char* redes[][2], int num_redes, int max_intentos);
void reconnect();
void lecturaPzems();
void leerDHT11();
void leerDetectores(void *parameter);
void seteoDetector();
void resetVectorDetectores();
void enviarDatosMQTT();
void nivelTanquedeAgua2m();
void parpadearLed(void *pvParameters);
void leerNA();
void leerCombustibleGarin();
void leerBateriaGarin();
void nivelTanquedeAgua2m_sensor2();
void leerDS18B20();
void stopWiFi();

float voltajeR = 0;
float voltajeS = 0;
float voltajeT = 0;

float corrienteR = 0;
float corrienteS = 0;
float corrienteT = 0;

float dhtTemperatura1 = 0;
float dhtHumedad1 = 0;
float dhtTemperatura2 = 0;
float dhtHumedad2 = 0;
float dhtTemperatura3 = 0;
float dhtHumedad3 = 0;
float dhtTemperatura4 = 0;
float dhtHumedad4 = 0;

int nivelTanque2m = 0;
static char NA[7];
static char Bat[7];
static char Comb[7];
int cont = 0;
int contador_tension = 0;

String detector1 = "OFF";
String detector2 = "OFF";
String detector3 = "OFF";
String detector4 = "OFF";

String vectorDetector1[VECTOR_DETECTOR];
String vectorDetector2[VECTOR_DETECTOR];
String vectorDetector3[VECTOR_DETECTOR];
String vectorDetector4[VECTOR_DETECTOR];

int contador = 0;
int contadorPzem = 0;
bool tareaActiva = true; 
bool firstTime = true;
int nivelTanque2m_sensor2 = 0;
//La bobina del pzem queda iugal
//La bobina azul se multiplica x 2
//La bobinas de 1000A se multiplican x100

const float referenceVoltage = 3.3;  // Voltaje de referencia del ADC (3.3 V para ESP32)
const int adcResolution = 4095;      // Resolución del ADC del ESP32 (12 bits)
const float resistorRatio = (82.0 + 10.0) / 10.0;  // Relación del divisor resistivo
const int numSamples = 20;
const float calibrationFactor = 1.02;  // Factor de ajuste basado en mediciones reales
//const float a = 1.210;  // Pendiente
//const float b = 1.41;   // Intersección



//-------------------- VOID SETUP -------------

void setup() {
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

  Serial.begin(115200);
  // Imprime el valor del define
  Serial.print("Este es el Modulo: ");
  Serial.println(EQUIPO);

  if (strcmp(mqtt_server, "143.198.66.52") == 0) {
    Serial.println("Conectado al Broker de EDENOR");
  } else {
    Serial.println("Conectado al Broker de PRUEBAS");
  }

  // ########## SELECCION DE TIPO DE SENSOR ##########
  #ifdef SENSOR_DHT
    dht1.begin();
    dht2.begin();
    dht3.begin();
    dht4.begin();
  #elif defined(SENSOR_DS18B20)
    sensors1.begin();
    sensors2.begin();
    sensors3.begin();
    sensors4.begin();
  #endif
  // #################################################
  //wifiInit();
  wifiInit(redes, maxWifiSSID, maxWifiRetry); // 3 redes y 10 intentos por cada una
  client.setServer(mqtt_server, 1884);

  xTaskCreate(
    leerDetectores,   // Función de la tarea
    "Leer Detectores",// Nombre de la tarea
    4096,            // Tamaño de la pila
    NULL,             // Parámetros de la tarea
    1,                // Prioridad de la tarea
    NULL              // Identificador de la tarea (no necesitamos este en este caso)
  );

  xTaskCreate(parpadearLed, "Blink Task", 4096, NULL, 0, NULL);

  resetVectorDetectores();
  analogReadResolution(12);
  Serial.println("Setup finalizado");

}




void loop() {

/*   long adcSum = 0;
  for (int i = 0; i < numSamples; i++) {
    adcSum += analogRead(ADC_1);  // Suma las lecturas del ADC
    delay(250);  // Pequeña espera entre lecturas para estabilizar la señal
  }

// Calcula el promedio de las muestras
  int adcValue = adcSum / numSamples;

  // Convierte el valor ADC a voltaje medido (Vout del divisor)
  float vout = (adcValue * referenceVoltage) / adcResolution;

  // Calcula el voltaje de entrada original (Vin)
  float vin = vout * resistorRatio;

  // Imprime los resultados
  Serial.print("Promedio ADC Value: ");
  Serial.print(adcValue);
  Serial.print(" | Vout Promedio: ");
  Serial.print(vout, 2);
  Serial.print(" V | Vin Promedio: ");
  Serial.print(vin, 2);
  Serial.println(" V");


  delay(5000); */

  tiempoReset = millis();
  if(tiempoReset >= RESET){
    ESP.restart();
  }

  tiempoActual = millis();
// Comprobar si ha pasado el tiempo especificado
  if (tiempoActual - ultimoEnvio >= TIEMPO_DE_ENVIO || firstTime == true) {

  //#########################################################################################
  //#########################################################################################
  //#########################################################################################
  //#########################################################################################
  //#########################################################################################

    //Tarrda 15 segundos
    printf("Leyendo Pzems\n");
    lecturaPzems();
    printf("Leyendo Sensores\n");

    #ifdef SENSOR_DHT
        leerDHT11();
    #elif defined(SENSOR_DS18B20)
        leerDS18B20();
    #endif


    printf("Enviando datos por MQTT\n");
    tareaActiva = false;
    
    seteoDetector();
    //leerBateriaGarin();
    //leerCombustibleGarin(); //Varilla de 25 cm
    //stopWiFi();
    //nivelTanquedeAgua2m();
    //nivelTanquedeAgua2m_sensor2();
    //leerNA(); // Varilla de 1m
    

  //#########################################################################################
  //#########################################################################################
  //#########################################################################################
  //#########################################################################################
  //#########################################################################################
  
    enviarDatosMQTT();
    resetVectorDetectores();
    tareaActiva = true;
    delay(1000); 
    ultimoEnvio = tiempoActual;
    firstTime = false;
    printf("Datos enviados");
  }

}

void stopWiFi() {
    WiFi.disconnect(true); // Desconecta y detiene el Wi-Fi
    WiFi.mode(WIFI_OFF);   // Apaga el módulo Wi-Fi
    delay(10);             // Pequeño retraso para asegurarte de que se detuvo
}

//---------------- VOID CONECTAR WiFi -------------------
void wifiInit(const char* redes[][2], int numRedes, int max_intentos) {
  preferences.begin("wifi", false); // Iniciar NVS para guardar/leer datos

  String last_ssid = preferences.getString("last_ssid", "");
  String last_password = preferences.getString("last_password", "");
  preferences.end();

  Serial.println("Iniciando conexión WiFi...");
  bool conectado = false;

  // Si hay una red guardada, intenta conectarte primero a ella
  if (last_ssid != "") {
    Serial.print("Intentando con la última red guardada: ");
    Serial.println(last_ssid);

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

  // Intentar conectarse con las redes del vector
  for (int i = 0; i < numRedes; i++) {
    const char* ssid = redes[i][0];
    const char* password = redes[i][1];

    Serial.print("Intentando conectar a: ");
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



//----------------- VOID CONECTAR MQTT ----------
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

void parpadearLed(void *pvParameters) {
  for (;;) {
    digitalWrite(LED_ROJO,HIGH);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    digitalWrite(LED_ROJO,LOW);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void leerDetectores(void *parameter) {
  for (;;) {
    if(tareaActiva == true ){
    vectorDetector1[contador] = (digitalRead(DETECTOR_1) == HIGH) ? "ON" : "OFF";
    vectorDetector2[contador] = (digitalRead(DETECTOR_2) == HIGH) ? "ON" : "OFF";
    vectorDetector3[contador] = (digitalRead(DETECTOR_3) == HIGH) ? "ON" : "OFF";
    vectorDetector4[contador] = (digitalRead(DETECTOR_4) == HIGH) ? "ON" : "OFF";
    contador ++;
    
    }else{
      //printf("Tarea Pausada\n");
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void seteoDetector(){
  for (size_t i = 0; i < VECTOR_DETECTOR; i++){
    if(vectorDetector1[i] == "ON"){
      detector1= "ON";}
    if(vectorDetector2[i] == "ON"){
      detector2= "ON";}
    if(vectorDetector3[i] == "ON"){
      detector3= "ON";}
    if(vectorDetector4[i] == "ON"){
      detector4= "ON";}
  }

  printf("Detector 1: %s\n",String(detector1).c_str());
  printf("Detector 2: %s\n",String(detector2).c_str());
  printf("Detector 3: %s\n",String(detector3).c_str());
  printf("Detector 4: %s\n",String(detector4).c_str());
}

void resetVectorDetectores(){
  for (size_t i = 0; i < VECTOR_DETECTOR; i++){
    vectorDetector1[i] = "OFF";
    vectorDetector2[i] = "OFF";
    vectorDetector3[i] = "OFF";
    vectorDetector4[i] = "OFF";
  }

  detector1 = "OFF";
  detector2 = "OFF";
  detector3 = "OFF";
  detector4 = "OFF";
  contador = 0;
}

#ifdef SENSOR_DHT
  void leerDHT11(){
  // ######## T y H ########

    dhtTemperatura1 = dht1.readTemperature();
    dhtHumedad1 = dht1.readHumidity();
    if(isnan(dhtTemperatura1)){
      dhtTemperatura1 = ERROR;
      dhtHumedad1 = ERROR;
    }
    delay(2000);

    dhtTemperatura2 = dht2.readTemperature();
    dhtHumedad2 = dht2.readHumidity();
    if(isnan(dhtTemperatura2)){
      dhtTemperatura2 = ERROR;
      dhtHumedad2 = ERROR;
    }
    delay(2000);

    dhtTemperatura3 = dht3.readTemperature();
    dhtHumedad3 = dht3.readHumidity();
    if(isnan(dhtTemperatura3)){
      dhtTemperatura3 = ERROR;
      dhtHumedad3 = ERROR;
    }
    delay(2000);

    dhtTemperatura4 = dht4.readTemperature();
    dhtHumedad4 = dht4.readHumidity();
    if(isnan(dhtTemperatura4)){
      dhtTemperatura4 = ERROR;
      dhtHumedad4 = ERROR;
    }
    delay(2000); 

    printf("Temperatura 1: %f, Humedad 1 : %f\n",dhtTemperatura1,dhtHumedad1);
    printf("Temperatura 2: %f, Humedad 2 : %f\n",dhtTemperatura2,dhtHumedad2);
    printf("Temperatura 3: %f, Humedad 3 : %f\n",dhtTemperatura3,dhtHumedad3);
    printf("Temperatura 4: %f, Humedad 4 : %f\n",dhtTemperatura4,dhtHumedad4);
  }
#elif defined(SENSOR_DS18B20)
  void leerDS18B20(){
    // Solicitar temperatura a cada sensor
    sensors1.requestTemperatures();
    sensors2.requestTemperatures();
    sensors3.requestTemperatures();
    sensors4.requestTemperatures();

      // Leer y mostrar las temperaturas
    dhtTemperatura1 = sensors1.getTempCByIndex(0);
    dhtTemperatura2 = sensors2.getTempCByIndex(0);
    dhtTemperatura3 = sensors3.getTempCByIndex(0);
    dhtTemperatura4 = sensors4.getTempCByIndex(0);

    Serial.print("Temperatura Sensor 1: ");
    Serial.println(dhtTemperatura1);
    Serial.print("Temperatura Sensor 2: ");
    Serial.println(dhtTemperatura2);
    Serial.print("Temperatura Sensor 3: ");
    Serial.println(dhtTemperatura3);
    Serial.print("Temperatura Sensor 4: ");
    Serial.println(dhtTemperatura4);
  }
#endif



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
  voltajeR = pzem.voltage();
  voltajeR = voltajeR*factorVoltaje;
  delay(1000);
  corrienteR = pzem.current()*factor_bobina;

  if(isnan(voltajeR)){
    voltajeR = ERROR;
    corrienteR = ERROR;
  }

  printf("Voltaje Fase R: %f \n",voltajeR);
  printf("Corriente Fase R: %f\n",corrienteR);

// ######## FASE S ########
  digitalWrite(FASE_S, HIGH);
  digitalWrite(FASE_T, HIGH);
  digitalWrite(FASE_R, HIGH);
  delay(2000);
  //pzem.resetEnergy();
  digitalWrite(FASE_S, LOW);


  delay(3000);
  //printf("FASE S\n");
  voltajeS = pzem.voltage();
  voltajeS = voltajeS*factorVoltaje;
  delay(1000);
  corrienteS = pzem.current()*factor_bobina;

  if(isnan(voltajeS)){
    voltajeS = ERROR;
    corrienteS = ERROR;
  }

  printf("Voltaje Fase S: %f\n",voltajeS);
  printf("Corriente Fase S: %f\n",corrienteS);


// ######## FASE T ########
  digitalWrite(FASE_S, HIGH);
  digitalWrite(FASE_T, HIGH);
  digitalWrite(FASE_R, HIGH);
  delay(2000);
  //pzem.resetEnergy();
  digitalWrite(FASE_T, LOW);


  delay(3000);
  //printf("FASE T\n");
  voltajeT = pzem.voltage();
  voltajeT = voltajeT*factorVoltaje;
  delay(1000);
  corrienteT = pzem.current()*factor_bobina;

  if(isnan(voltajeT)){
    voltajeT = ERROR;
    corrienteT = ERROR;
  }

  printf("Voltaje Fase T: %f\n",voltajeT);
  printf("Corriente Fase T: %f\n",corrienteT);

  digitalWrite(FASE_S, HIGH);
  digitalWrite(FASE_T, HIGH);
  digitalWrite(FASE_R, HIGH);
  delay(2000);
  printf("******Fin de Leer Pzem******\n");
}

void nivelTanquedeAgua2m() {
  //Consta de dos flotantes, cuando se abre uno marca 60% y cuando se abren los dos marca 30%
  //Va en serie con una r de 220 ohm y otra en paralo al ADC
  nivelTanque2m=analogRead(ADC_1);
  nivelTanque2m=0.044*nivelTanque2m-30.52;

  if (nivelTanque2m >= 70) {  
    nivelTanque2m =100;
  }
  if (nivelTanque2m >= 60 && nivelTanque2m<70) {  
    nivelTanque2m =70;
  }
  if (nivelTanque2m < 60) {  
    nivelTanque2m =60;
  }
 
  Serial.print("Nivel de Combustible: "); 
  Serial.print(nivelTanque2m); Serial.println("%; ");
  Serial.println("-----------------------");

    
 delay(1000); 
}

void nivelTanquedeAgua2m_sensor2() {
  //Consta de dos flotantes, cuando se abre uno marca 60% y cuando se abren los dos marca 30%
  //Va en serie con una r de 220 ohm y otra en paralo al ADC
  nivelTanque2m_sensor2=analogRead(ADC_2);
  nivelTanque2m_sensor2=0.044*nivelTanque2m_sensor2-30.52;

  if (nivelTanque2m_sensor2 >= 70) {  
    nivelTanque2m_sensor2 =100;
  }
  if (nivelTanque2m_sensor2 >= 60 && nivelTanque2m_sensor2<70) {  
    nivelTanque2m_sensor2 = 70;
  }
  if (nivelTanque2m_sensor2 < 60) {  
    nivelTanque2m_sensor2 = 60;
  }
 
  Serial.print("Nivel de Combustible 2: "); 
  Serial.print(nivelTanque2m_sensor2); Serial.println("%; ");
  Serial.println("-----------------------");

    
 delay(1000); 
}
//#########################################################################################
//#########################################################################################
//#########################################################################################
//#########################################################################################
//#########################################################################################

void enviarDatosMQTT(){
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
    client.connect(CLIENT_ID, mqtt_user, mqtt_pass);

  client.publish("EMSETEC/" EQUIPO "/FASER/VOLTAJE:", String(voltajeR).c_str());delay(250);
  client.publish("EMSETEC/" EQUIPO "/FASES/VOLTAJE:", String(voltajeS).c_str());delay(250);
  client.publish("EMSETEC/" EQUIPO "/FASET/VOLTAJE:", String(voltajeT).c_str());delay(250);
  client.publish("EMSETEC/" EQUIPO "/FASER/CORRIENTE:", String(corrienteR).c_str());delay(250);
  client.publish("EMSETEC/" EQUIPO "/FASES/CORRIENTE:", String(corrienteS).c_str());delay(250);
  client.publish("EMSETEC/" EQUIPO "/FASET/CORRIENTE:", String(corrienteT).c_str());delay(250);

  if(dhtTemperatura1 > 0){
    client.publish("EMSETEC/" EQUIPO "/SENSOR1/TEMPERATURA:", String(dhtTemperatura1).c_str());delay(250);
  }if(dhtTemperatura2 > 0){
    client.publish("EMSETEC/" EQUIPO "/SENSOR2/TEMPERATURA:", String(dhtTemperatura2).c_str());delay(250);
  }if(dhtTemperatura3 > 0){
    client.publish("EMSETEC/" EQUIPO "/SENSOR3/TEMPERATURA:", String(dhtTemperatura3).c_str());delay(250);
  }if(dhtTemperatura4 > 0){
    client.publish("EMSETEC/" EQUIPO "/SENSOR4/TEMPERATURA:", String(dhtTemperatura4).c_str());delay(250);
  }if(dhtHumedad1 > 0){
    client.publish("EMSETEC/" EQUIPO "/SENSOR1/HUMEDAD:", String(dhtHumedad1).c_str());delay(250);
  }if(dhtHumedad2 > 0){
    client.publish("EMSETEC/" EQUIPO "/SENSOR2/HUMEDAD:", String(dhtHumedad2).c_str());delay(250);
  }if(dhtHumedad3 > 0){
    client.publish("EMSETEC/" EQUIPO "/SENSOR3/HUMEDAD:", String(dhtHumedad3).c_str());delay(250);
  }if(dhtHumedad4 > 0){
    client.publish("EMSETEC/" EQUIPO "/SENSOR4/HUMEDAD:", String(dhtHumedad4).c_str());delay(250);
  }
  
  //client.publish("EMSETEC/" EQUIPO "/DETECTOR1/ESTADO:", String(detector1).c_str());delay(250);
  //client.publish("EMSETEC/" EQUIPO "/DETECTOR2/ESTADO:", String(detector2).c_str());delay(250);
  //detector3 = (detector3 == "ON") ? "OFF" : "ON";
  //client.publish("EMSETEC/" EQUIPO "/DETECTOR1/ESTADO:", String(detector1).c_str());delay(250);
  //client.publish("EMSETEC/" EQUIPO "/DETECTOR2/ESTADO:", String(detector2).c_str());delay(250);
  //client.publish("EMSETEC/" EQUIPO "/DETECTOR3/ESTADO:", String(detector3).c_str());delay(250);
  //client.publish("EMSETEC/" EQUIPO "/DETECTOR4/ESTADO:", String(detector4).c_str());delay(250);

  //client.publish("EMSETEC/" EQUIPO "/DETECTOR5/ESTADO:", String(detector1).c_str());delay(250);
  //client.publish("EMSETEC/" EQUIPO "/DETECTOR6/ESTADO:", String(detector2).c_str());delay(250);

  //client.publish("EMSETEC/" EQUIPO "/AD3/NIVELAGUA:", NA); // Nivel de Inundacion
  //client.publish("EMSETEC/" EQUIPO "/SENSOR1/COMBUSTIBLE:", String(nivelTanque2m).c_str());delay(250); //Nivel de Tanque de Agua
  //client.publish("EMSETEC/" EQUIPO "/SENSOR2/COMBUSTIBLE:", String(nivelTanque2m_sensor2).c_str());delay(250); //Nivel de Tanque de Agua
  //client.publish("EMSETEC/SERIE10/AD1/COMBUSTIBLE:", Comb);delay(250);
  //client.publish("EMSETEC/SERIE10/AD2/VOLTAJE:", Bat);delay(250);

}

//#########################################################################################
//#########################################################################################
//#########################################################################################
//#########################################################################################
//#########################################################################################

  void leerNA() {

//  Valor=analogRead(32);

int total = 0;

// Integrar las muestras
  for (int i = 0; i < numSamples; i++) {
    total += analogRead(ADC_1);
    delay(10); // Pequeño retraso para evitar lecturas muy rápidas
  }
  
  // Calcular el promedio
  int Valor = total / numSamples;




  
  
 // Resistencia de 100ohm en serie con medidor de combustible y alimentado de 3,3V

// Valor =-0.00003*pow(Valor, 2)) + 0.0321*Valor + 91.297;

//Nivel de agua medido con una varilla de 1 metro
Valor =-0.0332*Valor+127.88;
  
if (Valor <=0.2) {  
 Valor = 0;
 }
 
   
 
  
 

//envia el estado del Valor solamente cuando cambia.

 // if (Valor != UltimoValor) { Serial.print("Nivel de agua: "); 
 // Serial.print(Valor); Serial.println(" centimetros; "); 
 // UltimoValor=Valor;
// }
 //else { Serial.print(Valor); Serial.println("%; ");
     
  //  }
 Serial.print("Nivel de Agua: "); 
 Serial.print(Valor); Serial.println(" cm; ");
Serial.println("-----------------------");

  
    dtostrf(Valor, 6, 2, NA);

    
  
    
  
    
 delay(1000); 
  }


//--------------------VOID COMBUSTIBLE--------------------------------
void leerCombustibleGarin() {

//  Valor=analogRead(32);

int total = 0;

// Integrar las muestras pausada
  for (int i = 0; i < numSamples; i++) {
    total += analogRead(ADC_1);
    delay(10); // Pequeño retraso para evitar lecturas muy rápidas
  }
  
  // Calcular el promedio
  int Valor = total / numSamples;


 // Resistencia de 100ohm en serie con medidor de combustible y alimentado de 3,3V
if (Valor < 3400) {  
// Valor =-0.00003*pow(Valor, 2)) + 0.0321*Valor + 91.297;
Valor =-0.0378*Valor+161.06;
  }
 else { Valor=0;}
  
 if (Valor >95) {  
  Valor =100;
  }
   
 
//envia el estado del Valor solamente cuando cambia.

 // if (Valor != UltimoValor) { Serial.print("Nivel de Combustible: "); 
 // Serial.print(Valor); Serial.println("%; "); 
 // UltimoValor=Valor;
// }
 //else { Serial.print(Valor); Serial.println("%; ");
     
  //  }
 Serial.print("Nivel de Combustible: "); 
 Serial.print(Valor); Serial.println("%; ");
  Serial.println("-----------------------");

  //static char Comb[7];
    dtostrf(Valor, 6, 2, Comb);

    
  
    
  //client.publish("EMSETEC/SERIE10/AD1/COMBUSTIBLE:", Comb);
    
 delay(1000); 
  }


  //--------------------VOID BATERIA--------------------------------


  void leerBateriaGarin() {

//  Valor2=analogRead(32);

int total = 0;

// Integrar las muestras
  for (int i = 0; i < numSamples; i++) {
    total += analogRead(ADC_1);
    delay(10); // Pequeño retraso para evitar lecturas muy rápidas
  }
  
  // Calcular el promedio
  float Valor2 = total / numSamples;



  
  
 
// Valor de calibracion para medir voltaje de bateria DE 12V

//Valor2 =0.0086*Valor2+1.4977;
//Valor2 =0.0134*Valor2+3.4582;
//Valor2 =0.0134*Valor2+2.7286;
Valor2 =0.0086*Valor2+5.4977;

//  }
// else { Valor=0;}
  
// if (Valor >95) {  
 // Valor =100;
 // }
   
 
  
 


 Serial.print("Nivel de BATERIA: "); 
 Serial.print(Valor2); Serial.println("V; ");
  Serial.println("-----------------------");

  //static char Bat[7];
    dtostrf(Valor2, 6, 2, Bat);

    
  
    
  //client.publish("EMSETEC/SERIE10/AD2/VOLTAJE:", Bat);
    
 delay(1000); 
  }
  
 
