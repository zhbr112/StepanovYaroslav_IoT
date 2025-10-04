#include <util/atomic.h>

// --- Настройки ---
#define TX_PIN_PORT PORTD
#define TX_PIN_DDR  DDRD
#define TX_PIN      PD3

#define RX_PIN_PORT PORTD
#define RX_PIN_DDR  DDRD
#define RX_PIN_PIN  PIND
#define RX_PIN      PD2

#define UART_BUFFER_SIZE 64

// --- Глобальные переменные ---
volatile char tx_buffer[UART_BUFFER_SIZE];
volatile uint8_t tx_head = 0, tx_tail = 0;

volatile char rx_buffer[UART_BUFFER_SIZE];
volatile uint8_t rx_head = 0, rx_tail = 0;

// Состояние передатчика
enum { TX_IDLE, TX_START_BIT, TX_DATA_BITS, TX_STOP_BIT } volatile tx_state = TX_IDLE;
volatile uint8_t tx_byte_to_send;
volatile uint8_t tx_bit_count;

// Состояние приёмника
volatile uint8_t rx_byte;
volatile uint8_t rx_bit_count;

// Настройки таймера
volatile uint16_t ticks_per_bit = 0;

// API
void uart_set_baudrate(int rate) {
    ticks_per_bit = (F_CPU / 8) / rate;
    TCCR1A = 0;
    TCCR1B = (1 << CS11);
}

void uart_send(char b) {
    uint8_t next_head = (tx_head + 1) % UART_BUFFER_SIZE;
    if (next_head == tx_tail) return;

    tx_buffer[tx_head] = b;
    tx_head = next_head;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (tx_state == TX_IDLE) {
            tx_state = TX_START_BIT;
            OCR1A = TCNT1 + 10;
            TIMSK1 |= (1 << OCIE1A);
        }
    }
}

void uart_send_string(const char *msg) {
    while (*msg) uart_send(*msg++);
}

uint8_t uart_available() {
    uint8_t count;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        count = (rx_head - rx_tail + UART_BUFFER_SIZE) % UART_BUFFER_SIZE;
    }
    return count;
}

char uart_read() {
    if (rx_head == rx_tail) return -1;
    char b = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % UART_BUFFER_SIZE;
    return b;
}

// Прерывание для передачи (TX)
ISR(TIMER1_COMPA_vect) {
    OCR1A += ticks_per_bit;

    switch (tx_state) {
        case TX_START_BIT:
            if (tx_head != tx_tail) {
                tx_byte_to_send = tx_buffer[tx_tail];
                tx_tail = (tx_tail + 1) % UART_BUFFER_SIZE;
                
                // Отправляем старт-бит
                TX_PIN_PORT &= ~(1 << TX_PIN);
                tx_bit_count = 0;
                tx_state = TX_DATA_BITS;
            } else {
                tx_state = TX_IDLE;
                TIMSK1 &= ~(1 << OCIE1A);
            }
            break;

        case TX_DATA_BITS:
            // Отправляем следующий бит данных (LSB first)
            if (tx_byte_to_send & 1) {
                TX_PIN_PORT |= (1 << TX_PIN);
            } else {
                TX_PIN_PORT &= ~(1 << TX_PIN);
            }
            tx_byte_to_send >>= 1;
            tx_bit_count++;
            if (tx_bit_count >= 8) {
                tx_state = TX_STOP_BIT;
            }
            break;

        case TX_STOP_BIT:
            // Отправляем стоп-бит
            TX_PIN_PORT |= (1 << TX_PIN); // HIGH
            // Проверяем, есть ли еще байты для отправки
            if (tx_head != tx_tail) {
                // Если есть, следующий такт начнёт новый старт-бит
                tx_state = TX_START_BIT;
            } else {
                // Если нет, переходим в состояние покоя
                tx_state = TX_IDLE;
                // И отключаем прерывание, т.к. больше отправлять нечего
                TIMSK1 &= ~(1 << OCIE1A);
            }
            break;
    }
}


// Прерывание для приёма (RX)
ISR(TIMER1_COMPB_vect) {
    OCR1B += ticks_per_bit; 

    if (rx_bit_count < 8) {
        if (RX_PIN_PIN & (1 << RX_PIN)) {
            rx_byte |= (1 << rx_bit_count);
        }
        rx_bit_count++;
    } else {
        if (RX_PIN_PIN & (1 << RX_PIN)) {
            uint8_t next_head = (rx_head + 1) % UART_BUFFER_SIZE;
            if (next_head != rx_tail) {
                rx_buffer[rx_head] = rx_byte;
                rx_head = next_head;
            }
        }
        TIMSK1 &= ~(1 << OCIE1B);
        EIFR |= (1 << INTF0);
        EIMSK |= (1 << INT0);
    }
}

// Внешнее прерывание по спадающему фронту на RX-пине (старт-бит)
ISR(INT0_vect) {
    EIMSK &= ~(1 << INT0); 
    
    rx_byte = 0;
    rx_bit_count = 0;

    OCR1B = TCNT1 + ticks_per_bit + (ticks_per_bit / 2);
    TIFR1 |= (1 << OCF1B);
    TIMSK1 |= (1 << OCIE1B);
}

// --- Пример использования ---
void setup() {
    TX_PIN_DDR |= (1 << TX_PIN);
    TX_PIN_PORT |= (1 << TX_PIN);

    RX_PIN_DDR &= ~(1 << RX_PIN);
    RX_PIN_PORT |= (1 << RX_PIN);
    
    uart_set_baudrate(9600);

    EICRA |= (1 << ISC01);
    EIMSK |= (1 << INT0);

    sei();

    for(volatile long i=0; i<1000000; i++);

    uart_send_string("myUart - OK\r\nEcho server started:\r\n");
}

void loop() {
    if (uart_available()) {
        char c = uart_read();
        uart_send(c);
    }
}