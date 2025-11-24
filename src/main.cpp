// #include <Arduino.h>
#include "mbedtls/sha256.h"
#include <Arduino.h>
#include "stdlib.h"
#include "string.h"
#include <WiFi.h>
#include <PubSubClient.h>

//#define DEBUG

const char* ssid = "Minh Nhat";
const char* password = "14231009";

// MQTT Broker
const char* MQTT_SERVER = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;

WiFiClient espClient;
PubSubClient client(espClient); 

void connectWiFi() {
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
}

void connectMQTT() {
  while (!client.connected()) {
    
    String clientId = "ESP32-Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.println(" try again in 2s");
      delay(2000);
    }
  }
}

////////// DEFINE O DAY NE AE //////////////

uint8_t output36 [44];
void sha2(float soil, float temp, float hum) {
   uint8_t input[12];

  memcpy(input + 0, &soil, 4);
  memcpy(input + 4, &temp, 4);
  memcpy(input + 8, &hum, 4);
  //memcpy(, hash32, 32);
  uint8_t output32 [32];

  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts_ret(&ctx, 0); // 0 for SHA-256, 1 for SHA-224
  mbedtls_sha256_update_ret(&ctx, input, sizeof(input));
  mbedtls_sha256_finish_ret(&ctx, output32);

  memcpy(output36, input, 12);
  memcpy(output36 + 12, output32, 32);

#ifdef DEBUG
  Serial.print("soil: ");
  Serial.print(soil, 2);
  Serial.print(" ");

  Serial.print("temp: ");
  Serial.print(temp, 2);
  Serial.print(" ");

  Serial.print("hum: ");
  Serial.print(hum, 2);
  Serial.print(" ");

  Serial.println("");

  for (int i = 0; i < 44; i++) {
    Serial.printf("%02X", output36[i]);
  }
  Serial.print("\n");
  #endif
  delay(4000);
}

//[0…11]   → 12 byte = 3 float (soil, temp, hum) liên tiếp (mỗi float 4 byte)
//[12…43]  → 32 byte = SHA-256 của 12 byte trên

void setup() {
  Serial.begin(115200);
  delay(100);
  connectWiFi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
  // cryto part
  float soil = random(1000, 9999) / 100.0;
  float temp = random(1000, 9999) / 100.0;
  float hum = random(1000, 9999) / 100.0;

  sha2(soil, temp, hum);

  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 1000) {
    lastMsg = millis();
    client.publish("esp32/test", output36, 44);
  }
}



