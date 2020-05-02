
#ifndef __IO_H__
#define __IO_H__

#define _BV(v) (1 << (v))

#define RXCIE0 (7)
#define UDRIE0 (5)
#define RXC0 (7)
#define TXC0 (6)
#define UDRE0 (5)

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint8_t UCSR0B;
extern volatile uint8_t UCSR0A;
extern volatile uint8_t UDR0;
extern volatile uint8_t MCUSR;

#ifdef __cplusplus
}
#endif

#endif
