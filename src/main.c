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
#include <avr/wdt.h>

#define F_CPU 8000000L
#define BAUD 4800
#include <util/delay.h>
#include <util/setbaud.h>
#include <util/atomic.h>

#include "cmd.h"
#include "serial.h"
#include "cfg.h"
#include "pkt.h"

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

// hold global pointer to node configuration
config_t *nodecfg;

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

    // 1 ms timer tick using timer 0
    // 8mhz w /64 prescaler = 125000 hz
    // 125000 Hz /125 ==> 1 khz --> 1 ms
    // timer 0 in CTC mode, count up, OCR0A=124, OCF0A interrupt
    TCCR0A = _BV(WGM01); // CTC mode
    TCCR0B = _BV(CS01) | _BV(CS00); // prescaler /64
    OCR0A = 124;
    TIMSK0 = _BV(OCIE0A);
    // timer should be running and generating 1 ms interrupts
}

static volatile uint16_t systick = 0;

// TODO: timer isr here for now. create timer module
ISR(TIMER0_COMPA_vect)
{
    ++systick;
}

static uint16_t get_systick(void)
{
    uint16_t ticks;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        ticks = systick;
    }
    return ticks;
}

static bool is_timeout(uint16_t start, uint16_t timeout)
{
    return (get_systick() - start) >= timeout;
}

//static uint8_t testdata[2] = { 'A', '-' };

static uint16_t ticks_blink;

int main(void)
{
    // preserve the reset cause, take TBD actions
    uint8_t reset_cause = MCUSR;
    MCUSR = 0;
    wdt_disable();

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

    // load the node configuration
    nodecfg = cfg_load();

    pkt_reset(); // init packet parser
    ser_flush(); // init serial module

    // enable system interrupts
    sei();

    ticks_blink = get_systick();

    // blinky loop
    while (1)
    {
        if (is_timeout(ticks_blink, 500))
        {
            ticks_blink += 500;
            PORTA ^= _BV(PORTA6);
        }

        //UDR0 = 'Z'; // 0x5A
        //ser_write(testdata, 2);

        cmd_process();
    }

    return 0;
}
