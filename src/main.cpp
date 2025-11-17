// #include <Arduino.h>
#include "mbedtls/sha256.h"
#include <Arduino.h>
#include "stdlib.h"
#include "string.h"

#include <WiFi.h>
#include <PubSubClient.h>

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


 
uint8_t output36 [36];
void sha2(float value) {
   uint8_t input[4];

  memcpy(input, &value, sizeof(value));
  //memcpy(, hash32, 32);
  uint8_t output32 [32];

  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts_ret(&ctx, 0); // 0 for SHA-256, 1 for SHA-224
  mbedtls_sha256_update_ret(&ctx, input, sizeof(input));
  mbedtls_sha256_finish_ret(&ctx, output32);

  memcpy(output36, input, 4);
  memcpy(output36 + 4, output32, 32);

  Serial.print("Value: ");
  Serial.println(value, 2);

  for (int i = 0; i < 36; i++) {
    Serial.printf("%02X", output36[i]);
  }
  Serial.print("\n");
  delay(4000);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(100);
  connectWiFi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  //randomSeed(micros());
}

void loop() {
  // cryto part
  float value = random(1000, 9999) / 100.0;
  

  sha2(value);

  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 1000) {
    lastMsg = millis();
    client.publish("esp32/test", output36, 36);
  }
}



