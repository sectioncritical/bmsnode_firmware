
#ifndef __IO_H__
#define __IO_H__

#ifdef __cplusplus
extern "C" {
#endif

#define _BV(v)      (1 << (v))

// UCSR0A
extern volatile uint8_t UCSR0A;
#define RXC0        (7)
#define TXC0        (6)
#define UDRE0       (5)

// UCSR0B
extern volatile uint8_t UCSR0B;
#define RXCIE0      (7)
#define TXCIE0      (6)
#define UDRIE0      (5)
#define RXEN0       (4)
#define TXEN0       (3)

// UCSR0C
extern volatile uint8_t UCSR0C;
#define UCSZ01      (2)
#define UCSZ00      (1)

// UCSR0D
extern volatile uint8_t UCSR0D;
#define SFDE0       (5)

// baud rate registers, dummy init values
extern volatile uint8_t UBRR0L;
extern volatile uint8_t UBRR0H;
#define UBRRH_VALUE (0x67)
#define UBRRL_VALUE (0x89)

// PRR
extern volatile uint8_t PRR;
#define PRTWI       (7)
#define PRUSART1    (6)
#define PRSPI       (4)
#define PRTIM2      (3)
#define PRTIM1      (2)

// PHDE
extern volatile uint8_t PHDE;
#define PHDEA1      (1)
#define PHDEA0      (0)

// DDRA
extern volatile uint8_t DDRA;
#define DDA7        (7)
#define DDA6        (6)
#define DDA5        (5)
#define DDA4        (4)
#define DDA3        (3)
#define DDA2        (2)
#define DDA1        (1)
#define DDA0        (0)

// DDRB
extern volatile uint8_t DDRB;
#define DDB7        (7)
#define DDB6        (6)
#define DDB5        (5)
#define DDB4        (4)
#define DDB3        (3)
#define DDB2        (2)
#define DDB1        (1)
#define DDB0        (0)

// PUEA
extern volatile uint8_t PUEA;

// gpio port registers
extern volatile uint8_t PORTA;
#define PORTA7      (7)
#define PORTA6      (6)
#define PORTA5      (5)
#define PORTA4      (4)
#define PORTA3      (3)
#define PORTA2      (2)
#define PORTA1      (1)
#define PORTA0      (0)
extern volatile uint8_t PORTB;
#define PORTB7      (7)
#define PORTB6      (6)
#define PORTB5      (5)
#define PORTB4      (4)
#define PORTB3      (3)
#define PORTB2      (2)
#define PORTB1      (1)
#define PORTB0      (0)

// TCCR0A
extern volatile uint8_t TCCR0A;
#define WGM01       (1)

// TCCR0B
extern volatile uint8_t TCCR0B;
#define CS00        (0)
#define CS01        (1)

extern volatile uint8_t OCR0A;

// TIMSK0
extern volatile uint8_t TIMSK0;
#define OCIE0A      (1)

// TOCPMSA0
extern volatile uint8_t TOCPMSA0;
#define TOCC3S1     (7)
#define TOCC3S0     (6)
#define TOCC2S1     (5)
#define TOCC2S0     (4)
#define TOCC1S1     (3)
#define TOCC1S0     (2)
#define TOCC0S1     (1)
#define TOCC0S0     (0)

// TOCPMSA1
extern volatile uint8_t TOCPMSA1;
#define TOCC7S1     (7)
#define TOCC7S0     (6)
#define TOCC6S1     (5)
#define TOCC6S0     (4)
#define TOCC5S1     (3)
#define TOCC5S0     (2)
#define TOCC4S1     (1)
#define TOCC4S0     (0)

// TOCPMCOE
extern volatile uint8_t TOCPMCOE;
#define TOCC7OE     (7)
#define TOCC6OE     (6)
#define TOCC5OE     (5)
#define TOCC4OE     (4)
#define TOCC3OE     (3)
#define TOCC2OE     (2)
#define TOCC1OE     (1)
#define TOCC0OE     (0)

// TCCR1A
extern volatile uint8_t TCCR1A;
#define COM1A1      (7)
#define COM1A0      (6)
#define COM1B1      (5)
#define COM1B0      (4)
#define WGM11       (1)
#define WGM10       (0)

// TCCR1B
extern volatile uint8_t TCCR1B;
#define WGM13       (4)
#define WGM12       (3)
#define CS12        (2)
#define CS11        (1)
#define CS10        (0)

// OCR1AH/L/B
extern volatile uint8_t OCR1AH;
extern volatile uint8_t OCR1AL;
extern volatile uint8_t OCR1BH;
extern volatile uint8_t OCR1BL;


extern volatile uint8_t UDR0;
extern volatile uint8_t MCUSR;
extern volatile uint8_t ACSR0A;
extern volatile uint8_t ACSR1A;

// ADC

extern volatile uint8_t ADMUXA;
extern volatile uint8_t ADMUXB;
extern volatile uint8_t ADCH;
extern volatile uint8_t ADCL;
extern volatile uint8_t ADCSRA;
#define ADEN        (7)
#define ADSC        (6)
extern volatile uint8_t DIDR0;
#define ADC4D       (4)
extern volatile uint8_t DIDR1;
#define ADC8D       (2)
#define ADC11D      (0)

#ifdef __cplusplus
}
#endif

#endif
