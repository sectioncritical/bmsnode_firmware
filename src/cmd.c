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

#include "pkt.h"
#include "cmd.h"


//////////
//
// See header file for public function API descriptions.
//
//////////

// hard code a node id for now
static uint8_t nodeid = 1;

static void(*swreset)(void) = 0;

// implement PING command
static bool cmd_ping(packet_t *pkt)
{
    return pkt_send(PKT_FLAG_REPLY, pkt->addr, pkt->cmd, NULL, 0);
}

// implement BOOTLOAD command
static bool cmd_bootload(packet_t *pkt)
{
    // TODO: put IO pins in safe state
    // TODO: should it send a reply? then it will need to wait until
    // message was sent before resetting
    MCUSR = 0;
    swreset();
    return false;
}

// run command processor
bool cmd_process(void)
{
    bool ret = false;
    packet_t *pkt = pkt_ready();

    if (pkt)
    {
        if (pkt->addr == nodeid)
        {
            switch (pkt->cmd)
            {
                case CMD_PING:
                    ret = cmd_ping(pkt);
                    break;

                case CMD_BOOTLOAD:
                    ret = cmd_bootload(pkt);
                    break;

                default:
                    ret = false;
                    break;
            }
        }
        pkt_rx_free(pkt);
    }
    return ret;
}
