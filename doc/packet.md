BMS Node Packet Specification
=============================

Features
--------

* multi-drop, daisy-chained serial bus
* destination addressing
* broadcast capability
* controller initiated transactions
* 8-bit CRC

Hardware Considerations
-----------------------

The serial hardware design uses a daisy-chained multi-drop bus. Each node is
optically isolated and automatically propagates the serial data to the next
node. The MCU serial RX and TX lines are shared which means that the MCU is
listening most of the time, and only transmits in response to receiving a
packet that it addressed to that node.

The serial driver in the MCU must keep the TX signal as an input (or high-Z)
except when it is transmitting. It must also make sure that the entire last
byte has been transmitted before turning off the TX signal.

There is no hardware mechanism to prevent bus collisions. The design relies on
the command/response format and that no node transmits unless commanded and
only the controller (commanding host) issues commands. The controller software
must ensure enough time between commands for responses to finish.

The serial hardware design does incur a rise-time/fall-time error that
accumulates with each node which limits the total number of nodes that can be
chained together. That number is not characterized here.

The present firmware is using 9600 for the data rate.

Future Changes
--------------

The packet format presented below has been in use for a while. The flags field
was included in the format in anticipation of the need for additional
signaling. However during the firmware development there has not appeared a
need for any additional flags.

As a future optimization, the reply flag could be combined with the address or
command fields, saving a byte.

Another possible change, would be to change the preamble byte to another value
that would make it easier to perform autobaud. The current preamble byte is
a square wave which makes it possible to measure a single bit time using a
timer. But that requires adding bit measuring timer code to the firmware. There
are other autobaud techniques that use carefully chosen values that appear as
different valid values depending on the baud rate.

Packet Format
-------------

|Byte| Field  | Description                                                 |
|----|--------|-------------------------------------------------------------|
| -2 |Preamble| One or more bytes used to wake up devices and establish sync|
| -1 | Sync   | Sync byte to indicate start of packet                       |
|  0 | Flags  | TBD flags to indicate features of packet: broadcast, reply, etc|
|  1 | Address| Node address for packet                                     |
|  2 | Command| command ID                                                  |
|  3 | Length | payload length in bytes (can be 0)                          |
|  4+| Payload| variable payload contents (can be none)                     |
|  N | CRC    | 8-bit CRC                                                   |

### Preamble

**Value: 0x55**

Preamble bytes are used to "wake up" the nodes on the bus in preparation to
receive a packet. There must be at least one preamble byte and there is no
upper limit. Each node simply ignores preamble bytes.

A string of preamble bytes can be used to reset nodes parsing state machine.
_With the current limit of 12 data bytes_, along with a single CRC byte, 13
preamble bytes should always cause the parser to return to the searching
state.

### Sync

**Value: 0xF0**

The sync byte marks the start of a packet. There must always be at least one
preamble byte followed by a single sync byte to mark the start of a packet.

### Flags

| Bit | Flag | Description                          |
|-----|------|--------------------------------------|
|  7  | reply| 0=command to node, 1=reply from node |
|  6  | init | 0=normal, 1=init mode                |
| 5:0 | res  | reserved                             |

The function of the init flag is TBD.

It is possible that the flags field could be combined with the length field
to eliminate one byte of header.

### Address

This is the node address. Node addresses are assigned using the `ADDR` command.
Only one un-addressed node can be on a serial bus at one time. So nodes should
be added to the bus one at a time and get immediate address assignment using
the `ADDR` command. Or, they can be assigned when the board is tested or
provisioned by the board test or provisioning utility.

At the moment, addresses 1-254 are valid node addresses while 0 and 255 are
reserved. Address 254 is being used for testing.

For packets from the controller to a node, the controller sets the address
field to the destination node. Each node knows its own address and only
responds to packets with a matching address.

For response packets back to the controller, the address field is the address
of the responding node.

### Command

| ID | Command | Description                       |
|----|---------|-----------------------------------|
|  0 | reserved| not used                          |
|  1 | PING    | bus detection and aliveness       |
|  2 | DFU     | enter device firmware update mode |
|  3 | UID     | discover board unique ID (see notes in command spec)|
|  4 | ADDR    | set board bus address             |
|  5 | ADCRAW  | read raw ADC sample data          |
|  6 | STATUS  | read BMS node status              |
|  7 | SHUNTON | turn on shunting                  |
|  8 | SHUNTOFF| turn off shunting                 |
|  9 | SETPARM | set configuration parameter       |
| 10 | GETPARM | get configuration parameter       |

See [Command Specification](command) for command details.

### Length

The number of payload bytes, which can be 0 and up to 12 (TBD). If the length
is 0, then there is no payload and it is a command-only packet.

### CRC

An 8-bit CRC over the header and data bytes. It does not include preamble or
sync bytes. An 8-bit format is chosen to keep packet size as small as possible.

The AVR toolchain runtime library provides an 8-bit CRC function called
"crc8_ccitt" and that is used for BMS Node packet check field.

https://www.nongnu.org/avr-libc/user-manual/group__util__crc.html#gab27eaaef6d7fd096bd7d57bf3f9ba083

A Python implementation is provided at the end of this document.

Parser State Machine
--------------------

TODO: add state diagram

### Searching

The parser is searching for preamble bytes. It remains in this state until a
preamble byte is received.

### Sync

The parser is waiting for a sync byte. It will remain in this state as long
as it continues to receive preamble bytes. If it receives a sync byte it then
begins receiving a header. If it receives any value that is not sync or
preamble then it returns to *Searching*.

### Header

In the header state the parser is reading in header bytes. The the extent
possible it validates header bytes as they are received. If any header byte
is found to not be valid, the parser returns to *Searching*. Once the correct
number of header bytes are received, the parser goes to *Payload* or *Check*
states, depending on whether there is any payload.

### Payload

In the paylod state, the parser is storing incoming bytes into the payload
area of the packet buffer. It will store the number of bytes as data according
to the header length field. There is no validation of payload bytes. The parser
will only leave this state after receiving the correct number of payload bytes.

The parser can be forced out of this state by sending enough preamble bytes to
complete the payload state.

### Check

In this state the parser inteprets the next byte received as the CRC byte.
It compares the value to a crc-8 value it accumulated as header and payload
bytes were received. If they match, then the packet is valid. If they do not
match then the packet is not valid and the incoming buffer is discarded and
the parser returns to *Searching*.

### False Detection

It is possible that the parser may be in the searching state while a packet
is in progress on the bus. The parser header or payload data may appear to
be the start of a valid packet. For example the payload may contain a preamble
byte followed by a sync byte.

If this happens, the parser may incorrectly detect the start of a packet and
start to receive header and possible payload data. This will be detected and
the parser return to the search state if any header byte is detected as not
valid. And in the worst case it appears to have received a valid header, then
it will attempt to receive a number of data bytes and then finally the CRC will
fail.

The probability of this occuring is considered small enough for our purposes to
not be a concern. In the worst case the parser falsely thinks it has a header
for the maximum number of payload bytes. In this case the parser can always be
reset by sending the number of preamble bytes to match the maximum payload
size plus 1 (for the CRC byte). In the current design this is 13 preamble
bytes.

In operation, this should be handled as follows: if any node stops responding,
the controller should send out at least 13 preamble bytes to reset the parser
of all nodes on the bus.

CRC Python Implementation
-------------------------

Following is a python implementation of the same CRC function used by the
packet processor. The `pktbytes` argument is bytes-like.

```
def crc8_ccitt(pktbytes):
    crc = 0

    for inbyte in pktbytes:

        databyte = crc ^ inbyte

        for idx in range(8):
            if (databyte & 0x80) != 0:
                databyte <<= 1
                databyte ^= 0x07
            else:
                databyte <<= 1

        crc = databyte & 0xFF
    return crc
```
