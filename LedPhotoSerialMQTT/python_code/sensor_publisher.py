import serial
import time
import paho.mqtt.client as mqtt
import threading
import random
import argparse

# --- Настройки ---
SERIAL_BAUD_RATE = 9600
MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
CLIENT_ID = f"iot-project-sensor-pub-{random.randint(0, 1000)}"

# Топики для публикации
SENSOR_DATA_TOPIC = "iot/project/sensor/data"
SENSOR_STATUS_TOPIC = "iot/project/sensor/status"

# Глобальная переменная для последовательного порта
ser = None


def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("Успешное подключение к MQTT-брокеру.")
        client.publish(SENSOR_STATUS_TOPIC, "Publisher connected", qos=1, retain=True)
    else:
        print(f"Не удалось подключиться, код возврата: {rc}")


def read_from_mcu(client: mqtt.Client):
    """Функция для чтения данных из Serial порта в отдельном потоке."""
    global ser
    while True:
        try:
            if ser and ser.in_waiting > 0:
                line = ser.readline().decode("utf-8").strip()
                if line.startswith("SENSOR_VALUE:"):
                    try:
                        value = line.split(":")[1]
                        print(f"Получено от MCU: {value}")
                        # Публикуем значение в MQTT
                        client.publish(SENSOR_DATA_TOPIC, payload=value, qos=1)
                    except (IndexError, ValueError) as e:
                        print(f"Ошибка парсинга данных от MCU: {line}, {e}")
                else:
                    # Публикуем служебные сообщения от MCU (например, "STREAM_STARTED")
                    client.publish(SENSOR_STATUS_TOPIC, f"MCU_MSG: {line}", qos=1)
                    print(f"Служебное сообщение от MCU: {line}")
        except serial.SerialException as e:
            print(f"Ошибка чтения с последовательного порта: {e}")
            break
        except Exception as e:
            print(f"Неизвестная ошибка в потоке чтения: {e}")
        time.sleep(0.1)


def main():
    global ser

    # --- Парсер аргументов ---
    parser = argparse.ArgumentParser(description="Запускает MQTT-паблишер для Sensor MCU.")
    parser.add_argument('port', help="Последовательный порт для подключения к Sensor MCU (например, COM3 или /dev/ttyUSB0)")
    args = parser.parse_args()

    # --- Настройка MQTT клиента ---
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=CLIENT_ID)
    client.on_connect = on_connect
    client.will_set(
        SENSOR_STATUS_TOPIC, "Publisher disconnected unexpectedly", qos=1, retain=True
    )
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start()

    try:
        # --- Настройка Serial соединения ---
        print(f"Попытка подключения к {args.port}...")
        ser = serial.Serial(args.port, SERIAL_BAUD_RATE, timeout=1)
        time.sleep(2)  # Даем время на установку соединения
        print("Успешное подключение к Sensor MCU.")
        client.publish(
            SENSOR_STATUS_TOPIC, "Connected to Sensor MCU", qos=1, retain=True
        )

        # --- Запуск потока для чтения данных ---
        thread = threading.Thread(target=read_from_mcu, args=(client,), daemon=True)
        thread.start()

        # --- Запрос на запуск потоковой передачи данных ---
        print("Отправка команды 's' для начала потоковой передачи...")
        ser.write(b"s")

        # --- Основной цикл для поддержания работы ---
        while True:
            time.sleep(10)

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
            SENSOR_STATUS_TOPIC, "Publisher disconnected", qos=1, retain=True
        )
        time.sleep(1)  # Даем время на отправку последнего сообщения
        client.loop_stop()
        client.disconnect()
        print("MQTT-клиент отключен.")


if __name__ == "__main__":
    main()
