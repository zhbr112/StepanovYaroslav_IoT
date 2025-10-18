// Пин, к которому подключен светодиод
const int LED_PIN = 13;

// Переменные для неблокирующего мигания
bool isBlinking = false;
unsigned long previousMillis = 0;
const long blinkInterval = 500; // Интервал мигания (500 мс)
int ledState = LOW;

void setup() {
  // Инициализация последовательного порта
  Serial.begin(9600);
  // Настройка пина светодиода на выход
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // Проверка наличия входящих данных от ПК
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    // Обработка команды 'u' - включить светодиод
    if (cmd == 'u') {
      isBlinking = false; // Отключаем мигание
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED_GOES_ON");
    }
    // Обработка команды 'd' - выключить светодиод
    else if (cmd == 'd') {
      isBlinking = false; // Отключаем мигание
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED_GOES_OFF");
    }
    // Обработка команды 'b' - мигать светодиодом
    else if (cmd == 'b') {
      isBlinking = true;
      previousMillis = millis(); // Сбрасываем таймер мигания
      Serial.println("LED_WILL_BLINK");
    }
  }

  // Если включен режим мигания, выполняем неблокирующее переключение светодиода
  if (isBlinking) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      ledState = (ledState == LOW) ? HIGH : LOW;
      digitalWrite(LED_PIN, ledState);
    }
  }
}