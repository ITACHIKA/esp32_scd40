import paho.mqtt.client as mqtt
import sqlite3
from datetime import datetime

conn = sqlite3.connect("mqtt_messages.db")
cursor = conn.cursor()

cursor.execute("""
CREATE TABLE IF NOT EXISTS messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    topic TEXT,
    content TEXT,
    timestamp TEXT
)
""")
conn.commit()

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.subscribe("sensor/scd40/#")

def on_message(client, userdata, msg):
    content = msg.payload.decode()
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{timestamp}] Topic: {msg.topic}, Message: {content}")

    cursor.execute("INSERT INTO messages (topic, content, timestamp) VALUES (?, ?, ?)",
                   (msg.topic, content, timestamp))
    conn.commit()

def main():
    client = mqtt.Client()

    client.on_connect = on_connect
    client.on_message = on_message

    client.connect("localhost", 1883, 60)

    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print("Exiting...")
        conn.close()

if __name__ == "__main__":
    main()