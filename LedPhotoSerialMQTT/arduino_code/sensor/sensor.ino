// Пин к которому подключен фоторезистор
const int LDR_PIN = A0;

// Флаг для управления потоковой передачей данных
bool streaming = false;

void setup() {
  // Инициализация последовательного порта
  Serial.begin(9600);
}

void loop() {
  // Проверка наличия входящих данных от ПК
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    // Обработка команды 'p' - запросить одно значение
    if (cmd == 'p') {
      streaming = false; // Останавливаем поток, если он был активен
      int sensorValue = analogRead(LDR_PIN);
      Serial.print("SENSOR_VALUE:");
      Serial.println(sensorValue);
    }
    // Обработка команды 's' - начать потоковую передачу
    else if (cmd == 's') {
      streaming = true;
      Serial.println("STREAM_STARTED");
    }
  }

  // Если включен режим потоковой передачи, отправляем данные каждую секунду
  if (streaming) {
    int sensorValue = analogRead(LDR_PIN);
    Serial.print("SENSOR_VALUE:");
    Serial.println(sensorValue);
    delay(1000); // Интервал передачи данных
  }
}