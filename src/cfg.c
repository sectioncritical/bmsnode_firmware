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

#include <avr/eeprom.h>
#include <util/crc16.h>

#include "cfg.h"


// configuration block type
// this will change if the configuration block is updated
#define CFG_TYPE_1 1

// where the configuration block is stored in EEPROM
#define CFG_ADDR ((void *)0)

// where the board data is stored in EEPROM
#define CFG_ADDR_UID ((void *)(0x200-4))
#define CFG_ADDR_BOARD_TYPE ((void *)(0x200-5))

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
config_t *cfg_load(void)
{
    static config_t cfg;
    eeprom_read_block(&cfg, CFG_ADDR, sizeof(cfg));
    // validate header items
    if ((cfg.type == CFG_TYPE_1) && (cfg.len == sizeof(cfg)))
    {
        uint8_t crc = cfg_compute_crc(&cfg);
        if (crc == cfg.crc)
        {
            return &cfg;
        }
    }

    // getting here means a check failed, populate with defaults
    cfg.addr = 0;
    return &cfg;
}

// store configuration provided by caller. the header fields are updated
// and the crc calculated before storing
void cfg_store(config_t *cfg)
{
    cfg->len = sizeof(config_t);
    cfg->type = CFG_TYPE_1;
    cfg->crc = cfg_compute_crc(cfg);
    eeprom_update_block(cfg, CFG_ADDR, sizeof(config_t));
}
