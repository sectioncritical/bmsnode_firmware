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
#include <avr/cpufunc.h>

#include <util/atomic.h>

#define F_CPU 10000000UL
#ifndef BAUDRATE
#define BAUDRATE 9600UL
#endif

#include <util/delay.h>

// per data sheet, calculation of BAUD rate register (16-bit) follows:
//
// BAUDREG = (64 * Fclk) / (16 * Fbaud)
// (reduces to)
// BAUDREG = (4 * Fclk) / Fbaud
#define BAUDREG ((uint16_t)(((float)((F_CPU) * 4UL) / (float)(BAUDRATE)) + 0.5))

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
#include "iomap.h"

/*
 * Pin Assignments
 *
 * see iomap.h
 *
 */

void device_init(void)
{
    // at reset, MCU is running from 20 MHz oscillator with /6 ==> 3.3 MHz
    // change the prescaler so it runs at 10 MHz
    ccp_write_io((void *)&(CLKCTRL.MCLKCTRLB), CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm);

    // set up IO
    // this sets up GPIOs. alt functions are set up with peripheral inits
    PORTA.OUT = 0; // set all outputs low, to get known initial state
    PORTB.OUT = 0;
    PORTA.DIR = PORTADIR;      // from iomap.h
    PORTB.DIR = PORTBDIR;

    // enable the USART0 alt pin muxing to get to the pins we want to use
    PORTMUX.CTRLB = PORTMUX_USART0_ALTERNATE_gc;

    // set up UART0
    // bmsnode style boards (shared tx/rx)
    // set up for one-wire half duplex
    // enables RX, RX interrupt only. leave TX disabled until needed
    // not clear we need pullup on TX since there is board pullup
    // PORTA.PIN1CTRL = PORT_PULLUPEN_bm;  // enable pullup on TX pin
    USART0.CTRLA = USART_LBME_bm | USART_RXCIE_bm;  // enable loopback and RX int
    USART0.BAUD = BAUDREG;  // set data rate
    // UART protocol 8,n,1 is already the default at boot
    USART0.CTRLB = USART_TXEN_bm | USART_RXEN_bm | USART_ODME_bm | USART_SFDEN_bm; // enable, open-drain
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
            wdt_reset();    // pet the watchdog
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
            wdt_reset();    // pet the watchdog
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
            wdt_reset();    // pet the watchdog
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
            led_blink(LED_BLUE, 800, 200);
            shunt_start();  // start the shunt module running
            break;
        }

        // if SHUNTOFF command then signal shunt module to stop
        case EVT_CMD:
        {
            uint8_t cmd = ((packet_t *)p_event->data.p)->cmd;;
            if (cmd == CMD_SHUNTOFF)
            {
                shunt_stop();
                p_ns = KISSM_STATEREF(idle);
            }
        }

        case KISSM_EVT_EXIT:
        {
            break;
        }

        default:
        {
            // run the shunt module (must be called continuously)
            enum shunt_status sts = shunt_run();
            wdt_reset();        // prevent watchdog reset
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
            led_blink(LED_BLUE, 200, 800);
            break;
        }

        case KISSM_EVT_EXIT:
        {
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
            // turn off watchdog timer
            RSTCTRL.RSTFR = 0;  // not sure if this is needed for '1614
            wdt_disable();
            adc_powerdown();    // shut down analog circuits

            // force LEDs and external outputs off
            led_off(LED_BLUE);
            led_off(LED_GREEN);
            EXTIO_PORT.OUTCLR = EXTIO_PIN;
            LOADON_PORT.OUTCLR = LOADON_PIN;
            REFON_PORT.OUTCLR = REFON_PIN;

            // go to sleep
            set_sleep_mode(SLEEP_MODE_STANDBY);
            //set_sleep_mode(SLEEP_MODE_PWR_DOWN);
            sleep_mode();

            // at this point MCU is sleep/pwrdn
            // when it wakes it resumes execution here

            break;
        }

        case KISSM_EVT_EXIT:
        {
            // WAKING UP
            // turn on blue LED to show we wake up
            led_on(LED_BLUE);

            // re-enable the analog
            adc_powerup();

            // re-enable watchdog
            wdt_enable(WDTO_1S);

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
    // interrupts and watchdog should be disabled on entry

    device_init();  // hardware init
    cfg_load();     // load node config (global cfg needed for later calls)

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

    // initialize state machine
    kissm_init(KISSM_STATEREF(powerup));

    // enable the watchdog while state machine is running
    wdt_enable(WDTO_1S);

    // forever loop
    for (;;)
    {
        struct kissm_event evt;
        packet_t *pkt;

        // run LED background
        led_run();

        // run ADC conversions
        adc_run();

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
