
#ifndef __EEPROM_H__
#define __EEPROM_H__

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t eeprom_read_byte(const uint8_t *);
extern uint32_t eeprom_read_dword(const uint32_t *);
extern void eeprom_read_block(void *, const void *, size_t);
extern void eeprom_update_block(const void *, void *, size_t);

#ifdef __cplusplus
}
#endif

#endif
