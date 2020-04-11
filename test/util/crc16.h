
// fake avr-libc header file so pkt.c can be compiled native for unit test

#ifndef __CRC16_H__
#define __CRC16_H__

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t _crc8_ccitt_update(uint8_t, uint8_t);

#ifdef __cplusplus
}
#endif

#endif
