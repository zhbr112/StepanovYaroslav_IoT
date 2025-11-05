// --- ОПРЕДЕЛЕНИЕ ПИНОВ И ПОРТОВ ---
#define LATCH_PIN_DDR  DDRD
#define LATCH_PIN_PORT PORTD
#define LATCH_PIN_MASK (1 << 5) // Arduino Pin 5

#define CLOCK_PIN_DDR  DDRD
#define CLOCK_PIN_PORT PORTD
#define CLOCK_PIN_MASK (1 << 3) // Arduino Pin 3

#define DATA_PIN_DDR   DDRD
#define DATA_PIN_PORT  PORTD
#define DATA_PIN_MASK  (1 << 7) // Arduino Pin 7

// --- ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ---
volatile int seconds_counter = 0;
volatile int user_value = -1; // Значение от пользователя (-1 означает нет нового значения)
volatile bool is_initialized = false; // Флаг, что счетчик получил начальное значение

const uint8_t SEGMENT_MAP[10] = {
  B10111011,  // 0
  B00001010,  // 1
  B01110011,  // 2
  B01011011,  // 3
  B11001010,  // 4
  B11011001,  // 5
  B11111101,  // 6
  B00001011,  // 7
  B11111011,  // 8
  B11011111   // 9
};

// Функция для побитовой отправки байта в сдвиговый регистр
void shift_byte(uint8_t data) {
  for (int i = 7; i >= 0; i--) {
    if (data & (1 << i)) {
      DATA_PIN_PORT |= DATA_PIN_MASK;
    } else {
      DATA_PIN_PORT &= ~DATA_PIN_MASK;
    }
    CLOCK_PIN_PORT |= CLOCK_PIN_MASK;
    CLOCK_PIN_PORT &= ~CLOCK_PIN_MASK;
  }
}

// Получение байта для сегментов
uint8_t get_segment_byte(int digit) {
  if (digit < 0 || digit > 9) return 0;
  uint8_t segment_data = SEGMENT_MAP[digit];
  return segment_data;
}

void setup() {
  LATCH_PIN_DDR |= LATCH_PIN_MASK;
  CLOCK_PIN_DDR |= CLOCK_PIN_MASK;
  DATA_PIN_DDR  |= DATA_PIN_MASK;

  Serial.begin(9600);

  cli();
  TCCR1A = 0; TCCR1B = 0; TCNT1  = 0;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12) | (1 << CS10);
  OCR1A = 15624;
  TIMSK1 |= (1 << OCIE1A);
  sei();
}

// Обработчик прерывания таймера Timer1 (вызывается каждую секунду)
ISR(TIMER1_COMPA_vect) {
  int value_to_display;
  int current_user_value;

  // Атомарно считываем и сбрасываем значение от пользователя
  cli();
  current_user_value = user_value;
  user_value = -1; // Сбрасываем, чтобы использовать только один раз
  sei();

  if (!is_initialized) {
    // Система ждет первого значения от пользователя
    if (current_user_value != -1) {
      // Получили первое значение, инициализируем счетчик
      seconds_counter = current_user_value;
      is_initialized = true;
    }
    value_to_display = seconds_counter;
  } else {
    // Система уже инициализирована и работает
    seconds_counter++;
    if (seconds_counter >= 60) {
      seconds_counter = 0;
    }
    
    if (current_user_value != -1) {
      // Если пришло новое значение от пользователя, оно ПЕРЕОПРЕДЕЛЯЕТ отображение на этот тик
      value_to_display = current_user_value;
    } else {
      // Если нового значения нет, отображаем значение основного счетчика
      value_to_display = seconds_counter;
    }
  }

  // Убедимся, что отображаемое значение в рамках 0-59
  if (value_to_display >= 60) {
    value_to_display = value_to_display % 60;
  }

  // Разделяем значение для отображения на десятки и единицы
  int tens = value_to_display / 10;
  int ones = value_to_display % 10;
  
  // --- Обновление дисплеев ---
  LATCH_PIN_PORT &= ~LATCH_PIN_MASK; // LATCH LOW
  shift_byte(get_segment_byte(tens)); // Отправляем десятки
  shift_byte(get_segment_byte(ones)); // Отправляем единицы
  LATCH_PIN_PORT |= LATCH_PIN_MASK;  // LATCH HIGH
}

void loop() {
  // Статические переменные для хранения состояния между вызовами loop()
  static char inputBuffer[4];
  static byte bufferIndex = 0;
  static unsigned long lastCharTime = 0; // Время получения последнего символа
  const unsigned long timeout = 25;      // Таймаут в миллисекундах

  // Чтение данных, если они есть
  if (Serial.available() > 0) {
    char incomingChar = Serial.read();

    // Собираем в буфер только цифры и пока есть место
    if (isDigit(incomingChar) && bufferIndex < 2) {
      inputBuffer[bufferIndex] = incomingChar;
      bufferIndex++;
      lastCharTime = millis(); // Обновляем время получения символа
    }
  }

  // Обработка по таймауту
  // Если в буфере что-то есть (bufferIndex > 0)
  // И если прошло достаточно времени с момента получения последнего символа
  if (bufferIndex > 0 && (millis() - lastCharTime > timeout)) {
    // Мы считаем, что ввод завершен
    inputBuffer[bufferIndex] = '\0';   // Завершаем строку
    int val = atoi(inputBuffer);       // Преобразуем в число

    // Атомарно передаем значение в ISR
    cli();
    user_value = val;
    sei();

    // Сбрасываем индекс буфера, готовясь к следующему вводу
    bufferIndex = 0;
  }
}