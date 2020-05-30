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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Structure to hold the BMS Node configuration data.
 *
 * @note The fields marked _(private)_ are maintained by the "cfg" module
 * and should not be used by the client.
 */
typedef struct __attribute__ ((__packed__))
{
    uint8_t   len;      ///< (private) total length of config structure
    uint8_t   type;     ///< (private) structure type/version ID
    uint8_t   addr;     ///< the BMSNode bus address
    uint16_t  vscale;   ///< voltage calibration scaler
    int16_t   voffset;  ///< voltage calibration offset
    uint16_t  tscale;   ///< temperature calibration scaler (TBD)
    int16_t   toffset;  ///< temperature calibration offset (TBD)
    uint16_t  xscale;   ///< external sensor calibration scaler (TBD)
    int16_t   xoffset;  ///< external sensor calibration offset (TBD)
    uint16_t  shunton;  ///< shunt turn-on millivolts
    uint16_t  shuntoff; ///< shunt turn-off millivolts (should be <= shunton)
    uint16_t  shunttime;///< shunt inactivity timeout in seconds
    int8_t    temphi;   ///< temperature regulation upper limit in C
    int8_t    templo;   ///< temperature regulation lower limit in C  (should be < temphi)
    uint16_t  tempadj;  ///< TBD temperature regulation algorithm factor
    uint8_t   crc;      ///< (private) structure CRC for non-volatile storage
} config_t;

/**
 * Global system configuration.
 *
 * This is where the configuration parameters are stored. This is populated
 * by calling cfg_load() and other modules can access structure members
 * directly. cfg_store() will cause the values in this structure to be stored
 * to permanent memory. However, the proper way to modify the structure is to
 * use cfg_set() followed by cfg_store() to commit the changes.
 */
extern config_t g_cfg_parms;

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
 * The configuration will be read from persistent memory and stored into the
 * global configuration \ref g_cfg_parms. If the stored configuration is not
 * present or not valid, then the global configuration will be populated with
 * default values.
 *
 * After calling this function, the configuration parameters can be accessed
 * directly using \ref g_cfg_parms.
 *
 * @note this function should be called at least once during system init.
 *
 * @return `true` if a configuration was loaded from persistent memory, or
 * `false` if defaults were loaded.
 */
extern bool cfg_load(void);

/**
 * Stores the global configuration to persistent memory.
 */
extern void cfg_store(void);

/**
 * Set a parameter value from a payload buffer.
 *
 * @param len the payload length of p_value (includes id byte)
 * @param p_value payload buffer that holds parameter ID and value bytes
 *
 * This function takes a parameter ID and value from a payload buffer and
 * stores it into the global configuration. The buffer format is the same as
 * the payload of a SETPARM command. This allows the command processor to pass
 * the payload directly.
 *
 * The first byte holds the parameter ID and the remaining bytes hold the
 * value, which depends on which parameter it is.
 *
 * @note the permanent configuration is not updated. The client must call
 * cfg_store() in order to commit any changes.
 *
 * @return `true` if the parameter was updated, `false` if there is an error
 * such as invalid parameter ID.
 */
extern bool cfg_set(uint8_t len, uint8_t *p_value);

/**
 * Get a parameter value from the global configuration, into a payload buffer.
 *
 * @param len the maximum length of the passed payload buffer p_buf
 * @param p_buf a payload buffer to use for storing the parameter value
 *
 * This function will get the value of a configuration parameter and store it
 * into the caller-supplied payload buffer. The buffer format is the same as
 * the payload of a GETPARM command. This allows the command processor to
 * use the result directly as a payload for a reply packet.
 *
 * The first byte of the buffer should be pre-populated with the parameter
 * ID by the caller.
 *
 * @return the number of bytes in the buffer, including the parameter ID and
 * parameter value bytes. This is the same value to be used as the payload
 * length. If there is any problem with input parameters, the zero (0) will
 * be returned.
 */
extern uint8_t cfg_get(uint8_t len, uint8_t *p_buf);

#ifdef __cplusplus
}
#endif

#endif

/** @} */
