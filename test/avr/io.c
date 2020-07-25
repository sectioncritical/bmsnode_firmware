#include <stdint.h>
#include <stdbool.h>

#include <avr/io.h>

volatile uint8_t UCSR0B = 0;
volatile uint8_t UCSR0A = 0;
volatile uint8_t UCSR0C = 0;
volatile uint8_t UCSR0D = 0;
volatile uint8_t UBRR0L = 0;
volatile uint8_t UBRR0H = 0;
volatile uint8_t UDR0 = 0;
volatile uint8_t MCUSR = 0;
volatile uint8_t PRR = 0;
volatile uint8_t PHDE = 0;
volatile uint8_t DDRA = 0;
volatile uint8_t DDRB = 0;
volatile uint8_t PUEA = 0;
volatile uint8_t PORTA = 0;
volatile uint8_t PORTB = 0;
volatile uint8_t TCCR0A = 0;
volatile uint8_t TCCR0B = 0;
volatile uint8_t OCR0A = 0;
volatile uint8_t TIMSK0 = 0;
volatile uint8_t ACSR0A = 0;
volatile uint8_t ACSR1A = 0;
volatile uint8_t ADMUXA = 0;
volatile uint8_t ADMUXB = 0;
volatile uint8_t ADCH = 0;
volatile uint8_t ADCL = 0;
volatile uint8_t ADCSRA = 0;
volatile uint8_t DIDR0 = 0;
volatile uint8_t DIDR1 = 0;

volatile bool global_int_flag = false;

void _delay_loop_2(uint16_t count)
{
    return;
}
