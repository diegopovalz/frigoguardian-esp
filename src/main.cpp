#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>

#define DHTPIN 4
#define DHTTYPE DHT11

#define RED_PIN         25
#define GREEN_PIN       26
#define BLUE_PIN        27

#define SS_PIN 21    // Define the SDA pin connected to GPIO21
#define RST_PIN 16   // Define the RST pin connected to GPIO16

// WiFi
const char* ssid = "iPhone de Poveda";
const char* password = "poveda123.";

// Ubidots MQTT Broker
const char* UBIDOTS_SERVER = "industrial.api.ubidots.com";
const int UBIDOTS_PORT = 1883;
const char* UBIDOTS_TOKEN = "BBUS-I9UF1izg1Kq8ZJ1QTW6OvJDC6jTTck";
const char* topic = "/v1.6/devices/frigo-guardian/temperatura";

DHT dht(DHTPIN, DHTTYPE);
WiFiClient wifi_client;
PubSubClient ubidots_client(wifi_client);
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance

void setLEDColor(float temperature) {
  if (temperature <= 0) {
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(BLUE_PIN, HIGH);   // Blue for cold
  } else if (temperature > 0 && temperature < 10) {
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, LOW);    // Green for moderate
  } else {
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(BLUE_PIN, LOW);    // Red for hot
  }
}

void mqttConnect() {
  // Loop until we're reconnected
  while (!ubidots_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (ubidots_client.connect("UbidotsFrigoGuardian", UBIDOTS_TOKEN, "")) {
      Serial.println("OK");
    } 
    else {     
      // Retry connection
      Serial.print("failed, rc=");
      Serial.print(ubidots_client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);    
    }
  }
}

void setup_wifi() {
  Serial.print("\nConnecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password); // Connect to network

  while (WiFi.status() != WL_CONNECTED) { // Wait for connection
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  dht.begin();
  randomSeed(analogRead(0));

  
  setup_wifi();

  // MQTT setup
  ubidots_client.setServer(UBIDOTS_SERVER, UBIDOTS_PORT);

  SPI.begin();      // Initiate SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
}

void loop() {
  if (!ubidots_client.connected()) {
    mqttConnect();
  }
  ubidots_client.loop();

  // Wait a few seconds between measurements.
  delay(5000);
  
  char temp_payload[100];

  float t = dht.readTemperature();
  if (isnan(t)) {
    Serial.println("Failed to read temperature from DHT sensor!");
    int randomInt = random(0, 10001);

    // Map the integer to the desired float range
    t = (randomInt / 10000.0) * (20.0 - (-5.0)) + (-5.0);
  }
  
  setLEDColor(t);
  
  snprintf(temp_payload, sizeof(temp_payload), "{\"value\": %.2f}", t);
  ubidots_client.publish(topic, temp_payload);

  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print("UID tag: ");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
}
