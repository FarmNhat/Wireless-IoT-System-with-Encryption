# mqtt_subscriber.py
import paho.mqtt.client as mqtt
import struct
import hashlib
import json
import time
import os

BROKER = "broker.hivemq.com"
PORT = 1883

TOPIC_SENSOR = "esp32/test"
TOPIC_COMMAND = "esp32/command"

DEVICE_ID = "esp32_001"
JSON_FILE = "conf.json"
CONF_FILE = "command.json"     # File chứa lệnh gửi xuống ESP32


# ============================================================
#  SAVE SENSOR JSON TO FILE
# ============================================================
def save_to_json(data):
    try:
        with open(JSON_FILE, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2)
    except Exception as e:
        print("Save JSON error:", e)


# ============================================================
#  SEND BINARY COMMAND TO ESP32
#  FORMAT = 12 byte (cmd_id, duration_ms, mode) + 32 byte SHA256
# ============================================================
def send_command_array(client, cmd_id, duration_ms, mode):
    # Tạo 12 byte đầu
    b = struct.pack('<BIB', cmd_id, duration_ms, mode)

    # SHA256 12 byte
    h = hashlib.sha256(b).digest()

    packet = b + h  # 44 byte
    # print(struct.pack('<B', cmd_id))
    # print(struct.pack('<I', duration_ms))
    # print(struct.pack('<B', mode))
    print("Sending COMMAND:", packet.hex())
    client.publish(TOPIC_COMMAND, packet)


# ============================================================
#  PROCESS conf.json COMMAND FILE
# ============================================================
def check_and_send_command(client):
    if not os.path.exists(CONF_FILE):
        return  # không có file → bỏ qua

    try:
        with open(CONF_FILE, "r", encoding="utf-8") as f:
            cmd = json.load(f)

        command = cmd.get("command", "")
        duration = int(cmd.get("duration", 0))
        mode = cmd.get("mode", "manual")

        # map command -> id
        command_map = {
            "pump_start": 1,
            "pump_stop": 2
        }

        cmd_id = command_map.get(command, 0)
        mode_id = 0 if mode == "manual" else 1

        # print("\n=== COMMAND LOADED FROM JSON ===")
        # print("cmd_id:", cmd_id)
        # print("duration:", duration)
        # print("mode:", mode_id)
        # print("================================")

        # Gửi binary array
        send_command_array(client, cmd_id, duration, mode_id)
        time.sleep(1)   

    except Exception as e:
        print("Read conf.json error:", e)


# ============================================================
#   MQTT CALLBACKS
# ============================================================
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("MQTT connected.")
        client.subscribe(TOPIC_SENSOR)
    else:
        print("Failed to connect, return code:", rc)


def on_message(client, userdata, msg):
    try:
        payload = msg.payload
        print(f"[{msg.topic}] {payload.hex()}")
    except Exception as e:
        print("Error decoding message:", e)
        return

    # Kiểm tra đủ 12 byte đầu
    if len(payload) < 16:
        print("Payload too short.")
        return

    # unpack sensor
    b4 = payload[:16]
    soil, temp, hum, float = struct.unpack('<ffff', b4)

    # Check SHA256
    if len(payload) >= 48:
        recv_hash = payload[16:48]
        calc_hash = hashlib.sha256(b4).digest()

        if calc_hash == recv_hash:
            print("soil:", soil, " temp:", temp, " hum:", hum, " float:", float)

            result = {
                "device_id": DEVICE_ID,
                "data": {
                    "soil": round(soil, 2),
                    "temp": round(temp, 2),
                    "hum": round(hum, 2),
                    "float": round(float, 2)
                },
                "hash": hashlib.sha256(b4).hexdigest()
            }

            save_to_json(result)

        else:
            print("Hash mismatch!")

    # Sau khi xử lý sensor → kiểm tra lệnh
    check_and_send_command(client)


# ============================================================
#   START MQTT
# ============================================================
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, PORT, 60)
client.loop_forever()
