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
 * BOOTLOAD command code
 *
 * Causes the node to enter boot loader mode.
 * */
#define CMD_BOOTLOAD 2

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

#ifdef __cplusplus
}
#endif

#endif
