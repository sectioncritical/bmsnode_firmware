#ifndef __SLEEP_H__
#define __SLEEP_H__

#define SLEEP_MODE_PWR_DOWN (88)

#ifdef __cplusplus
extern "C" {
#endif

extern void set_sleep_mode(uint8_t mode);
extern void sleep_mode(void);

#ifdef __cplusplus
}
#endif

#endif
