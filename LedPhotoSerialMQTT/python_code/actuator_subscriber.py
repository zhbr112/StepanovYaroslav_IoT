import serial
import time
import paho.mqtt.client as mqtt
import random
import argparse

# --- Настройки ---
SERIAL_BAUD_RATE = 9600
MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
CLIENT_ID = f"iot-project-actuator-sub-{random.randint(0, 1000)}"

# Топики для подписки и публикации
SENSOR_DATA_TOPIC = "iot/project/sensor/data"
ACTUATOR_STATUS_TOPIC = "iot/project/actuator/status"

# Пороговое значение освещенности. Если значение с датчика НИЖЕ этого порога, включаем свет.
LUMINOSITY_THRESHOLD = 35

# Глобальные переменные
ser = None
current_led_state = "UNKNOWN"


def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("Успешное подключение к MQTT-брокеру.")
        client.subscribe(SENSOR_DATA_TOPIC, qos=1)
        print(f"Подписка на топик: {SENSOR_DATA_TOPIC}")
        client.publish(
            ACTUATOR_STATUS_TOPIC, "Subscriber connected", qos=1, retain=True
        )
    else:
        print(f"Не удалось подключиться, код возврата: {rc}")


def on_message(client, userdata, msg):
    global current_led_state
    try:
        payload = msg.payload.decode("utf-8")
        print(f"Получено сообщение из топика '{msg.topic}': {payload}")

        luminosity = int(payload)

        target_state = "ON" if luminosity < LUMINOSITY_THRESHOLD else "OFF"

        # Отправляем команду, только если состояние должно измениться
        if target_state != current_led_state:
            if target_state == "ON":
                command = b"u"  # Команда "up" - включить
                print(
                    f"Освещенность ({luminosity}) ниже порога ({LUMINOSITY_THRESHOLD}). Включаем LED."
                )
            else:
                command = b"d"  # Команда "down" - выключить
                print(
                    f"Освещенность ({luminosity}) выше порога ({LUMINOSITY_THRESHOLD}). Выключаем LED."
                )

            if ser and ser.is_open:
                ser.write(command)
                # Ожидаем и читаем ответ от MCU
                response = ser.readline().decode("utf-8").strip()
                print(f"Ответ от MCU: {response}")

                # Обновляем и публикуем новое состояние
                current_led_state = target_state
                client.publish(
                    ACTUATOR_STATUS_TOPIC,
                    f"LED is now {current_led_state}",
                    qos=1,
                    retain=True,
                )
            else:
                print("Ошибка: последовательный порт не доступен.")
        else:
            print(
                f"Состояние LED уже соответствует требуемому ('{current_led_state}'). Команда не отправляется."
            )

    except (ValueError, UnicodeDecodeError) as e:
        print(f"Ошибка обработки сообщения: {e}")


def main():
    global ser

    # --- Парсер аргументов ---
    parser = argparse.ArgumentParser(description="Запускает MQTT-сабскрайбер для Actuator MCU.")
    parser.add_argument('port', help="Последовательный порт для подключения к Actuator MCU (например, COM4 или /dev/ttyUSB1)")
    args = parser.parse_args()

    # --- Настройка MQTT клиента ---
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=CLIENT_ID)
    client.on_connect = on_connect
    client.on_message = on_message
    client.will_set(
        ACTUATOR_STATUS_TOPIC,
        "Subscriber disconnected unexpectedly",
        qos=1,
        retain=True,
    )
    client.connect(MQTT_BROKER, MQTT_PORT, 60)

    try:
        # --- Настройка Serial соединения ---
        print(f"Попытка подключения к {args.port}...")
        ser = serial.Serial(args.port, SERIAL_BAUD_RATE, timeout=1)
        time.sleep(2)
        print("Успешное подключение к Actuator MCU.")
        client.publish(
            ACTUATOR_STATUS_TOPIC, "Connected to Actuator MCU", qos=1, retain=True
        )

        # --- Запуск MQTT клиента в блокирующем режиме ---
        client.loop_forever()

    except serial.SerialException as e:
        print(f"Ошибка: Не удалось подключиться к порту {args.port}. {e}")
        print("Убедитесь, что микроконтроллер подключен и выбран правильный порт.")
    except KeyboardInterrupt:
        print("\nПрограмма завершена пользователем.")
    finally:
        if ser and ser.is_open:
            ser.close()
            print("Последовательный порт закрыт.")
        client.publish(
            ACTUATOR_STATUS_TOPIC, "Subscriber disconnected", qos=1, retain=True
        )
        time.sleep(1)
        client.disconnect()
        print("MQTT-клиент отключен.")


if __name__ == "__main__":
    main()
