BMS Node Packet Specification
=============================

_This is very preliminary and WIP._

Features
--------

* multi-drop, daisy-chained serial bus
* destination addressing
* broadcast capability
* controller initiated transactions
* 8-bit CRC

Hardware Concerns
-----------------

The current hardware uses a separate serial RX and TX signals. The nodes are
intended to be chained together. Therefore, each node must repeat or propagate
all data down the daisy chained serial bus.

The current data rate is constrained to 4800 due to slow response time of the
optical isolators.

The future hardware will combine the RX and TX signals to form a true
multi-drop serial bus. This means that each node will no longer have to copy
bytes. Instead each node just listens on the bus for any packets that require
action.

The future hardware will also attempt to achieve higher data rates.

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

This is the node address. Node addresses are assigned during provisioning by
TBD method. At the moment, addresses 1-254 are valid node addresses while
0 and 255 are reserved.

For packets from the controller to a node, the controller sets the address
field to the destination node. Each node knows its own address and only
responds to packets with a matching address.

For response packets back to the controller, the address field is the address
of the responding node.

### Command

| ID | Command | Description                 |
|----|---------|-----------------------------|
|  0 | reserved| not used                    |
|  1 | ping    | bus detection and aliveness |
|  2 | bootload| enter boot loader mode      |

### Length

The number of payload bytes, which can be 0 and up to 12 (TBD). If the length
is 0, then there is no payload and it is a command-only packet.

### CRC

An 8-bit CRC over the header and data bytes. It does not include preamble or
sync bytes. An 8-bit format is chosen to keep packet size as small as possible.

The AVR toolchain runtime library provides an 8-bit CRC function called
"crc8_ccitt" and that is used for BMS Node packet check field.

https://www.nongnu.org/avr-libc/user-manual/group__util__crc.html#gab27eaaef6d7fd096bd7d57bf3f9ba083

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
of all nodes on the bus.o
