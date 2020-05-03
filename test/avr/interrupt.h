
#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#define ISR(fn) void fn(void)

#ifdef __cplusplus
extern "C" {
#endif

extern volatile bool global_int_flag;

#define cli() (global_int_flag = false)
#define sei() (global_int_flag = true)

#ifdef __cplusplus
}
#endif

#endif
