/******************************************************************************
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2021 Joseph Kroesche
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

#ifndef __IOMAP_H__
#define __IOMAP_H__

/** @addtogroup iomap IOMAP
 *
 * Pin Assignments
 *
 * **1614**
 *
 * | Port | Pin | IO        | Function                              |
 * |------|-----|-----------|---------------------------------------|
 * |  A0  | 10  | dedicated | RSTn/UPDI                             |
 * |  A1  | 11  | dedicated | UART0 TX TXRX                         |
 * |  A2  | 12  | unused    | UART0 RX                              |
 * |  A3  | 13  | dig out   | EXTIO                                 |
 * |  A4  |  2  | analog in | ADC4 TSENSE                           |
 * |  A5  |  3  | dig out   | SPAREIO                               |
 * |  A6  |  4  | dig out   | GREEN                                 |
 * |  A7  |  5  | analog in | AIN7 VSENSE                           |
 * |  B0  |  9  | analog in | ADC11 EXTTEMP                         |
 * |  B1  |  8  | dig out   | LOADON            (W01)               |
 * |  B2  |  7  | dig out   | BLUE              (WO2)               |
 * |  B3  |  6  | dig out   | REFON                                 |
 *
 */

// all pin definitions are bit mask

#define TX_PORT         PORTA
#define TX_PIN          PIN1_bm
#define TX_DIR          (TX_PIN)

// RX pin is just connected to a test point
// since we are using half duplex, this pin can be spare GPIO
#define RX_PORT         PORTA
#define RX_PIN          PIN2_bm
#define RX_DIR          0       // input

#define EXTIO_PORT      PORTA
#define EXTIO_PIN       PIN3_bm
#define EXTIO_DIR       (EXTIO_PIN)

#define TSENSE_PORT     PORTA
#define TSENSE_PIN      PIN4_bm
#define TSENSE_DIR      0
#define TSENSE_ADC      4
#define TSENSE_PINCTRL  PIN4CTRL

#define SPAREIO_PORT    PORTA
#define SPAREIO_PIN     PIN5_bm
#define SPAREIO_DIR     0

#define GREEN_PORT      PORTA
#define GREEN_PIN       PIN6_bm
#define GREEN_DIR       (GREEN_PIN)

#define VSENSE_PORT     PORTA
#define VSENSE_PIN      PIN7_bm
#define VSENSE_DIR      0
#define VSENSE_ADC      7
#define VSENSE_PINCTRL  PIN7CTRL

#define EXTTEMP_PORT    PORTB
#define EXTTEMP_PIN     PIN0_bm
#define EXTTEMP_DIR     0
#define EXTTEMP_ADC     11
#define EXTTEMP_PINCTRL PIN0CTRL

#define LOADON_PORT     PORTB
#define LOADON_PIN      PIN1_bm
#define LOADON_DIR      (LOADON_PIN)

#define BLUE_PORT       PORTB
#define BLUE_PIN        PIN2_bm
#define BLUE_DIR        (BLUE_PIN)

#define REFON_PORT      PORTB
#define REFON_PIN       PIN3_bm
#define REFON_DIR       (REFON_PIN)

#define PORTADIR        (TX_DIR | EXTIO_DIR | GREEN_DIR)
#define PORTBDIR        (LOADON_DIR | BLUE_DIR | REFON_DIR)

#endif
