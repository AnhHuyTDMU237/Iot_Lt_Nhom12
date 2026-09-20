import os
import time
import json
import random
import datetime
import paho.mqtt.client as mqtt

BROKER = os.getenv("MQTT_BROKER", "localhost")
PORT = int(os.getenv("MQTT_PORT", 1883))
NUM_DEVICES = int(os.getenv("NUM_DEVICES", 1))

class Device:
    def __init__(self, device_id):
        self.device_id = device_id
        self.led_state = False
        self.client = mqtt.Client(client_id=f"simulator-{self.device_id}")
        
        # Last Will and Testament
        lwm_topic = f"device/{self.device_id}/status"
        lwm_payload = json.dumps({
            "deviceId": self.device_id,
            "status": "OFFLINE",
            "timestamp": datetime.datetime.now(datetime.timezone.utc).isoformat()
        })
        self.client.will_set(lwm_topic, lwm_payload, qos=1, retain=True)

        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        
    def connect(self):
        while True:
            try:
                print(f"[{self.device_id}] Connecting to {BROKER}:{PORT}...")
                self.client.connect(BROKER, PORT, 60)
                self.client.loop_start()
                break
            except Exception as e:
                print(f"[{self.device_id}] Connection failed: {e}. Retrying in 5 seconds...")
                time.sleep(5)

    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print(f"[{self.device_id}] Connected successfully.")
            # Publish ONLINE status
            status_topic = f"device/{self.device_id}/status"
            status_payload = json.dumps({
                "deviceId": self.device_id,
                "status": "ONLINE",
                "timestamp": datetime.datetime.now(datetime.timezone.utc).isoformat()
            })
            self.client.publish(status_topic, status_payload, qos=1, retain=True)
            
            # Subscribe to command
            command_topic = f"device/{self.device_id}/command"
            self.client.subscribe(command_topic, qos=1)
            print(f"[{self.device_id}] Subscribed to {command_topic}")
        else:
            print(f"[{self.device_id}] Connection failed with code {rc}")

    def on_message(self, client, userdata, msg):
        topic = msg.topic
        payload = msg.payload.decode('utf-8')
        print(f"[{self.device_id}] Received message on {topic}: {payload}")
        
        try:
            data = json.loads(payload)
            command_id = data.get("commandId")
            action = data.get("action")
            
            if action == "LED_ON":
                self.led_state = True
            elif action == "LED_OFF":
                self.led_state = False
                
            # Publish ACK
            ack_topic = f"device/{self.device_id}/command/ack"
            ack_payload = json.dumps({
                "commandId": command_id,
                "deviceId": self.device_id,
                "action": action,
                "status": "ACKNOWLEDGED",
                "led": self.led_state,
                "timestamp": datetime.datetime.now(datetime.timezone.utc).isoformat()
            })
            self.client.publish(ack_topic, ack_payload, qos=1)
            print(f"[{self.device_id}] Sent ACK for command {command_id}")
            
        except Exception as e:
            print(f"[{self.device_id}] Error processing message: {e}")

    def publish_telemetry(self):
        temp = round(random.uniform(25.0, 35.0), 1)
        hum = round(random.uniform(50.0, 80.0), 1)
        ill = round(random.uniform(200.0, 800.0), 1)
        soil = round(random.uniform(40.0, 90.0), 1)
        
        telemetry_topic = f"device/{self.device_id}/telemetry"
        telemetry_payload = json.dumps({
            "deviceId": self.device_id,
            "temperature": temp,
            "humidity": hum,
            "illuminance": ill,
            "soilMoisture": soil,
            "led": self.led_state,
            "timestamp": datetime.datetime.now(datetime.timezone.utc).isoformat()
        })
        self.client.publish(telemetry_topic, telemetry_payload, qos=0)
        print(f"[{self.device_id}] Published telemetry: T={temp}C, H={hum}%, L={ill}lx, S={soil}%")

if __name__ == "__main__":
    devices = []
    for i in range(1, NUM_DEVICES + 1):
        device_id = f"esp32-{i:03d}"
        device = Device(device_id)
        device.connect()
        devices.append(device)
        
    try:
        while True:
            time.sleep(5)
            for device in devices:
                device.publish_telemetry()
    except KeyboardInterrupt:
        print("Shutting down...")
        for device in devices:
            device.client.loop_stop()
            device.client.disconnect()
