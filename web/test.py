# mqtt_subscriber.py
import paho.mqtt.client as mqtt
import struct
import hashlib

BROKER = "broker.hivemq.com"   # hoặc "broker.hivemq.com"
PORT = 1883
TOPIC = "esp32/test"

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

    if len(payload) < 4:
        print("Payload too short, skipping unpack.")
        return

    b4 = payload[:4]
    global value 
    value = struct.unpack('<f', b4)[0]
   
    if len(payload) >= 36:
        recv_hash = payload[4:36]
        calc_hash = hashlib.sha256(b4).digest()
        if (calc_hash == recv_hash):
            print("recv:", value)
        else:
            print("Hash mismatch!")
    else:
        print("No hash provided")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

def start_mqtt():
    client.connect(BROKER, PORT, 60)
    client.loop_forever()
