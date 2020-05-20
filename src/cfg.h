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

#ifndef __CFG_H__
#define __CFG_H__

/** @addtogroup cfg Config
 *
 * @{
 */

/**
 * Structure to hold the BMS Node configuration data.
 *
 * @note The fields marked _(private)_ are maintained by the "cfg" module
 * and should not be used by the client.
 */
typedef struct __attribute__ ((__packed__))
{
    uint8_t len;    //< (private) total length of config structure
    uint8_t type;   //< (private) structure type/version ID
    uint8_t addr;   //< the BMSNode bus address
    uint8_t crc;    //< (private) structure CRC for non-volatile storage
} config_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get the board unique ID.
 *
 * This value is set during board production.
 *
 * @return the board unique ID as a 32-bit integer
 */
extern uint32_t cfg_uid(void);

/**
 * Get the board type.
 *
 * This value is set during board production.
 *
 * @return the board type as 8-bit integer.
 */
extern  uint8_t cfg_board_type(void);

/**
 * Load the node configuration from persistent memory.
 *
 * The configuration will be read from persistent memory. If the stored
 * configuration is not present or not valid, then defaults will be returned.
 * The returned structure pointer will remain valid until the next call to
 * this function.
 *
 * @return A pointer to a `config_t` structure that holds the node
 * configuration.
 */
extern config_t *cfg_load(void);

/**
 * Stores the node configuration to persistent memory.
 */
extern void cfg_store(config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif

/** @} */
