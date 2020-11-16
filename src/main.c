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
#ifndef BAUD
#define BAUD 4800
#endif
#include <util/setbaud.h>
#include <util/atomic.h>

#include "cmd.h"
#include "ser.h"
#include "cfg.h"
#include "pkt.h"
#include "tmr.h"
#include "adc.h"
#include "shunt.h"
#include "testmode.h"
#include "led.h"
#include "kissm.h"

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
 * |  A4  |  9  | AI | ADC4 internal temp sensor (ISP SCK)|
 * |  A5  |  8  | O  | GPIO blue LED (ISP MISO)           |
 * |  A6  |  7  | O  | GPIO green LED (ISP MOSI)          |
 * |  A7  |  6  | O  | GPIO ref enable (needs high drive) |
 * |  B0  |  2  | AI | ADC11 external temp sensor         |
 * |  B1  |  3  | I/O| GPIO spare external                |
 * |  B2  |  5  | AI | ADC8 cell voltage                  |
 * |  B3  |  4  | AI | reset (pulled up)                  |
 *
 */

void device_init(void)
{
    // global not populated yet, so get the board type
    uint8_t boardtype = cfg_board_type();

    // turn off peripherals we are not using
    PRR = _BV(PRTWI) | _BV(PRUSART1) | _BV(PRSPI) | _BV(PRTIM2) | _BV(PRTIM1);

    // turn off analog comparators
    ACSR0A = 0x80;
    ACSR1A = 0x80;

    // set up IO
    // this sets up GPIOs. alt functions are set up with peripheral inits
    PORTA = 0; // set all outputs low, to get known initial state
    PORTB = 0;
    PHDE = _BV(PHDEA1); // PA7 drives the reference diode and needs high drive
    DDRA = _BV(DDA3) | _BV(DDA5) | _BV(DDA6) | _BV(DDA7); // GPIO outputs
    DDRB = _BV(DDB1);   // PB1 spare is set as output

    // set up UART0
    if (boardtype < BOARD_TYPE_BMSNODE)
    {
        // old diyBMS style board types
        UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0); // enable tx, rx, rx int
    }
    else
    {
        // bmsnode style boards (shared tx/rx)
        // enables RX, RX interrupt only. leave TX disabled until needed
        UCSR0B = _BV(RXEN0) | _BV(RXCIE0);

        // keep TX line as input when not actively transmitting
        // enable pullup on the pin so that it keeps the shared TX/RX line
        // pulled high when not being driven low
        // This is overridden whenever TX is enabled in the ser module for
        // sending data
        PUEA = _BV(PORTA1);
    }

    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // 8n1
    UCSR0D = _BV(SFDE0); // start frame detector
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
}

// app event types
enum app_event_types
{
    EVT_TMR = KISSM_EVT_APP,    // timer event occurred
    EVT_TIMEOUT,                // state timeout
    EVT_CMD                     // command event occurred
};

// declare states used in this application
KISSM_DECLSTATE(powerup);
KISSM_DECLSTATE(idle);
KISSM_DECLSTATE(dfu);
KISSM_DECLSTATE(shunt);
KISSM_DECLSTATE(testmode);
KISSM_DECLSTATE(sleep);

// app level variables
static struct tmr state_tmr;

#define STATE_TMR 1

// powerup state
// does rapid blink, 1 second delay after system start
KISSM_DEFSTATE(powerup)
{
    struct kissm_state *p_ns = NULL;

    switch (p_event->type)
    {
        case KISSM_EVT_ENTRY:
        {
            led_blink(LED_BLUE, 50, 50);    // LED rapid blink 50 ms
            // state timeout 1 second
            tmr_schedule(&state_tmr, STATE_TMR, 1000, false);
            break;
        }

        case EVT_TIMEOUT:
        {
            // if the state timed out, switch to idle
            p_ns = KISSM_STATEREF(idle);
            break;
        }

        default:
        {
            break;
        }
    }
    return p_ns;
}

// idle state
// monitor activity and keep awake while stuff is happening
KISSM_DEFSTATE(idle)
{
    struct kissm_state *p_ns = NULL;

    switch (p_event->type)
    {
        case KISSM_EVT_ENTRY:
        {
            // stay awake for at least 1 second
            tmr_schedule(&state_tmr, STATE_TMR, 1000, false);
            led_blink(LED_BLUE, 200, 200);  // normal activity blinker
            break;
        }

        // no exit-specific action
        case KISSM_EVT_EXIT:
        {
            break;
        }

        // handle commands
        case EVT_CMD:
        {
            // for any command received, reset the timeout
            tmr_schedule(&state_tmr, STATE_TMR, 1000, false);

            // process the command
            uint8_t cmd = ((packet_t *)p_event->data.p)->cmd;;
            switch (cmd)
            {
                case CMD_DFU:
                    p_ns = KISSM_STATEREF(dfu);
                    break;

                case CMD_SHUNTON:
                    p_ns = KISSM_STATEREF(shunt);
                    break;

                case CMD_TESTMODE:
                    p_ns = KISSM_STATEREF(testmode);
                    break;

                case CMD_PING:
                    led_oneshot(LED_GREEN, 1000);   // green LED on for 1 sec
                    break;
            }
            break;
        }

        // handle state timeout
        // means there has been no activity so need to sleep
        case EVT_TIMEOUT:
        {
            p_ns = KISSM_STATEREF(sleep);
            break;
        }

        // if not other event, then monitor for activity and keep awake
        // as long as things are happening
        default:
        {
            // if a command was just processed, or if other modules
            // are current active (packets in processs) then
            // reset the state timeout
            if (pkt_is_active() || ser_is_active())
            {
                tmr_schedule(&state_tmr, STATE_TMR, 1000, false);
            }

            break;
        }
    }
    return p_ns;
}

// enter DFU 8 second delay with slow blink
// if this node was addressed then the command processor already
// invoked the boot loader
// but if not this node, then this delay just ensures the node stays
// awake long enough for the "other" node to get boot loader started
//
// NOTE: this behavior is a legacy of the original design that required
// the node to continue to repeat all data even if it was not addressed for
// DFU. The present hardware design does not require this and this state
// can probably be eliminated
KISSM_DEFSTATE(dfu)
{
    struct kissm_state *p_ns = NULL;

    switch (p_event->type)
    {
        case KISSM_EVT_ENTRY:
        {
            tmr_schedule(&state_tmr, STATE_TMR, 8000, false);
            led_blink(LED_BLUE,  1000, 1000);
            break;
        }

        case EVT_TIMEOUT:
        {
            p_ns = KISSM_STATEREF(idle);
            break;
        }

        case KISSM_EVT_EXIT:
        default:
        {
            break;
        }
    }
    return p_ns;
}

// state for running shunting mode
// state will remain active as long as the shunt module is running
// the shunt module will return indication when it is finished and
// then this state can exit
KISSM_DEFSTATE(shunt)
{
    struct kissm_state *p_ns = NULL;

    switch (p_event->type)
    {
        case KISSM_EVT_ENTRY:
        {
            // enable the WDT so that we cant get stuck in this state
            wdt_enable(WDTO_1S);
            shunt_start();  // start the shunt module running
            break;
        }

        // if SHUNTOFF command then signal shunt module to stop
        // we do not actually exit here. the shunt module will indicate
        // when it is stopped (in the default case below)
        case EVT_CMD:
        {
            uint8_t cmd = ((packet_t *)p_event->data.p)->cmd;;
            if (cmd == CMD_SHUNTOFF)
            {
                shunt_stop();
            }
        }

        case KISSM_EVT_EXIT:
        {
            // turn off watchdog timer
            MCUSR = 0;
            wdt_disable();
            break;
        }

        default:
        {
            // pet the watchdog
            // then run the shunt module (must be called continuously)
            wdt_reset();
            shunt_status_t sts = shunt_run();
            if (SHUNT_OFF == sts)
            {
                // if shunt stops running, then go back to idle
                p_ns = KISSM_STATEREF(idle);
            }
            break;
        }
    }

    return p_ns;
}

// run the testmode state
// test mode is already activated by command module
// this state just keeps it running and monitors for completion
KISSM_DEFSTATE(testmode)
{
    struct kissm_state *p_ns = NULL;

    switch (p_event->type)
    {
        case KISSM_EVT_ENTRY:
        {
            // enabled watchdog so we cant get stuck here
            wdt_enable(WDTO_1S);
            break;
        }

        case KISSM_EVT_EXIT:
        {
            // turn off watchdog timer
            MCUSR = 0;
            wdt_disable();
            break;
        }

        default:
        {
            // pet the watchdog
            wdt_reset();
            // keep the testmode module running
            testmode_status_t test_sts = testmode_run();
            // if testmode ends, switch back to idle
            if (test_sts == TESTMODE_OFF)
            {
                p_ns = KISSM_STATEREF(idle);
            }
        }
    }

    return p_ns;
}

// sleep state
// activate from idle when there has been no activity
// processor stops running code during sleep
KISSM_DEFSTATE(sleep)
{
    struct kissm_state *p_ns = NULL;

    switch (p_event->type)
    {
        case KISSM_EVT_ENTRY:
        {
            adc_powerdown();    // shut down analog circuits

            // force LEDs and external outputs off
            led_off(LED_BLUE);
            led_off(LED_GREEN);
            PORTA &= ~_BV(PORTA3);

            // TODO: this does not turn off external IO, if it was on

            // this is only needed for old boards
            if (g_board_type < BOARD_TYPE_BMSNODE)
            {
                // disable the UART TX (for power saving)
                UCSR0B &= ~_BV(TXEN0);
            }

            // go to sleep
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);
            sleep_mode();

            // at this point MCU is sleep/pwrdn
            // when it wakes it resumes execution here

            break;
        }

        case KISSM_EVT_EXIT:
        {
            // WAKING UP
            // this is only needed for old boards
            if (g_board_type < BOARD_TYPE_BMSNODE)
            {
                // re-enable UART TX
                UCSR0B |= _BV(TXEN0);
            }
            // turn on blue LED to show we wake up
            led_on(LED_BLUE);

            // re-enable the analog
            adc_powerup();

            break;
        }

        default:
        {
            // we pass through here after wake up,
            // signal state machine to switch to idle state
            p_ns = KISSM_STATEREF(idle);
            break;
        }
    }

    return p_ns;
}

// system init and main loop
void main_loop(void)
{
    static uint16_t adc_timeout;    // static to keep off the stack

    cli();      // should already be disabled, just to be sure
    MCUSR = 0;  // this has to be done here in order to disable WDT
    wdt_disable();

    device_init();  // hardware init
    cfg_load();     // load node config (global cfg needed for later calls)

    // old board style needs this byte sent at init
    if (g_board_type < BOARD_TYPE_BMSNODE)
    {
        // send out one byte on wake (this causes TXC to get set)
        UDR0 = 0x55;
    }

    // init the software modules
    tmr_init();
    pkt_reset();
    ser_flush();

    // enable interrupts to start things running
    sei();

    // turn on LED for aliveness indicator
    led_on(LED_BLUE);

    // power up ADC circuitry
    adc_powerup();
    adc_timeout = tmr_set(1);   // cause first ADC conversion to happen soon
    // TODO: move ADC run timeout to adc module

    // initialize state machine
    kissm_init(KISSM_STATEREF(powerup));

    // forever loop
    for (;;)
    {
        struct kissm_event evt;
        packet_t *pkt;

        // run LED background
        led_run();

        // run ADC conversions
        // TODO: should be moved to an adc_run() method
        if (tmr_expired(adc_timeout))
        {
            adc_timeout += 1000;
            adc_sample();
        }

        // event generator
        // check for possible events in the system
        // check first for expiring timers, then incoming commands
        // default is NONE
        evt.type = KISSM_EVT_NONE;
        pkt = NULL;

        struct tmr *expired = tmr_process();    // any timers expired?
        if (expired)
        {
            // if state timeout timer, then generate timeout event
            if (expired->id == STATE_TMR)
            {
                evt.type = EVT_TIMEOUT;
            }
            // TODO: add else for other timers if any other timers are added
        }
        else    // if no timers, then check commands
        {
            pkt =  cmd_process();
            if (pkt)
            {
                evt.type = EVT_CMD;
                evt.data.p = pkt;
            }
        }

        // run the state machine
        kissm_run(&evt);
        if (pkt)
        {
            pkt_rx_free(pkt);
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
