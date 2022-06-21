
#include "setup.h"
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h> //https://github.com/marvinroger/async-mqtt-client
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

// Variables conf gyrophare
char message_buff[500];
String power = "off" ;
int rouge = 0 ;
int vert = 0 ;
int bleu = 0 ;
int delai = 100 ;
int line = 4 ;

// LED strip connecté sur D4
#define LED_PIN1 D4
// Nombre de LEDS
#define NUM_LEDS 48
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN1, NEO_GRB + NEO_KHZ800);

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  uint16_t packetIdSub = mqttClient.subscribe(MQTT_TOPIC, 1);
  Serial.print("Subscribing at QoS 1, packetId: ");
  Serial.println(packetIdSub);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, len);
  int i; for (i = 0; i < len; i++) message_buff[i] = payload[i]; message_buff[i] = '\0';
  String msgString = String(message_buff);   
  if (strcmp(topic, MQTT_TOPIC) == 0) {                                                    
    String p = doc["power"];
    int r = doc["rouge"];
    int v = doc["vert"];
    int b = doc["bleu"];
    int d = doc["delai"];
    int l = doc["ligne"];
    if ( r ) {rouge = r; }
    if ( v ) {vert = v;  }
    if ( b ) {bleu = b;  }
    if ( d ) {delai = d; }
    if ( l ) {line = l; }
    //if ( !l ) {line = 4; }
    if ( p != "null" ) {power = p; }
    Serial.print("power :");
    Serial.println(p);
    Serial.print("lignes :");
    Serial.println(line);
    Serial.print("Délais :");
    Serial.println(d);
    Serial.print("rouge :");
    Serial.println(r);
    Serial.print("vert :");
    Serial.println(v);
    Serial.print("bleu :");
    Serial.println(b);
  }
  doc.clear();
}

void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void girophare(uint8_t r, uint8_t g, uint8_t b, uint8_t wait, uint8_t ligne) {
  for (uint16_t i = 0; i < 12; i++) {
    strip.clear();
    strip.setPixelColor(i+36, r, g, b);
    if ( ligne > 1 ) {
      strip.setPixelColor(i+24, r, g, b);
    }
    if ( ligne > 2 ) {
      strip.setPixelColor(i+12, r, g, b);
    }
    if ( ligne > 3 ) {
       strip.setPixelColor(i, r, g, b);
    }
    strip.show();
    delay(wait);
  }
}


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USER, MQTT_PWD);
  connectToWifi();

  strip.begin(); //start neopixels
  strip.show();
}

void loop() {
  if ( power == "on" ) {
    girophare(rouge, vert, bleu, delai, line); 
  } else { 
    strip.clear();
    strip.show();
  }
}
