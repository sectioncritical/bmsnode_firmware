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
#include <util/crc16.h>
#include <util/atomic.h>

#include "pkt.h"

/*
 * |Byte| Field  | Description                              |
 * |----|--------|------------------------------------------|
 * | -2 |Preamble| One or more bytes used to wake up devices and establish sync|
 * | -1 | Sync   | Sync byte to indicate start of packet    |
 * |  0 | Flags  | TBD flags to indicate features of packet: broadcast, reply, etc|
 * |  1 | Address| Node address for packet                  |
 * |  2 | Cmd/Rsp| command ID                               |
 * |  3 | Length | payload length in bytes (can be 0)       |
 * |  4+| Payload| variable payload contents (can be none)  |
 * |  N | CRC    | 8-bit CRC                                |
 */

#define PKT_PREAMBLE 0x55
#define PKT_SYNC 0xF0
#define PKT_GET_LEN(buf) ((buf)[3])

// parser state machine states
typedef enum
{
    RX_SEARCH,
    RX_SYNC,
    RX_HEADER,
    RX_DATA,
    RX_CHECK
} rx_state_t;

static rx_state_t state = RX_SEARCH;

// indicates single RX buffer is allocated
static bool rxbuf_inuse = false;

// buffer used for RX packets
static uint8_t rxbuf[16];

// storage for received packet waiting for pickup
static packet_t *ready_packet = NULL;

//////////
//
// See header file for public function API descriptions.
//
//////////

// Reset the packet parser internal state.
void pkt_reset(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        state = RX_SEARCH;
        rxbuf_inuse = false;
        ready_packet = NULL;
    }
}

// Release an RX packet buffer for re-use.
void pkt_rx_free(packet_t *pktbuf)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        rxbuf_inuse = false;
    }
}

// Allocate RX packet buffer.
packet_t *pkt_rx_alloc(void)
{
    packet_t *ret;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (!rxbuf_inuse)
        {
            rxbuf_inuse = true;
            ret = (packet_t *)rxbuf;
        }
        else
        {
            ret = NULL;
        }
    }
    return ret;
}

// return a received packet
packet_t *pkt_ready(void)
{
    packet_t *ret;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        ret = ready_packet;
        ready_packet = NULL;
    }
    return ret;
}

// Process next byte in stream and parse packets.
void pkt_parser(uint8_t nextbyte)
{
    static uint8_t idx;
    static uint8_t crc;
    static uint8_t len;
    static uint8_t *pbuf;

    // process packet state machine
    switch (state)
    {
        // searching for preamble
        case RX_SEARCH:
            if (nextbyte == PKT_PREAMBLE)
            {
                state = RX_SYNC;
            }
            break;

        // receiving preamble bytes, waiting for sync byte
        case RX_SYNC:
            // waiting for sync byte
            if (nextbyte == PKT_SYNC)
            {
                // get buffer to store incoming packet
                pbuf = (uint8_t *)pkt_rx_alloc();
                if (pbuf)
                {
                    state = RX_HEADER;
                    idx = 0;
                    crc = 0;
                }
                // if no buffer is available, ignore this packet and go
                // back to search for next
                else
                {
                    state = RX_SEARCH;
                }
            }
            // if its not a sync, it should be preamble
            // if not, then go back to search
            else if (nextbyte != PKT_PREAMBLE)
            {
                state = RX_SEARCH;
            }
            break;

        // read in header bytes
        case RX_HEADER:
            crc = _crc8_ccitt_update(crc, nextbyte);
            pbuf[idx] = nextbyte;
            ++idx;
            // all header bytes received
            if (idx == PKT_HEADER_LEN)
            {
                // get the length and validate it
                len = PKT_GET_LEN(pbuf);

                // special case, if len is 0 then no payload, do crc
                if (len == 0)
                {
                    state = RX_CHECK;
                }
                // if len is too big, then abandon this packet
                else if (len > PKT_PAYLOAD_LEN)
                {
                    pkt_rx_free((packet_t*)pbuf);
                    state = RX_SEARCH;
                }
                // length field appear valid
                // read in payload bytes
                else
                {
                    state = RX_DATA;
                }
            }
            break;

        // put incoming bytes into payload buffer until `len` bytes
        // have been stored
        case RX_DATA:
            crc = _crc8_ccitt_update(crc, nextbyte);
            pbuf[idx] = nextbyte;
            ++idx;
            --len;
            if (len == 0)
            {
                state = RX_CHECK;
            }
            break;

        // last byte of packet is crc. compare it to the computed crc
        // for the incoming bytes
        case RX_CHECK:
            state = RX_SEARCH;
            if (nextbyte == crc)
            {
                // good packet, save for client
                ready_packet = (packet_t *)pbuf;
            }
            else
            {
                // bad packet, abandon
                pkt_rx_free((packet_t *)pbuf);
            }
            break;

        default:
            state = RX_SEARCH;
            break;
    }
}
