
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

extern volatile uint8_t UDR0;
extern volatile uint8_t MCUSR;
extern volatile uint8_t ACSR0A;
extern volatile uint8_t ACSR1A;

#ifdef __cplusplus
}
#endif

#endif
