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

#ifndef __PKT_H__
#define __PKT_H__

/**
 * @addtogroup pkt Packet
 *
 * @{
 */

/**
 * Number of bytes in packet header
 */
#define PKT_HEADER_LEN 4

/**
 * Maximum number of payload bytes
 */
#define PKT_PAYLOAD_LEN 12

/**
 * BMS Node Packet Format
 */
typedef struct
{
    uint8_t flags;  //!< flags definition TBD
    uint8_t addr;   //!< node address (0, 255 - reserved)
    uint8_t cmd;    //!< packet command code
    uint8_t len;    //!< payload length
    uint8_t payload[PKT_PAYLOAD_LEN];   //!< data bytes (variable length)
    uint8_t crc;    //!< CRC over header and data
} packet_t;

#define PKT_FLAG_REPLY 0x80 //< indicates reply packet
#define PKT_FLAG_INIT 0x40  //< node init packet

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reset the packet parser internal state.
 *
 * The parser will be placed in the initial state. Any in-progress packet
 * will be discarded and remaining incoming packet contents will be ignored
 * until the valid start of the next packet. If a packet was obtained by a
 * client from pkt_parser() then that packet will be released immediately
 * and should no longer be used.
 *
 * @note This function is mainly intended for testing. If it is not called by
 * production firmware, then the linker will remove it from the final image.
 */
extern void pkt_reset(void);

/**
 * Determine if packet processor is active.
 *
 * Checks internal state of packet processor for ongoing activity. This
 * function checks for any incoming packets in process and any packets not yet
 * handled.
 *
 * @return `true` if the packet processor is active at the moment.
 *
 * @note the return state is valid only for the moment that it is called. A
 * new interrupt could occur in the next instruction cycle after the return.
 * It is up to the caller to ensure interrupt enablement is handled in a safe
 * way prior to putting the MCU to sleep.
 */
extern bool pkt_is_active(void);

/**
 * Release an RX packet buffer for re-use.
 *
 * @param pkt pointer to packet to free
 *
 * Marks the RX packet buffer as available for re-use so that another
 * incoming packet can be processed. This should be called by the client
 * of pkt_parser() once it no longer needs the buffer.
 */
extern void pkt_rx_free(packet_t *pkt);

/**
 * Allocate RX packet buffer.
 *
 * This function will return a pointer to a buffer that can be used for
 * storing incoming data packets.
 *
 * @return pointer to a packet buffer or NULL if there is no buffer available.
 *
 * @note This is not a general allocation function. It returns a buffer from
 * a very limited pool of available buffers specifically for use by the packet
 * parser.
 */
extern packet_t *pkt_rx_alloc(void);

/**
 * Get a received packet that is ready.
 *
 * @return A valid packet that has been received or NULL if there is no new
 * available packet.
 */
extern packet_t *pkt_ready(void);

/**
 * Assemble a packet and send it.
 *
 * @param flags flags field TBD
 * @param addr packet address
 * @param cmd packet command
 * @param payload pointer to buffer containing payload data
 * @param len length of payload data (can be 0)
 *
 * This function will assemble a packet, along with the computed CRC and
 * the preamble and sync bytes, and transmit it on the serial port. Upon
 * return, all data will have been copied into an outgoing buffer, so the
 * payload data buffer can be reused. The outgoing packet is queued, but will
 * still be in progress when this function returns.
 *
 * @return `true` if the packet was copied to output, `false` if not. A return
 * value of false means that the packet could not fit in the serial transmit
 * buffer.
 */
extern bool pkt_send(uint8_t flags, uint8_t addr, uint8_t cmd,
                     uint8_t *payload, uint8_t len);

/**
 * Process next byte in stream and parse packets.
 *
 * @param nextbyte next byte of incoming stream to parse
 *
 * This function is called for each byte in the data stream. It detects the
 * start of a packet, parses the incoming data into a packet structure. Once
 * a complete packet is received and validated, this function returns a pointer
 * to the packet. Until then, it returns NULL.
 *
 * Once the buffer pointer is returned, a new packet cannot be parsed until
 * the client releases the buffer by calling pkt_rx_free().
 *
 * @return A pointer to a valid packet or NULL.
 *
 * @note This function is expected to be called from (UART RX) interrupt
 * context. So it should non-blocking and execute in minimal time.
 */
extern void pkt_parser(uint8_t nextbyte);

#ifdef __cplusplus
}
#endif

#endif

/** @} */
