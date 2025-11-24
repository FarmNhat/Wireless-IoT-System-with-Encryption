# mqtt_subscriber.py
import paho.mqtt.client as mqtt
import struct
import hashlib
import json

BROKER = "broker.hivemq.com"   # hoặc "broker.hivemq.com"
PORT = 1883
TOPIC = "esp32/test"
DEVICE_ID = "esp32_001"
JSON_FILE = "conf.json"        # File sẽ được ghi đè mỗi lần nhận dữ liệu mới

def save_to_json(data):
    try:
        with open(JSON_FILE, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2)
        #print(f"Saved to {JSON_FILE}")
    except Exception as e:
        print("Save JSON error:", e)


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        client.subscribe(TOPIC)
    else:
        print("Failed to connect, return code:", rc)

def on_message(client, userdata, msg):
    try:
        payload = msg.payload
        print(f"[{msg.topic}] {payload.hex()}")
    except Exception as e:
        print("Error decoding message:", e)

    if len(payload) < 12:
        print("Payload too short, skipping unpack.")
        return

    b4 = payload[:12]
    global value 
    soil, temp, hum = struct.unpack('<fff', b4)
   
    if len(payload) >= 44:
        recv_hash = payload[12:44]
        calc_hash = hashlib.sha256(b4).digest()
        if (calc_hash == recv_hash):
            print("soil: ", soil," temp: ", temp, " hum: ", hum)
            result = {
                "device_id": DEVICE_ID,
                "data": {
                    "soil": round(soil, 2),      
                    "temp": round(temp, 2),
                    "hum": round(hum, 2),
                },
                "hash": hashlib.sha256(b4).hexdigest()   
            }

            result = json.dumps(result, indent=2)
            save_to_json(result)
        else:
            print("Hash mismatch!")
    else:
        print("No hash provided")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

#def start_mqtt():
client.connect(BROKER, PORT, 60)
client.loop_forever()
