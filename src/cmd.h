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

#ifndef __CMD_H__
#define __CMD_H__

/**
 * PING command code
 *
 * Causes a ping reply packets to be sent.
 */
#define CMD_PING 1

/**
 * DFU command code
 *
 * Causes the node to enter DFU (firmware update) mode.
 */
#define CMD_DFU 2

/**
 * UID command code
 *
 * Causes node to report UID and board type.
 */
#define CMD_UID 3

/**
 * ADDR command code
 *
 * Sets device bus address
 */
#define CMD_ADDR 4

/**
 * ADCRAW command code
 *
 * Reads ADC raw sample data
 */
#define CMD_ADCRAW 5

/**
 * STATUS command code
 *
 * Returns BMS Node status (such as voltage, temp, etc. - see spec)
 */
#define CMD_STATUS 6

/**
 * SHUNTON command code
 *
 * Turns on cell shunting (balancing mode).
 */
#define CMD_SHUNTON 7

/**
 * SHUNTOFF command code
 *
 * Turns off cell shunting (balancing mode).
 */
#define CMD_SHUNTOFF 8

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Run the command processor.
 *
 * This function should be called periodically from the main loop. It checks
 * for any available incoming packets. It checks if the packets is intended
 * for this node. If so then it dispatches the command for further processing
 * according to its function.
 *
 * @return returns `true` if a command was processed and `false` if not.
 */
extern bool cmd_process(void);

/**
 * Get the last command that was processed.
 *
 * This function can be called to discover that last command procedded by
 * the command processor. This can be used by the main app loop to adjust
 * operating state based on commands to this node. The last command will be
 * cleared once this function is called.
 *
 * @return the command code of the last processed command, or 0 if there is
 * no recent command.
 */
extern uint8_t cmd_get_last(void);

#ifdef __cplusplus
}
#endif

#endif
