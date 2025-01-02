#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_INA219.h>
#include <DHT.h>

// Déclaration des fonctions
void setupWiFi();
void reconnectMQTT();
void publishToSeparateTopics();
void deepSleepSetup();

// WiFi parameters
const char* ssid = "login";
const char* password = "password";

// MQTT parameters
const char* mqtt_server = "192.168.1.62";
const char* mqtt_user = "login";
const char* mqtt_password = "password";

WiFiClient espClient;
PubSubClient client(espClient);

// Pin definitions
#define SWITCH_LETTER 27
#define SWITCH_PARCEL 15
#define VBAT_PIN 33
#define DHT_PIN 13
#define DHT_TYPE DHT22
#define PIR_PIN 4

// Capteurs instances
Adafruit_INA219 ina219_solar(0x40);
Adafruit_INA219 ina219_battery(0x45);
DHT dht(DHT_PIN, DHT_TYPE);

// Deep sleep parameters
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 600
RTC_DATA_ATTR int bootCount = 0;

void setup() {
    Serial.begin(115200);
    pinMode(SWITCH_LETTER, INPUT);
    pinMode(SWITCH_PARCEL, INPUT);
    pinMode(PIR_PIN, INPUT);

    // Initialisation des capteurs
    if (!ina219_solar.begin()) {
        Serial.println("Failed to find INA219 (solar) chip");
    }
    if (!ina219_battery.begin()) {
        Serial.println("Failed to find INA219 (battery) chip");
    }
    dht.begin();

    // Configuration WiFi et MQTT
    setupWiFi();
    client.setServer(mqtt_server, 1883);

    // Publication des données
    publishToSeparateTopics();

    // Mise en veille profonde
    deepSleepSetup();
}

void loop() {
    // Rien à faire ici puisque l'ESP32 entrera en deep sleep
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

void publishToSeparateTopics() {
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop();

    // PIR logic
    static bool lastPirState = false;
    bool pirState = digitalRead(PIR_PIN);

    if (pirState != lastPirState) {
        lastPirState = pirState;
        if (pirState) {
            client.publish("mailbox/pir", "1");
            delay(1000); // Maintenir l'état "1" pendant 1 seconde
            client.publish("mailbox/pir", "0");
        }
        Serial.printf("PIR State: %d\n", pirState);
    }

    // Letter sensor logic
    static bool lastLetterState = false;
    bool letterState = digitalRead(SWITCH_LETTER);

    if (letterState != lastLetterState) {
        lastLetterState = letterState;
        client.publish("mailbox/letter", letterState ? "1" : "0");
        Serial.printf("Letter State: %d\n", letterState);
    }

    // Parcel sensor logic
    static bool lastParcelState = false;
    bool parcelState = digitalRead(SWITCH_PARCEL);

    if (parcelState != lastParcelState) {
        lastParcelState = parcelState;
        client.publish("mailbox/parcel", parcelState ? "1" : "0");
        Serial.printf("Parcel State: %d\n", parcelState);
    }

    // Logs des autres capteurs ici (température, humidité, etc.)
    Serial.println("Publishing other data...");

    // Lecture des autres capteurs
    float vbat = analogRead(VBAT_PIN) * (3.3 / 4095.0) * 3.7; // Ajustez selon votre diviseur
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    float icsolaire = ina219_solar.getCurrent_mA();
    float icbatterie = ina219_battery.getCurrent_mA();

    // Récupération des informations réseau
    String ipAddress = WiFi.localIP().toString();
    int rssi = WiFi.RSSI();

    // Publication des données sur des topics séparés
    client.publish("mailbox/temperature", String(temp).c_str());
    client.publish("mailbox/humidity", String(hum).c_str());
    client.publish("mailbox/vbat", String(vbat).c_str());
    client.publish("mailbox/Icsolaire", String(icsolaire).c_str());
    client.publish("mailbox/Icbatterie", String(icbatterie).c_str());
    client.publish("mailbox/ip", ipAddress.c_str());
    client.publish("mailbox/rssi", String(rssi).c_str());

    // Logs des publications
    Serial.printf("Temp: %.2f, Hum: %.2f, Vbat: %.2f, Icsolaire: %.2f, Icbatterie: %.2f\n",
                  temp, hum, vbat, icsolaire, icbatterie);
    Serial.printf("IP: %s, RSSI: %d\n", ipAddress.c_str(), rssi);
}

void deepSleepSetup() {
    Serial.println("Setting up deep sleep...");
    delay(4000);

    // Configure les GPIO pour le réveil (niveau haut)
    esp_sleep_enable_ext1_wakeup(
        (1ULL << SWITCH_LETTER) | (1ULL << SWITCH_PARCEL) | (1ULL << PIR_PIN),
        ESP_EXT1_WAKEUP_ANY_HIGH
    );

    // Configure le réveil par minuterie
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

    Serial.println("Going to sleep...");
    esp_deep_sleep_start();
}
