#ifndef __WDT_H__
#define __WDT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define WDTO_1S 1000

extern void wdt_enable(uint16_t);
extern void wdt_disable(void);
extern void wdt_reset(void);

#ifdef __cplusplus
}
#endif

#endif
