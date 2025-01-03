#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_INA219.h>
#include "DHT.h"

// WiFi credentials
const char* ssid = "Bbox-5D58DA16";
const char* password = "mirage2000";

// MQTT credentials
const char* mqtt_server = "192.168.1.62";
const char* mqtt_user = "Teleinfomqtt";
const char* mqtt_password = "mqtt42";

// GPIO definitions
#define PIN_LETTER 27
#define PIN_PARCEL 15
#define PIN_PIR 4
#define PIN_VBAT 33
#define DHT_PIN 13
#define DHT_TYPE DHT22

// INA219 addresses
Adafruit_INA219 inaSolar(0x40);
Adafruit_INA219 inaBattery(0x45);

// DHT sensor
DHT dht(DHT_PIN, DHT_TYPE);

// WiFi and MQTT clients
WiFiClient espClient;
PubSubClient client(espClient);

// Variables
bool letterDetected = false;
bool parcelDetected = false;
bool pirDetected = false;
bool pirTriggered = false;
bool pirImpulseCompleted = true;
unsigned long pirLastDetectedTime = 0;
const unsigned long pirDebounceTime = 2000; // 2 seconds debounce time

void setupWiFi() {
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("Connected to MQTT");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void publishMQTT(const String& topic, const String& payload) {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.publish(topic.c_str(), payload.c_str());
  Serial.print("Published topic: ");
  Serial.print(topic);
  Serial.print(" with payload: ");
  Serial.println(payload);
}

void publishAllData(const String& prefix) {
  // Publish letter, parcel, and PIR events as separate topics
  if (prefix == "letter") {
    publishMQTT("mailbox/letter", letterDetected ? "1" : "0");
  } else if (prefix == "parcel") {
    publishMQTT("mailbox/parcel", parcelDetected ? "1" : "0");
  } else if (prefix == "PIR") {
    publishMQTT("mailbox/PIR", pirTriggered ? "1" : "0");
  }

  // Publish environmental and power data as separate topics
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  if (!isnan(temp)) {
    publishMQTT("mailbox/temp", String(temp));
  }
  if (!isnan(hum)) {
    publishMQTT("mailbox/hum", String(hum));
  }

  float solarCurrent = inaSolar.getCurrent_mA();
  float batteryCurrent = inaBattery.getCurrent_mA();
  publishMQTT("mailbox/icsolaire", String(solarCurrent));
  publishMQTT("mailbox/icbatterie", String(batteryCurrent));

  int vbatRaw = analogRead(PIN_VBAT);
  float vbat = (vbatRaw / 4095.0) * 3.3 * 2; // Voltage divider assumed
  publishMQTT("mailbox/vbat", String(vbat));

  // Publish RSSI and IP address
  int rssi = WiFi.RSSI();
  String ipAddress = WiFi.localIP().toString();
  publishMQTT("mailbox/rssi", String(rssi));
  publishMQTT("mailbox/ip", ipAddress);
}

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);

  // Setup GPIOs
  pinMode(PIN_LETTER, INPUT);
  pinMode(PIN_PARCEL, INPUT);
  pinMode(PIN_PIR, INPUT);

  // Initialize sensors
  dht.begin();
  if (!inaSolar.begin() || !inaBattery.begin()) {
    Serial.println("Failed to initialize INA219 sensors!");
    while (1);
  }

  // Setup WiFi and MQTT
  setupWiFi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  // Read GPIO states
  bool newLetterDetected = digitalRead(PIN_LETTER) == HIGH;
  bool newParcelDetected = digitalRead(PIN_PARCEL) == HIGH;
  bool newPirDetected = digitalRead(PIN_PIR) == HIGH;

  // Debugging outputs for GPIO states
  Serial.println("--- GPIO Status ---");
  Serial.print("PIN_LETTER state: ");
  Serial.println(newLetterDetected);
  Serial.print("PIN_PARCEL state: ");
  Serial.println(newParcelDetected);
  Serial.print("PIN_PIR state: ");
  Serial.println(newPirDetected);

  if (newLetterDetected != letterDetected) {
    letterDetected = newLetterDetected;
    Serial.println("Letter state changed!");
    publishAllData("letter");
  }

  if (newParcelDetected != parcelDetected) {
    parcelDetected = newParcelDetected;
    Serial.println("Parcel state changed!");
    publishAllData("parcel");
  }

  // Handle PIR with debounce and impulse logic
  if (newPirDetected && pirImpulseCompleted && (millis() - pirLastDetectedTime > pirDebounceTime)) {
    pirTriggered = true;
    pirImpulseCompleted = false;
    pirLastDetectedTime = millis();
    Serial.println("PIR detected with impulse!");
    publishAllData("PIR");
  }

  if (!newPirDetected && pirTriggered) {
    pirTriggered = false;
    pirImpulseCompleted = true;
    Serial.println("PIR reset.");
  }

  // Publish environmental and power data every 10 minutes
  static unsigned long lastPublishTime = 0;
  if (millis() - lastPublishTime > 600000) {
    lastPublishTime = millis();
    publishAllData("periodic");
  }

  // Enter deep sleep if no activity
  Serial.println("Entering deep sleep...");

  // Configure wakeup using ext1 for multiple GPIOs
  esp_sleep_enable_ext1_wakeup((1ULL << PIN_LETTER) | (1ULL << PIN_PARCEL) | (1ULL << PIN_PIR), ESP_EXT1_WAKEUP_ANY_HIGH);

  esp_deep_sleep_start();
}