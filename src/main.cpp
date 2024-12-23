// Code MailBox by Franck le 22/12/24
// mode sleep, puis reveille sur evenement parcel ou letter + plusblish mqtt
// publish telemetrie vbat, icsolaire, icbatterie , temp, hum toutes les 10 minutes
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_INA219.h>
#include <DHT.h>

// WiFi parameters
const char* ssid = "Bbox-5D58DA16";
const char* password = "mirage2000";

// MQTT parameters
const char* mqtt_server = "192.168.1.62";
const char* mqtt_user = "Teleinfomqtt";
const char* mqtt_password = "mqtt42";

WiFiClient espClient;
PubSubClient client(espClient);

// Pin definitions
#define SWITCH_LETTER 27
#define SWITCH_PARCEL 15
#define VBAT_PIN 33
#define DHT_PIN 13
#define DHT_TYPE DHT22
#define SCL_PIN 22
#define SDA_PIN 21

// INA219 instances
Adafruit_INA219 ina219_solar(0x40);
Adafruit_INA219 ina219_battery(0x45);

// DHT sensor instance
DHT dht(DHT_PIN, DHT_TYPE);

// Deep sleep parameters pour le reveil de l'ESP32
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 600 // 10 minutes
RTC_DATA_ATTR int bootCount = 0;

// Function prototypes
void setupWiFi();
void reconnectMQTT();
void publishData();
void deepSleepSetup();

void setup() {
  Serial.begin(115200);
  pinMode(SWITCH_LETTER, INPUT);
  pinMode(SWITCH_PARCEL, INPUT);

  // Initialize sensors
  if (!ina219_solar.begin()) {
    Serial.println("Failed to find INA219 (solar) chip");
  }
  if (!ina219_battery.begin()) {
    Serial.println("Failed to find INA219 (battery) chip");
  }
  dht.begin();

  // Setup WiFi and MQTT
  setupWiFi();
  client.setServer(mqtt_server, 1883);

  // Publish data and setup deep sleep
  publishData();
  deepSleepSetup();
}

void loop() {
  // Nothing to do here since ESP32 will enter deep sleep
}

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
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void publishData() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  // Read sensor data
  float vbat = analogRead(VBAT_PIN) * (3.3 / 4095.0) * 3.7; // Adjust as per resistor divider
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  float icsolaire = ina219_solar.getCurrent_mA();
  float icbatterie = ina219_battery.getCurrent_mA();

// Get WiFi information
  String ipAddress = WiFi.localIP().toString();
  int rssi = WiFi.RSSI();

  // Publish MQTT topics
  client.publish("mailbox/letter", digitalRead(SWITCH_LETTER) ? "1" : "0");
  client.publish("mailbox/parcel", digitalRead(SWITCH_PARCEL) ? "1" : "0");
  client.publish("mailbox/temperature", String(temp).c_str());
  client.publish("mailbox/humidity", String(hum).c_str());
  client.publish("mailbox/vbat", String(vbat).c_str());
  client.publish("mailbox/Icsolaire", String(icsolaire).c_str());
  client.publish("mailbox/Icbatterie", String(icbatterie).c_str());
  client.publish("mailbox/ip", ipAddress.c_str());
  client.publish("mailbox/rssi", String(rssi).c_str());
  Serial.println("Data published to MQTT");

}


void deepSleepSetup() {
  Serial.println("Setting up deep sleep...");
  delay(2000);

  // Configure les GPIO pour le réveil (niveau haut)
  esp_sleep_enable_ext1_wakeup(
    (1ULL << SWITCH_LETTER) | (1ULL << SWITCH_PARCEL), // GPIO 27 et GPIO 15
    ESP_EXT1_WAKEUP_ANY_HIGH // Réveil si l'un des deux passe à HIGH
  );

  // Configure le réveil par minuterie
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR); // 10 minutes

  Serial.println("Going to sleep...");
  esp_deep_sleep_start();
}
