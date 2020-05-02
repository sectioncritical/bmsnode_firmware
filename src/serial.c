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
#define TXINT_ENABLE() (UCSR0B |= _BV(UDRIE0))
#define TXINT_DISABLE() (UCSR0B &= ~_BV(UDRIE0))

// the serial transmit buffer is used by app code that wants to send
// a packet. but it is also used by the RX interrupt to copy any incoming
// bytes to output. Therefore the TX buffer operations also need to be
// protected from RX interrupt. Hence the critical section setup below.
//
// The rxint enable/disable are functions for now, instead of macros
// for reasons. TODO: figure out if a macro can be used
// The rxint enable/disable functions are used with the CRITICAL_RX()
// macro

// enable receive ISR and return false
bool rxint_enable(void)
{
    UCSR0B |= _BV(RXCIE0);
    return false;
}

// disable receive interrupt and return true
bool rxint_disable(void)
{
    UCSR0B &= ~_BV(RXCIE0);
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
// - tx interrupt is enabled (which means there is ongoing tx operation)
// - txc complete flag is not set (tx byte in progress)
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
         || (UCSR0B & _BV(UDRIE0))
         || !(UCSR0A & _BV(TXC0))
         || (UCSR0A & _BV(RXC0)))
        {
            b_isactive = true;
        }
    }
    return b_isactive;
}

// write data to the serial output
// TODO: consider all or nothing write, instead of partial when there
// is not enough room in the buffer
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
        TXINT_ENABLE();
    }
    return cnt;
}

// flush the serial buffer and reset internals
void ser_flush(void)
{
    CRITICAL_RX()
    {
        TXINT_DISABLE();
        headp = 0;
        tailp = 0;
    }
}

// serial RX interrupt handler
// called whenever a received byte is available
ISR(USART0_RX_vect)
{
    // read uart status
    uint8_t flags = UCSR0A;

    // check for rx received
    if (flags & _BV(RXC0))
    {
        // first, propogate the bytes down the bus, no matter what else
        // is going on
        uint8_t ch = UDR0;

        // stuff byte into tx buffer
        if (BUF_NOTFULL())
        {
            txbuf[headp++] = ch;
            headp &= 0x1F;
            TXINT_ENABLE();
        }

        // process bytes into packets
        pkt_parser(ch);
    }
}

// serial TX interrupt handler
// called whenever there is space in the outgoing data buffer
ISR(USART0_UDRE_vect)
{
    // get uart status
    uint8_t flags = UCSR0A;

    // check for data in buffer to send
    if (BUF_NOTEMPTY())
    {
        // make sure uart tx buffer is free (this should always be true)
        if (flags & _BV(UDRE0))
        {
            // read byte from buffer and write to uart TX
            // clear TXC0 so that it can be used to detect tx complete
            UCSR0A &= ~_BV(TXC0);
            UDR0 = txbuf[tailp++];
            tailp &= 0x1F;
        }
    }
    // there was no data to send so turn off the TX interrupt
    else
    {
        TXINT_DISABLE();
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
