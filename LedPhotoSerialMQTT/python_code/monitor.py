import paho.mqtt.client as mqtt
import time
import random

# --- Настройки ---
MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
CLIENT_ID = f"iot-project-monitor-{random.randint(0, 1000)}"

TOPIC_WILDCARD = "iot/project/#"


def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("Монитор успешно подключился к MQTT-брокеру.")
        client.subscribe(TOPIC_WILDCARD, qos=1)
        print(f"Подписка на все топики: {TOPIC_WILDCARD}")
    else:
        print(f"Не удалось подключиться, код возврата: {rc}")


def on_message(client, userdata, msg):
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
    payload = msg.payload.decode("utf-8")
    print(f"[{timestamp}] Topic: {msg.topic} | Payload: {payload}")


def main():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=CLIENT_ID)
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        print("Запуск монитора...")
        client.loop_forever()
    except KeyboardInterrupt:
        print("\nМонитор остановлен.")
    finally:
        client.disconnect()
        print("MQTT-клиент монитора отключен.")


if __name__ == "__main__":
    main()
