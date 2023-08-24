#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WifiClientSecure.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include "secrets.h"
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <time.h>
#define DHTPIN 15    // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11

#define COLUMS 16
#define ROWS   2

#define PAGE   ((COLUMS) * (ROWS))

LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);

#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"
 
float h ;
float t;
unsigned long timestamp;
DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);
void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}

void connectAWS()
{
   // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
 
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);
 
  // Create a message handler
  client.setCallback(messageHandler);
 
  Serial.println("Connecting to AWS IOT");
  lcd.clear();
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print("AWS IOT");
  delay(1000);

  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    lcd.clear();
    lcd.print(".");
    delay(1000);
  }
 
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }
 
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println("AWS IoT Connected!");
  lcd.clear();
  lcd.print(F("AWS IoT"));
  lcd.setCursor(0, 1);
  lcd.print(F(" Connected!"));
  delay(1000);
}


void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["humedad"] = round(h);
  doc["temperatura"] = round(t * 10) / 10;
  doc["timestamps"]=timestamp;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
 
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}
 

void loopConecct() {
  h = dht.readHumidity();
  t = dht.readTemperature();
  timestamp = millis() / 1000;
 
  if (isnan(h) || isnan(t) )  // Check if any reads failed and exit early (to try again).
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
 
  Serial.print(F("Humedad: "));
  Serial.print(h);
  Serial.print(F("%  Temperatura: "));
  Serial.print(t);
  Serial.println(F("°C "));
 
 
  lcd.clear();
  lcd.print(F("Temperatura:"));
  lcd.print(round(t * 10) / 10);
  lcd.print(F("°C "));
  lcd.setCursor(0, 1);
  lcd.print("Humedad:");
  lcd.print(round(h));
  lcd.print(F("%"));


  publishMessage();
  client.loop();
  delay(5000);
}

WebServer server(80);

void handleRoot() {
  String html = "<html><body>";
  html += "<form method='POST' action='/wifi'>";
  html += "Red Wi-Fi: <input type='text' name='ssid'><br>";
  html += "Contraseña: <input type='password' name='password'><br>";
  html += "<input type='submit' value='Conectar'>";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleWifi() {
  String ssid =server.arg("ssid");
  String password = server.arg("password");
  Serial.print("Conectando a la red Wi-Fi ");
  Serial.println(ssid);
  Serial.print("Clave Wi-Fi ");
  Serial.println(password);
  Serial.print("...");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Red Wi-Fi ");
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  delay(3000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Clave Wi-Fi ");
  lcd.setCursor(0, 1);
  lcd.print("**********");
  delay(3000);
  lcd.clear();



  WiFi.disconnect(); // Desconectar la red Wi-Fi anterior, si se estaba conectado
  WiFi.begin(ssid.c_str(), password.c_str(),6);
  int cnt = 0;
  while (WiFi.status() != WL_CONNECTED and cnt < 8) {
    delay(1000);
    Serial.print(".");
    cnt++;
  }
  
if (WiFi.status() == WL_CONNECTED)
  {
  Serial.println("Conexión establecida");
  server.send(200, "text/plain", "Conexión establecida");
  connectAWS();
  }
else{
  Serial.println("Conexión no establecida");
  server.send(200, "text/plain", "Conexión no establecida");
  }
  
}



void initAP(const char* apSsid,const char* apPassword) { // Nombre de la red Wi-Fi y  Contraseña creada por el ESP32
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid, apPassword);

  server.on("/", handleRoot);
  server.on("/wifi", handleWifi);

  server.begin();
  Serial.print("Ip de esp32...");
  Serial.println(WiFi.softAPIP());
  //Serial.println(WiFi.localIP());
  Serial.println("Servidor web iniciado");
  
  while (lcd.begin(COLUMS, ROWS) != 1) //colums - 20, rows - 4
  {
    Serial.println(F("PCF8574 is not connected or lcd pins declaration is wrong. Only pins numbers: 4,5,6,16,11,12,13,14 are legal."));
    delay(5000);   
  }
  lcd.print(F("PCF8574 is OK..."));    //(F()) saves string to flash & keeps dynamic memory free
  delay(2000);
  lcd.clear();
  lcd.print(F("Ip de esp32..."));
  lcd.setCursor(0, 1);
  lcd.print(WiFi.softAPIP());
  delay(2000);
  lcd.clear();
  dht.begin();
}

void loopAP() {

    server.handleClient();
    if (WiFi.status() == WL_CONNECTED)
    {
    loopConecct();
    }
    
    
}









