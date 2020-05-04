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
#include <avr/sleep.h>

#define F_CPU 8000000L
#define BAUD 4800
#include <util/setbaud.h>
#include <util/atomic.h>

#include "cmd.h"
#include "ser.h"
#include "cfg.h"
#include "pkt.h"
#include "tmr.h"

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
    PRR = _BV(PRTWI) | _BV(PRUSART1) | _BV(PRSPI) | _BV(PRTIM2) | _BV(PRTIM1);

    // turn off analog comparators
    ACSR0A = 0x80;
    ACSR1A = 0x80;

    // not using watchdog, so disable it

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
    UCSR0D = _BV(SFDE0); // start frame detector
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
}


static uint16_t blink_timeout;
static uint16_t sleep_timeout;

void main_loop(void)
{
    cli(); // should already be disabled, just to be sure
    wdt_disable();

#if 0 // for now we do not have any dependency on reset cause
    // take action based on the reason for reset
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
#endif

    // hardware init of MCU
    device_init();
    tmr_init();

    // load the node configuration
    nodecfg = cfg_load();

    // send out one byte on wake (this causes TXC to get set)
    UDR0 = 0x55;

    pkt_reset(); // init packet parser
    ser_flush(); // init serial module

    // enable system interrupts
    sei();

    // turn on blue LED at the start
    PORTA |= _BV(PORTA5);

    // do an initial blink for 5 seconds
    // with 100 ms toggle
    blink_timeout = tmr_set(100);
    sleep_timeout = tmr_set(5000);

    while(!tmr_expired(sleep_timeout))
    {
        if (tmr_expired(blink_timeout))
        {
            blink_timeout += 100;
            PORTA ^= _BV(PORTA5);
        }
    }

    // leave the blue LED on
    PORTA |= _BV(PORTA5);

    // reset sleep timeout to 1 sec
    // and blink to 200 msec toggle
    blink_timeout = tmr_set(200);
    sleep_timeout = tmr_set(1000);

    // blinky loop
    while (1)
    {
        // run blue LED blinker
        if (tmr_expired(blink_timeout))
        {
            blink_timeout += 200;
            PORTA ^= _BV(PORTA5);
        }

        // run the command processor
        bool b_processed = cmd_process();

        // if there was a command processed, or if the pkt or ser
        // modules indicate they are active, reset the sleep timeout
        if (b_processed
         || pkt_is_active()
         || ser_is_active())
        {
            sleep_timeout = tmr_set(1000);
        }

        // if sleep timeout expires, then go to low pwer mode
        if (tmr_expired(sleep_timeout))
        {
            // force LED outputs off
            PORTA &= ~(_BV(PORTA5) | _BV(PORTA6));
            // disable the UART TX (for power saving)
            UCSR0B &= ~_BV(TXEN0);

            // go to sleep
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);
            sleep_mode();

            // WAKING UP
            // re-enable UART TX
            UCSR0B |= _BV(TXEN0);
            // turn on blue LED and set a timeout
            PORTA |= _BV(PORTA5);
            blink_timeout = tmr_set(200);
            sleep_timeout = tmr_set(1000);
        }

#ifdef UNIT_TEST
        extern bool test_exit;
        if (test_exit)
        {
            break;
        }
#endif

    }
}

#ifndef UNIT_TEST
int main(void)
{
    main_loop();
    return 0;
}
#endif

// this is code to fetch and save the reset cause, which can later be
// used in main in conditional code related to cause of reset
// I am keeping it here as a reference in case I need to add it back later
#if 0
// TODO make sure this is not initialized by C runtime
volatile uint8_t __attribute__ ((used)) reset_cause;

// this function will run before C startup
// use it to detect if boot loader started and to get the
// reset cause
void __attribute__ ((noinline, used)) __init(void)
{
    register uint8_t r2_value __asm__("r2");

    // if boot loader started us, then read reset cause from R2
    // note: BOOTLOADER_START is placeholder for method to detect bootloader
    if (BOOTLOADER_START)
    {
        __asm__ volatile("": "=r" (r2_value));
        reset_cause = r2_value;
    }
    // otherwise, no bootloader, read reset cause from MCU register
    else
    {
        reset_cause = MCUSR;
        MCUSR = 0;
    }
}
#endif
