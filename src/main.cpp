#include <Arduino.h>
#include "mbedtls/sha256.h"
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Minh Nhat";
const char* password = "14231009";

const char* MQTT_SERVER = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// 3 BIEN NAY DUNG DE DIEU KHIEN PUMP //
uint8_t pump_control = 0; // ON - OFF
uint32_t duration = 0; // THOI GIAN HOAT DONG
uint8_t mode = 0; // CHE DO LAM VIEC AUTO - MANUAL

// =====================================================
// CONNECT WIFI
// =====================================================
void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(400);
  }
  Serial.println("\nWiFi connected!");
}

// =====================================================
// CONNECT MQTT
// =====================================================
void connectMQTT() {
  while (!client.connected()) {
    Serial.print("MQTT connecting...");
    String id = "ESP32-" + String(random(0xffff), HEX);

    if (client.connect(id.c_str())) {
      Serial.println("OK");

      client.subscribe("esp32/command");   // <<==== SUBSCRIBE BINARY COMMAND

    } else {
      Serial.println("Retry 2s");
      delay(2000);
    }
  }
}

// =====================================================
// CALLBACK – RX BINARY COMMAND
// =====================================================

void callback(char* topic, byte* payload, unsigned int len) {
  Serial.println("=== BINARY COMMAND RECEIVED ===");

  if (len < 38) {
    Serial.println("ERROR: packet too small");
    return;
  }
    pump_control  = payload[0];
    duration = *(uint32_t*)(payload + 1);
    mode    = payload[5];


  uint8_t hash_recv[32];
  memcpy(hash_recv, payload + 6, 32);

  // === Recompute SHA256 ===
  uint8_t check_input[6];
  memcpy(check_input, payload, 6);

  uint8_t hash_calc[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts_ret(&ctx, 0);
  mbedtls_sha256_update_ret(&ctx, check_input, 6);
  mbedtls_sha256_finish_ret(&ctx, hash_calc);

  if (memcmp(hash_calc, hash_recv, 32) != 0) {
    Serial.println("HASH FAILED !!!");
    return;
  }

  Serial.println("HASH OK");
  Serial.print("cmd_id = ");     Serial.println(pump_control);
  Serial.print("duration_ms = "); Serial.println(duration);
  Serial.print("mode = ");       Serial.println(mode);

  // === HANDLE COMMAND ===
  if (pump_control == 1) {
    Serial.println("PUMP START");
  } else if (pump_control == 2) {
    Serial.println("PUMP STOP");
  }

  if (duration > 0) {
    Serial.print("Will run for ");
    Serial.print(duration);
    Serial.println(" ms");
  }
}

// =====================================================
// SENSOR PACK (unchanged)
// =====================================================

uint8_t output36[48];

void sha2(float soil, float temp, float hum, float flow) {
  uint8_t input[16];

  memcpy(input + 0, &soil, 4);
  memcpy(input + 4, &temp, 4);
  memcpy(input + 8, &hum, 4);
  memcpy(input + 12, &flow, 4);

  uint8_t out32[32];

  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts_ret(&ctx, 0);
  mbedtls_sha256_update_ret(&ctx, input, 16);
  mbedtls_sha256_finish_ret(&ctx, out32);

  memcpy(output36, input, 16);
  memcpy(output36 + 16, out32, 32);
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  connectWiFi();

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  if (!client.connected()) connectMQTT();
  client.loop();

  float soil = random(1000, 9999) / 100.0;
  float temp = random(1000, 9999) / 100.0;
  float hum  = random(1000, 9999) / 100.0;
  float flow = random(1000, 9999) / 100.0;

  sha2(soil, temp, hum, flow);

  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 2000) {
    lastMsg = millis();
    client.publish("esp32/test", output36, 48);
  }
}
