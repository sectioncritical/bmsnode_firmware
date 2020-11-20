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
#include <stdint.h>
#include <stdbool.h>

#include <avr/eeprom.h>
#include <util/crc16.h>

#include "cfg.h"


// configuration block type
// this will change if the configuration block is updated
#define CFG_TYPE_1 1
#define CFG_TYPE_2 2

// version 1 config block
// if needed for upgrades
typedef struct __attribute__ ((__packed__))
{
    uint8_t   len;
    uint8_t   type;
    uint8_t   addr;
    uint8_t   crc;
} config_v1_t;

// where the configuration block is stored in EEPROM
#define CFG_ADDR ((void *)0)

// where the board data is stored in EEPROM
#define CFG_ADDR_UID ((void *)(0x200-4))
#define CFG_ADDR_BOARD_TYPE ((void *)(0x200-5))

// global system configuration is kept here
config_t g_cfg_parms;

// global to hold board type
uint8_t g_board_type;

// compute crc of a configuration block
// this assumes that the length field is correct
static uint8_t cfg_compute_crc(config_t *cfg)
{
    uint8_t crc = 0;

    for (uint8_t idx = 0; idx < cfg->len - 1; ++idx)
    {
        crc = _crc8_ccitt_update(crc, ((uint8_t *)cfg)[idx]);
    }
    return crc;
}

//////////
//
// See header file for public function API descriptions.
//
//////////

// retrieve and return the board unique ID
uint32_t cfg_uid(void)
{
    return eeprom_read_dword(CFG_ADDR_UID);
}

// retrieve and return the board type
uint8_t cfg_board_type(void)
{
    return eeprom_read_byte(CFG_ADDR_BOARD_TYPE);
}

// read configuration from eeprom and validate it
// if the header is not correct or the CRC does not match, then
// it is loaded with default values before returning
// the retrieved data is saved in global g_cfg_parms if successful
// and returns true. if there is any error, then it is populated with
// defaults and returns false
bool cfg_load(void)
{
    // first, update the board type into the global variable, which
    // is used elsewhere for conditional execution
    g_board_type = cfg_board_type();

    // there are a small number of boards with v1 config block that
    // already have addresses programmed. We could put code here to
    // recognize those and upgrade the config block preserving the address.
    // but it adds some code  for a very limited contingency.
    // instead, save the code space and just reprogram that small number
    // of existing boards.
    // Eventually however, if the config block is updated again, then we
    // may need to deal with the ability to upgrade a config block

    // read the block (whatever is there) from permanent eeprom
    eeprom_read_block(&g_cfg_parms, CFG_ADDR, sizeof(config_t));

    // compute the crc for whatever was read in (v1 or v2)
    uint8_t crc = cfg_compute_crc(&g_cfg_parms);

    // validate header item (only allow v2 at this time)
    if ((g_cfg_parms.type == CFG_TYPE_2) && (g_cfg_parms.len == sizeof(config_t)))
    {
        if (crc == g_cfg_parms.crc)
        {
            return true;
        }
    }

    // getting here means a check failed, populate with defaults
    // TODO: this can possibly be made more memory efficient, perhaps
    // storing init values in code mem and doing a copy
    g_cfg_parms.addr = 0;
    g_cfg_parms.vscale = 4400;
    g_cfg_parms.voffset = 0;
    g_cfg_parms.tscale = 0;
    g_cfg_parms.toffset = 0;
    g_cfg_parms.xscale = 0;
    g_cfg_parms.xoffset = 0;
    g_cfg_parms.shuntmax = 4100;;
    g_cfg_parms.shuntmin = 4000;
    g_cfg_parms.shunttime = 300; // 5 minutes
    g_cfg_parms.temphi = 50;
    g_cfg_parms.templo = 40;
    g_cfg_parms.tempadj = 0;

    return false;
}

// commit global configuration back to eeprom
// the header fields are updated
// and the crc calculated before storing
void cfg_store(void)
{
    g_cfg_parms.len = sizeof(config_t);
    g_cfg_parms.type = CFG_TYPE_2;
    g_cfg_parms.crc = cfg_compute_crc(&g_cfg_parms);
    eeprom_update_block(&g_cfg_parms, CFG_ADDR, sizeof(config_t));
}

typedef struct
{
    uint8_t index;
    uint8_t count;
} parm_entry_t;

static const parm_entry_t parmtable[] =
{
    { 0, 0 },   // 0 - none
    { 2, 1 },   // 1 - addr
    { 3, 2 },   // 2 - vscale
    { 5, 2 },   // 3 - voffset
    { 7, 2 },   // 4 - tscale
    { 9, 2 },   // 5 - toffset
    { 11, 2 },  // 6 - xscale
    { 13, 2 },  // 7 - xoffset
    { 15, 2 },  // 8 - shunton
    { 17, 2 },  // 9 - shuntoff
    { 19, 2 },  // 10 - shunttime
    { 21, 1 },  // 11 - temphi
    { 22, 1 },  // 12 - templo
    { 23, 2 },  // 13 - tempadj
};
#define MAX_PARMID 13

bool cfg_set(uint8_t len, uint8_t *p_value)
{
    uint8_t id = p_value[0];
    if ((id < 1) || (id > MAX_PARMID))
    {
        // id does not appear legit
        return false;
    }

    else if (parmtable[id].count != (len - 1))
    {
        // length of payload doesnt match expected count for this parm ID
        return false;
    }

    // appears to have legit parm ID and length
    else
    {
        uint8_t cnt = parmtable[id].count;
        uint8_t idx = parmtable[id].index;
        uint8_t *p_cfg = (uint8_t *)&g_cfg_parms;
        
        // copy bytes from input directly into parms structure
        // since, for now, they are all either 1 or 2 bytes, we only
        // need to check for 2
        // cppcheck-suppress[objectIndex]
        p_cfg[idx] = p_value[1]; // first byte of value
        if (cnt == 2)
        {
            // copy second byte if there is one
            // cppcheck-suppress[objectIndex]
            p_cfg[idx + 1] = p_value[2];
        }

        // TODO: make this configurable
        cfg_store();    // update the persistent config store

        return true;
    }
}

// pass in buffer for storing result, and max len of buffer
// first element of pass in buffer is ID
// populates remaining bytes with parm value
// returns length of ID plus value bytes (ie payload length for packet)
// returns 0 if there is a problem
uint8_t cfg_get(uint8_t len, uint8_t *p_buf)
{
    // we are going to save some code and not check for NULL pointer
    // TODO add asserts for stuff like this

    uint8_t id = p_buf[0];

    if ((id < 1) || (id > MAX_PARMID))
    {
        // id does not appear legit
        return 0;
    }

    if (parmtable[id].count >= len)
    {
        // input buffer is not big enough to hold parameter value
        return 0;
    }

    // appears to be legit ID and buffer is big enough
    else
    {
        uint8_t cnt = parmtable[id].count;
        uint8_t idx = parmtable[id].index;
        uint8_t *p_cfg = (uint8_t *)&g_cfg_parms;

        // p_buf[0] already has the parm ID, just leave it there
        // copy bytes from parms structure into the caller-buffer
        // since, for now, they are all either 1 or 2 bytes, we only
        // need to check for 2
        // cppcheck-suppress[objectIndex]
        p_buf[1] = p_cfg[idx]; // first byte of value
        if (cnt == 2)
        {
            // copy second byte if there is one
            // cppcheck-suppress[objectIndex]
            p_buf[2] = p_cfg[idx + 1];
        }
        return cnt + 1; // payload length (includes parm id)
    }
}
