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

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "pkt.h"
#include "ser.h"
#include "cfg.h"

// serial transmit buffer and pointers
static uint8_t headp = 0;
static uint8_t tailp = 0;
static uint8_t txbuf[32];

// convenience macros for managing the serial buffer
#define BUF_EMPTY() (headp == tailp)
#define BUF_NOTEMPTY() (headp != tailp)
#define BUF_CNT() ((headp - tailp) & 0x1F)
#define BUF_FULL() (BUF_CNT() == 31)
#define BUF_NOTFULL() (BUF_CNT() < 31)

// easy enable/disable of TX ISR
#define TXINT_ENABLE() (USART0.CTRLA |= USART_DREIE_bm | USART_TXCIE_bm)
#define TXINT_DISABLE() (USART0.CTRLA &= ~USART_DREIE_bm)
// for DISABLE, TXC is disabled separately

// The following comment is no longer true. With half duplex hardware, the
// received data is no longer copied by firmware to the transmit. Leaving
// the comment though because it may explain some legacy aspects of the code.
// DEPRECATED COMMENT
// the serial transmit buffer is used by app code that wants to send
// a packet. but it is also used by the RX interrupt to copy any incoming
// bytes to output. Therefore the TX buffer operations also need to be
// protected from RX interrupt. Hence the critical section setup below.
//
// The rxint enable/disable are functions for now, instead of macros
// for reasons. TODO: figure out if a macro can be used
// The rxint enable/disable functions are used with the CRITICAL_RX()
// macro

// TODO the original duplex switching code was written for the '841 and
// required enabling and disabling TX and RX at the appropriate times in order
// to make sure that RX did not receive any of the TX data (thus confusing the
// parser), and that TX was left enabled long enough to make sure all the bits
// were shifted out. To port to '1614 I just ported all this behaviour directly
// from the 841 peripheral to the equivalent registers and bits of the '1614.
// However, the 1614 has USART support for half duplex (one-wire) and will
// automatically switch the TX and RX signals as needed. By modifying the
// parser to reject response packets, then it would not even matter if the
// RX was copying all the TX bits. So, I think the RX/TX control and logic
// in this file could be simplified.

// enable receive ISR and return false
bool rxint_enable(void)
{
    USART0.CTRLA |= USART_RXCIE_bm;
    return false;
}

// disable receive interrupt and return true
bool rxint_disable(void)
{
    USART0.CTRLA &= ~USART_RXCIE_bm;
    return true;
}

// defines a critical section using a for loop trick that allows the
// critical section to be enclosed in braces
#define CRITICAL_RX() \
    for(bool _crit=rxint_disable(); \
        _crit; \
        _crit=rxint_enable())

//////////
//
// See header file for public function API descriptions.
//
//////////

// determine if serial hardware is active
// criteria:
// - tx sw buffer is not  empty
// - tx UDRE interrupt is enabled (which means there is ongoing tx operation)
// - tx TXC interrupt is enabled (tx byte in progress)
// - rx is not empty (incoming bytes to process)
//
// This should be coupled with checking the packet processor as well
//
bool ser_is_active(void)
{
    bool b_isactive = false;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (BUF_NOTEMPTY()
         || (USART0.CTRLA & USART_DREIE_bm)
         || (USART0.CTRLA & USART_TXCIE_bm)
         || (USART0.STATUS & USART_RXCIF_bm))
        {
            b_isactive = true;
        }
    }
    return b_isactive;
}

// write data to the serial output
// TODO: consider all or nothing write, instead of partial when there
// is not enough room in the buffer
//
// NOTE: this starts transmitting. so assume there is not any contention on
// the serial bus (nothing being received)
uint8_t ser_write(uint8_t *buf, uint8_t len)
{
    uint8_t cnt = 0;
    CRITICAL_RX()
    {
        while (BUF_NOTFULL() && len--)
        {
            txbuf[headp++] = buf[cnt++];
            headp &= 0x1F;
        }
        // switch duplex
        USART0.CTRLB &= ~USART_RXEN_bm; // disable RX function
        USART0.CTRLB |= USART_TXEN_bm;  // enable TX function
        TXINT_ENABLE(); // will kick off serial transmit
    }
    return cnt;
}

// flush the serial buffer and reset internals
void ser_flush(void)
{
    CRITICAL_RX()
    {
        TXINT_DISABLE();
        USART0.CTRLA &= ~USART_TXCIE_bm;
        headp = 0;
        tailp = 0;
    }
    // CRITICAL_RX() has the side effect of leaving the RX interrupt enabled
}

// serial RX interrupt handler
// called whenever a received byte is available
ISR(USART0_RXC_vect)
{
    // read uart status
    uint8_t flags = USART0.STATUS;

    // check for rx received
    if (flags & USART_RXCIF_bm)
    {
        // read character from uart
        uint8_t ch = USART0.RXDATAL;

        // process bytes into packets
        pkt_parser(ch);
    }
}

// serial TX interrupt handler
// called whenever there is space in the outgoing data buffer
ISR(USART0_DRE_vect)
{
    // get uart status
    uint8_t flags = USART0.STATUS;

    // check for data in buffer to send
    if (BUF_NOTEMPTY())
    {
        // make sure uart tx buffer is free (this should always be true)
        if (flags & USART_DREIF_bm)
        {
            // read byte from buffer and write to uart TX
            // clear TXC0 so that it can be used to detect tx complete
            USART0.STATUS |= USART_TXCIF_bm;
            USART0.TXDATAL = txbuf[tailp++];
            tailp &= 0x1F;
        }
    }
    // there was no data to send so turn off the TX interrupt
    else
    {
        TXINT_DISABLE();
    }

    // TXC interrupt is still enabled so there is still one more interrupt
    // once the last byte is completely shifted out
}

ISR(USART0_TXC_vect)
{
    // on entry here, the last byte should have been shifted out and
    // there is no more data to send.
    // To be sure, double check the TX buffer and make sure there is not
    // any other data to send before shutting down the UART TX
    if (BUF_NOTEMPTY())
    {
        // re-enable the TX int and the handler will be invoked
        TXINT_ENABLE();
        // just return from here
    }

    // nothing else to send, as expected
    else
    {
        // disable the TXC interrupt
        // this indicates to serial module that TX no longer in use
        USART0.CTRLA &= ~USART_TXCIE_bm;

        // disable UART TX, and enable UART RX
        USART0.CTRLB &= ~USART_TXEN_bm;
        USART0.CTRLB |= USART_RXEN_bm;
    }
}

//////////
//
// Used for unit test to give test code access to internal state
//
#ifdef UNIT_TEST
struct serinternals
{
    uint8_t *pheadp;
    uint8_t *ptailp;
    uint8_t *ptxbuf;
    uint8_t bufsize;
} serial_internals = { &headp, &tailp, txbuf, 32 };
#endif
