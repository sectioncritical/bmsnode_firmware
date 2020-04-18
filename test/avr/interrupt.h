
#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#define ISR(fn) void fn(void)

#ifdef __cplusplus
extern "C" {
#endif

extern void cli(void);

#ifdef __cplusplus
}
#endif

#endif
