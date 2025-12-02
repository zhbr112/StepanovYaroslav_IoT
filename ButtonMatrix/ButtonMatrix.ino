#define ROW1 2
#define ROW2 3
#define ROW3 4

#define COL1 5
#define COL2 6
#define COL3 7

#define NROWS 3
#define NCOLS 3
#define TOTAL_BUTTONS (NROWS * NCOLS)

#define SCAN_INTERVAL_MS 10

#define DEBOUNCE_COUNT 3

volatile uint8_t debounce_counter[TOTAL_BUTTONS] = {0};
volatile bool raw_state[TOTAL_BUTTONS] = {false};
volatile bool stable_state[TOTAL_BUTTONS] = {false};
volatile bool current_state[TOTAL_BUTTONS] = {false};

bool previous_state[TOTAL_BUTTONS] = {false};

unsigned long press_start_time[TOTAL_BUTTONS] = {0};

volatile uint8_t current_row = 0;

volatile bool scan_complete = false;

void setup() {
  Serial.begin(9600);
  
  DDRD |= (1 << DDD2) | (1 << DDD3) | (1 << DDD4);
  
  PORTD |= (1 << PORTD2) | (1 << PORTD3) | (1 << PORTD4);
  
  DDRD &= ~((1 << DDD5) | (1 << DDD6) | (1 << DDD7));
  PORTD |= (1 << PORTD5) | (1 << PORTD6) | (1 << PORTD7);
  
  for (uint8_t i = 0; i < TOTAL_BUTTONS; i++) {
    debounce_counter[i] = 0;
    raw_state[i] = false;
    stable_state[i] = false;
    current_state[i] = false;
  }
  
  setup_timer1();
  
  Serial.println("Matrix Keyboard");
}

void setup_timer1() {
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  TCCR1B |= (1 << WGM12);
  uint16_t compare_value = (F_CPU / 256) * SCAN_INTERVAL_MS / 1000 - 1;
  OCR1A = compare_value;
  TCCR1B |= (1 << CS12);
  TIMSK1 |= (1 << OCIE1A);
  
  sei();
}

ISR(TIMER1_COMPA_vect) {
  const uint8_t row_masks[NROWS] = {
    (1 << PORTD2),
    (1 << PORTD3),
    (1 << PORTD4)
  };
  
  const uint8_t col_masks[NCOLS] = {
    (1 << PIND5),
    (1 << PIND6),
    (1 << PIND7)
  };
  
  PORTD |= row_masks[0] | row_masks[1] | row_masks[2];
  
  PORTD &= ~row_masks[current_row];
  
  asm volatile("nop\nnop\nnop\nnop");
  
  uint8_t col_state = PIND;
  
  for (uint8_t icol = 0; icol < NCOLS; icol++) {
    uint8_t btn_index = icol + NCOLS * current_row;
    
    bool new_raw = !(col_state & col_masks[icol]);
    
    // Алгоритм антидребезга
    if (new_raw == raw_state[btn_index]) {
      if (debounce_counter[btn_index] < DEBOUNCE_COUNT) {
        debounce_counter[btn_index]++;
      }
      
      if (debounce_counter[btn_index] >= DEBOUNCE_COUNT) {
        stable_state[btn_index] = new_raw;
      }
    } else {
      debounce_counter[btn_index] = 0;
      raw_state[btn_index] = new_raw;
    }
  }
  
  current_row++;
  if (current_row >= NROWS) {
    current_row = 0;
    
    for (uint8_t i = 0; i < TOTAL_BUTTONS; i++) {
      current_state[i] = stable_state[i];
    }
    
    scan_complete = true;
  }
}

void loop() {
  if (!scan_complete) {
    return;
  }
  
  cli();
  scan_complete = false;
  
  bool local_state[TOTAL_BUTTONS];
  for (uint8_t i = 0; i < TOTAL_BUTTONS; i++) {
    local_state[i] = current_state[i];
  }
  sei();
  
  bool state_changed = false;
  bool button_released[TOTAL_BUTTONS] = {false};
  unsigned long release_duration[TOTAL_BUTTONS] = {0};
  unsigned long release_start[TOTAL_BUTTONS] = {0};
  
  unsigned long current_time = millis();
  
  for (uint8_t i = 0; i < TOTAL_BUTTONS; i++) {
    if (local_state[i] != previous_state[i]) {
      state_changed = true;
      
      if (local_state[i]) {
        press_start_time[i] = current_time;
      } else {
        button_released[i] = true;
        release_start[i] = press_start_time[i];
        release_duration[i] = current_time - press_start_time[i];
      }
    }
  }
  
  if (state_changed) {
    report_pressed_buttons(local_state);
    
    for (uint8_t i = 0; i < TOTAL_BUTTONS; i++) {
      if (button_released[i]) {
        report_button_release(i + 1, release_duration[i], release_start[i]);
      }
    }
  }
  
  for (uint8_t i = 0; i < TOTAL_BUTTONS; i++) {
    previous_state[i] = local_state[i];
  }
}

void report_pressed_buttons(bool* state) {
  bool any_pressed = false;
  
  Serial.print("Pressed buttons: ");
  
  for (uint8_t i = 0; i < TOTAL_BUTTONS; i++) {
    if (state[i]) {
      if (any_pressed) {
        Serial.print(", ");
      }
      Serial.print(i + 1);
      any_pressed = true;
    }
  }
  
  if (!any_pressed) {
    Serial.print("none");
  }
  
  Serial.println();
}

void report_button_release(uint8_t button_num, unsigned long duration, unsigned long start_time) {
  Serial.print("Button ");
  Serial.print(button_num);
  Serial.print(" released: duration ");
  Serial.print(duration);
  Serial.print(" ms, started at ");
  Serial.print(start_time);
  Serial.println(" ms");
}