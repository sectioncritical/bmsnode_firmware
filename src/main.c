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

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 8000000L
#define BAUD 4800
#include <util/delay.h>
#include <util/setbaud.h>

#include "pkt.h"
#include "serial.h"

/*
 * Default clocking, per fuses, is 8 MHz internal oscillator with
 * clock divider of 1. So system runs at 8 MHz.
 */

/*
 * Pin Assignments
 *
 * "A" mean alt function
 *
 * | Port | Pin | IO | Function                           |
 * |------|-----|----|------------------------------------|
 * |  A0  | 13  | AI | ADC external reference             |
 * |  A1  | 12  | AO | UART0 TX                           |
 * |  A2  | 11  | AI | UART0 RX                           |
 * |  A3  | 10  | O  | GPIO load enable                   |
 * |  A4  |  9  | AI | ADC internal temp sensor (ISP SCK) |
 * |  A5  |  8  | O  | GPIO blue LED (ISP MISO)           |
 * |  A6  |  7  | O  | GPIO green LED (ISP MOSI)          |
 * |  A7  |  6  | O  | GPIO ref enable (needs high drive) |
 * |  B0  |  2  | AI | ADC external temp sensor           |
 * |  B1  |  3  | I/O| GPIO spare external                |
 * |  B2  |  5  | AI | ADC cell voltage                   |
 * |  B3  |  4  | AI | reset (pulled up)                  |
 *
 */

void device_init(void)
{
    // turn off peripherals we are not using
    PRR = _BV(PRTWI) | _BV(PRUSART1) | _BV(PRSPI);

    // set up watchdog

    // set up IO
    // this sets up GPIOs. alt functions are set up with peripheral inits
    PORTA = 0; // set all outputs low, to get known initial state
    PORTB = 0;
    PHDE = _BV(PHDEA1); // PA7 drives the reference diode and needs high drive
    DDRA = _BV(DDA3) | _BV(DDA5) | _BV(DDA6) | _BV(DDA7); // GPIO outputs
    DDRB = _BV(DDB1);   // PB1 spare is set as output

    // set up UART0
    UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0); // enable tx, rx, rx int
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // 8n1
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
}

static uint8_t testdata[2] = { 'A', '-' };

int main(void)
{
    // preserve the reset cause, take TBD actions
    uint8_t reset_cause = MCUSR;
    MCUSR = 0;

    if (reset_cause & WDRF)
    {
        // watchdog reset
    }
    else if (reset_cause & BORF)
    {
        // brownout
    }
    else if (reset_cause & EXTRF)
    {
        // external reset
    }
    else if (reset_cause & PORF)
    {
        // power-on reset
    }

    // hardware init of MCU
    device_init();

    // enable system interrupts
    sei();

    // blinky loop
    while (1)
    {
        PORTA = _BV(PORTA6);
        _delay_ms(500);
        PORTA = 0;
        _delay_ms(500);
        //UDR0 = 'Z'; // 0x5A
        ser_write(testdata, 2);

        // clear incoming packet, if any
        // this is a placeholder for eventual packet command process
        packet_t *pkt = pkt_ready();
        if (pkt)
        {
            pkt_rx_free(pkt);
        }
    }

    return 0;
}
