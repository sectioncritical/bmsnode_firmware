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

#include "pkt.h"
#include "cmd.h"
#include "cfg.h"

//////////
//
// See header file for public function API descriptions.
//
//////////

// global node configuration
// this is assumed to be set to non-NULL in main before anything
// else runs
extern config_t *nodecfg;

#define NODEID (nodecfg->addr)

typedef union
{
    uint32_t u32;
    uint8_t u8[4];
} u32buf_t;

static void(*swreset)(void) = 0;

// implement PING command
static bool cmd_ping(void)
{
    return pkt_send(PKT_FLAG_REPLY, NODEID, CMD_PING, NULL, 0);
}

// implement DFU command
static bool cmd_dfu(void)
{
    // TODO: put IO pins in safe state
    // TODO: should it send a reply? then it will need to wait until
    // message was sent before resetting
    // optiboot expects interrupts to be disabled
    cli();
    MCUSR = 0;
    swreset();
    return false;
}

// implement UID command
static bool cmd_uid(void)
{
    uint8_t pld[5];
    u32buf_t uid;
    uid.u32 = cfg_uid();
    pld[0] = uid.u8[0];
    pld[1] = uid.u8[1];
    pld[2] = uid.u8[2];
    pld[3] = uid.u8[3];
    pld[4] = cfg_board_type();
    return pkt_send(PKT_FLAG_REPLY, NODEID, CMD_UID, pld, 5);
}

// implement ADDR command
static bool cmd_addr(packet_t *pkt)
{
    uint32_t uid = cfg_uid();
    u32buf_t pktuid;
    pktuid.u8[0] = pkt->payload[0];
    pktuid.u8[1] = pkt->payload[1];
    pktuid.u8[2] = pkt->payload[2];
    pktuid.u8[3] = pkt->payload[3];
    // compare UID in packet to our UID
    if (uid == pktuid.u32)
    {
        // if the same, then update our address and store it
        nodecfg->addr = pkt->addr;
        cfg_store(nodecfg);
        return pkt_send(PKT_FLAG_REPLY, NODEID, CMD_ADDR, pkt->payload, 4);
    }
    // only send reply if the UID matches
    return false;
}

// run command processor
bool cmd_process(void)
{
    bool ret = false;
    packet_t *pkt = pkt_ready();

    // valid packet
    if (pkt)
    {
        // process ADDR command for any address
        if (pkt->cmd == CMD_ADDR)
        {
            ret = cmd_addr(pkt);
        }
        // special handling if our nodeid is not set
        else if (NODEID == 0)
        {
            // if UID command with addr==0 and we have no nodeid assigned
            if ((pkt->addr == 0) && (pkt->cmd == CMD_UID))
            {
                // we can respond to address 0 in this case
                ret = cmd_uid();
            }
        }
        // we have a nodeid so process normally
        else if (pkt->addr == NODEID)
        {
            switch (pkt->cmd)
            {
                case CMD_PING:
                    ret = cmd_ping();
                    break;

                case CMD_DFU:
                    ret = cmd_dfu();
                    break;

                case CMD_UID:
                    ret = cmd_uid();
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
