/******************************************************************************
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2020 Joseph Kroesche
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/

#ifndef __SER_H__
#define __SER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Write data to the serial transmit buffer.
 *
 * @param buf buffer holding data to write
 * @param len count of bytes to write
 *
 * Copies data bytes from _buf_ to the serial transmit buffer. If there are
 * more bytes that will fit in the TX buffer then the return value will
 * indicate the number that were actually copied. Once this function is called,
 * any bytes in the buffer will be transmitted on the serial port.
 *
 * @return the number of bytes that were copied to the transmit buffer.
 */
extern uint8_t ser_write(uint8_t *buf, uint8_t len);

/**
 * Flush the serial transmit buffer. Resets the internal state.
 */
extern void ser_flush(void);

/**
 * Determine if serial hardware is active.
 *
 * Checks internal state of the serial hardware to see if the serial module is
 * active (if active the MCU should not sleep). This function checks the
 * hardware to see if there is any ongoing transmit or receive activity, or if
 * there is any data in a serial buffer.
 *
 * @return `true` if the serail module is active at the moment (TX or RX is
 * in progress)
 *
 * @note the return state is valid only for the moment that it is called. A
 * new interrupt could occur in the next instruction cycle after the return.
 * It is up to the caller to ensure interrupt enablement is handled in a safe
 * way prior to putting the MCU to sleep.
 */
extern bool ser_is_active(void);

#ifdef __cplusplus
}
#endif

#endif
