#ifndef HTMLPAGE_HPP
#define HTMLPAGE_HPP

#include <pgmspace.h>

#define AP_SSID "ESP32CONFIG"
#define AP_PASS "1234567890"

// Servidor web en el puerto 80
WebServer server(80);
Preferences preferences;

struct Config {
    int serialNumber;
    String wifiSSID;String wifiPassword;
    String conexion;
    int factor;
    String sensor;
    String detector1;String detector2;String detector3;String detector4;
    String adc1;String adc2;String adc3;String adc4;
};

Config config;
    // Función: Iniciar Modo AP

const char htmlPage[] = R"rawliteral(
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="UTF-8">
            <title>E-Zeus Configuración </title>
            <meta name="viewport" content="width=device-width, initial-scale=1">
            <style>
                body {
                    background-color: #e8f5e9;
                    font-family: Arial, sans-serif;
                    color: #2e7d32;
                    margin: 0;
                    padding: 0;
                    text-align: center;
                }
                h2 {
                    background-color: #66bb6a;
                    padding: 20px;
                    margin: 0;
                    border-bottom: 4px solid #388e3c;
                    text-align: center;
                    color: #000000;
                }
                form {
                    background-color: #a5d6a7;
                    margin: 30px auto;
                    padding: 20px;
                    max-width: 500px;
                    border-radius: 10px;
                    box-shadow: 0 4px 6px rgba(0,0,0,0.1);
                    text-align: left;
                }
                label {
                    font-weight: bold;
                    display: block;
                    margin-top: 15px;
                    color: #000000;
                }
                input, select {
                    width: 100%;
                    padding: 10px;
                    margin-top: 5px;
                    border: 1px solid #81c784;
                    border-radius: 5px;
                    box-sizing: border-box;
                }
                button {
                    background-color: #e53935;
                    color: white;
                    border: none;
                    padding: 12px 20px;
                    margin-top: 20px;
                    border-radius: 8px;
                    font-size: 16px;
                    cursor: pointer;
                    display: block;
                    width: 100%;
                }
                button:hover {
                    background-color: #d32f2f;
                }
            </style>
        </head>
        <body>
            <h2>Configuración ESP32</h2>
            <form action="/save" method="post">
                <label>Número de Serie (0-500)</label>
                <input type="number" name="serial" min="0" max="500">
                
                <label>WiFi SSID</label>
                <input type="text" name="ssid">
                
                <label>WiFi Contraseña</label>
                <input type="password" name="password">
                
                <label>Tipo de Conexión</label>
                <select name="conexion">
                    <option value="Trifasica" selected>Trifásica</option>
                    <option value="Monofasica">Monofásica</option>
                    <option value="Sin Utilizar">Sin Utilizar</option>
                </select>
                
                <label>Factor de Bobina (0-500)</label>
                <input type="number" name="factor" min="0" max="500" value="2">
                
                <label>Sensor de Temperatura</label>
                <select name="sensor">
                    <option value="DHT11" selected>DHT11</option>
                    <option value="DS18B20">DS18B20</option>
                    <option value="Sin Utilizar">Sin Utilizar</option>
                </select>
                
                <label>Detector 1</label>
                <select name="detector1">
                    <option value="Sin Utilizar" selected>Sin Utilizar</option>
                    <option value="On/Off AA">On/Off AA</option>
                    <option value="Puerta">Puerta</option>
                    <option value="Agua">Agua</option>
                    <option value="Tanque">Tanque</option>
                </select>
                
                <label>Detector 2</label>
                <select name="detector2">
                    <option value="Sin Utilizar" selected>Sin Utilizar</option>
                    <option value="On/Off AA">On/Off AA</option>
                    <option value="Puerta">Puerta</option>
                    <option value="Agua">Agua</option>
                    <option value="Tanque">Tanque</option>
                </select>
                
                <label>Detector 3</label>
                <select name="detector3">
                    <option value="Sin Utilizar" selected>Sin Utilizar</option>
                    <option value="On/Off AA">On/Off AA</option>
                    <option value="Puerta">Puerta</option>
                    <option value="Agua">Agua</option>
                    <option value="Tanque">Tanque</option>
                </select>
                
                <label>Detector 4</label>
                <select name="detector4">
                    <option value="Sin Utilizar" selected>Sin Utilizar</option>
                    <option value="On/Off AA">On/Off AA</option>
                    <option value="Puerta">Puerta</option>
                    <option value="Agua">Agua</option>
                    <option value="Tanque">Tanque</option>
                </select>
                
                <label>ADC 1</label>
                <select name="adc1">
                    <option value="Sin Utilizar" selected>Sin Utilizar</option>
                    <option value="Combustible">Combustible</option>
                    <option value="Bateria 12">Batería 12V</option>
                    <option value="Bateria 24">Batería 24V</option>
                    <option value="Nivel Inundacion">Nivel Inundación</option>
                </select>
                
                <label>ADC 2</label>
                <select name="adc2">
                    <option value="Sin Utilizar" selected>Sin Utilizar</option>
                    <option value="Combustible">Combustible</option>
                    <option value="Bateria 12">Batería 12V</option>
                    <option value="Bateria 24">Batería 24V</option>
                    <option value="Nivel Inundacion">Nivel Inundación</option>
                </select>
                
                <label>ADC 3</label>
                <select name="adc3">
                    <option value="Sin Utilizar" selected>Sin Utilizar</option>
                    <option value="Combustible">Combustible</option>
                    <option value="Bateria 12">Batería 12V</option>
                    <option value="Bateria 24">Batería 24V</option>
                    <option value="Nivel Inundacion">Nivel Inundación</option>
                </select>
                
                <label>ADC 4</label>
                <select name="adc4">
                    <option value="Sin Utilizar" selected>Sin Utilizar</option>
                    <option value="Combustible">Combustible</option>
                    <option value="Bateria 12">Batería 12V</option>
                    <option value="Bateria 24">Batería 24V</option>
                    <option value="Nivel Inundacion">Nivel Inundación</option>
                </select>
                
                <button type="submit">Guardar Configuración</button>
            </form>
        </body>
        </html>
        )rawliteral";
    
        
void startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.println("Modo AP iniciado!");
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
  }
  
  
  // Función: Mostrar Página de Configuración
  
  void handleRoot() {
    server.send(200, "text/html", htmlPage);
  }
  
  
  // Función: Cargar Configuración de Preferences a Variables Globales
  
  void loadConfiguration() {
    preferences.begin("config", false);
    config.serialNumber = preferences.getInt("serial", 0);
    config.wifiSSID     = preferences.getString("ssid", "");
    config.wifiPassword = preferences.getString("password", "");
    config.conexion     = preferences.getString("conexion", "Trifasica");
    config.factor       = preferences.getInt("factor", 2);
    config.sensor       = preferences.getString("sensor", "DHT11");
    config.detector1    = preferences.getString("detector1", "Sin Utilizar");
    config.detector2    = preferences.getString("detector2", "Sin Utilizar");
    config.detector3    = preferences.getString("detector3", "Sin Utilizar");
    config.detector4    = preferences.getString("detector4", "Sin Utilizar");
    config.adc1         = preferences.getString("adc1", "Sin Utilizar");
    config.adc2         = preferences.getString("adc2", "Sin Utilizar");
    config.adc3         = preferences.getString("adc3", "Sin Utilizar");
    config.adc4         = preferences.getString("adc4", "Sin Utilizar");
    preferences.end();
  }
  
  
  // Función: Imprimir Configuración Actual por Serial
  
  void printConfiguration() {
    Serial.println("---- Configuración Actual ----");
    Serial.printf("Serial: %d\n", config.serialNumber);
    Serial.printf("WiFi SSID: %s\n", config.wifiSSID.c_str());
    Serial.printf("WiFi Pass: %s\n", config.wifiPassword.c_str());
    Serial.printf("Tipo de Conexión: %s\n", config.conexion.c_str());
    Serial.printf("Factor de Bobina: %d\n", config.factor);
    Serial.printf("Sensor: %s\n", config.sensor.c_str());
    Serial.printf("Detector1: %s\n", config.detector1.c_str());
    Serial.printf("Detector2: %s\n", config.detector2.c_str());
    Serial.printf("Detector3: %s\n", config.detector3.c_str());
    Serial.printf("Detector4: %s\n", config.detector4.c_str());
    Serial.printf("ADC1: %s\n", config.adc1.c_str());
    Serial.printf("ADC2: %s\n", config.adc2.c_str());
    Serial.printf("ADC3: %s\n", config.adc3.c_str());
    Serial.printf("ADC4: %s\n", config.adc4.c_str());
    Serial.println("-------------------------------");
  }
  
  
  // Función: Guardar Configuración Recibida del Formulario
  
  void handleSave() {
    if (server.method() != HTTP_POST) {
      server.send(405, "text/plain", "Método no permitido");
      return;
    }
  
    preferences.begin("config", false);
    // Solo actualizar si el campo no está vacío
    if (server.hasArg("serial") && server.arg("serial") != "")
      preferences.putInt("serial", server.arg("serial").toInt());
    if (server.hasArg("ssid") && server.arg("ssid") != "")
      preferences.putString("ssid", server.arg("ssid"));
    if (server.hasArg("password") && server.arg("password") != "")
      preferences.putString("password", server.arg("password"));
    if (server.hasArg("conexion") && server.arg("conexion") != "")
      preferences.putString("conexion", server.arg("conexion"));
    if (server.hasArg("factor") && server.arg("factor") != "")
      preferences.putInt("factor", server.arg("factor").toInt());
    if (server.hasArg("sensor") && server.arg("sensor") != "")
      preferences.putString("sensor", server.arg("sensor"));
    
    // Detectores
    if (server.hasArg("detector1"))
      preferences.putString("detector1", server.arg("detector1"));
    if (server.hasArg("detector2"))
      preferences.putString("detector2", server.arg("detector2"));
    if (server.hasArg("detector3"))
      preferences.putString("detector3", server.arg("detector3"));
    if (server.hasArg("detector4"))
      preferences.putString("detector4", server.arg("detector4"));
    
    // ADCs
    if (server.hasArg("adc1"))
      preferences.putString("adc1", server.arg("adc1"));
    if (server.hasArg("adc2"))
      preferences.putString("adc2", server.arg("adc2"));
    if (server.hasArg("adc3"))
      preferences.putString("adc3", server.arg("adc3"));
    if (server.hasArg("adc4"))
      preferences.putString("adc4", server.arg("adc4"));
    
    preferences.end();
  
    // Leer y asignar a las variables globales
    loadConfiguration();
    printConfiguration();
  
    server.send(200, "text/html", "<h3>Configuración guardada. Reinicie el ESP32 para aplicar los cambios.</h3>");
  }



#endif //HTTPPAGE_HPP