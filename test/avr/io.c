#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <avr/io.h>

#define REG8(n) volatile uint8_t n = 0

PORT_t PORTA;
PORT_t PORTB;
USART_t USART0;
RSTCTRL_t RSTCTRL;
TCB_t TCB0;
TCA_t TCA0;
ADC_t ADC0;

/*volatile uint8_t MCUSR = 0;
volatile uint8_t PRR = 0;
volatile uint8_t PHDE = 0;
volatile uint8_t DDRA = 0;
volatile uint8_t DDRB = 0;
volatile uint8_t PUEA = 0;*/
//volatile uint8_t PORTA = 0;
//volatile uint8_t PORTB = 0;
/*volatile uint8_t TCCR0A = 0;
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
volatile uint8_t TOCPMSA1 = 0;
volatile uint8_t TOCPMSA0 = 0;
volatile uint8_t TOCPMCOE = 0;
volatile uint8_t TCCR1A = 0;
volatile uint8_t TCCR1B = 0;
REG8(OCR1AH);
REG8(OCR1AL);
REG8(OCR1BH);
REG8(OCR1BL);*/

volatile bool global_int_flag = false;

void _delay_loop_2(uint16_t count)
{
    return;
}

void ccp_write_io (uint8_t *__ioaddr, uint8_t __value)
{
    //printf("ccp_write_io(%p, %u)", __ioaddr, __value);
    *__ioaddr = __value;
}
