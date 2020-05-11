BMSNode Command Specification
=============================

This document specifies the BMSNode commands and responses. For details about
the packet format, see the [Packet Specification](packet.md).

All commands are sent with the packet "reply" bit clear, and all responses have
the reply bit set.

PING (1)
--------

### Command

|Byte   |Usage |
|-------|------|
|CMD    | 1    |
|LEN    | 0    |
|PLD    | None |

### Response

With reply bit:

|Byte   |Usage |
|-------|------|
|CMD    | 1    |
|LEN    | 0    |
|PLD    | None |

### Description

The PING command is used for aliveness and device discovery. It simply replies
with the same command code to indicate presence.

DFU (2)
-------

### Command

|Byte   |Usage |
|-------|------|
|CMD    | 2    |
|LEN    | 0    |
|PLD    | None |

### Response

The DFU command does not have a reply.

### Description

Upon receiving this command the node enters DFU mode (boot loader running).
At this point it is awaiting boot loader commands. If no boot loader commands
are received within a timeout period (4 seconds at the time of this writing),
the the firmware restarts.

The boot loader is arduino compatible [Optiboot](../build/optiboot/README.md)
and uses [avrdude](https://www.nongnu.org/avrdude/) as the programming
utility.

UID (3)
-------

### Command

|Byte   |Usage |
|-------|------|
|CMD    | 3    |
|LEN    | 0    |
|PLD    | None |

### Response

With reply bit:

|Byte    |Usage                      |
|--------|---------------------------|
|CMD     | 3                         |
|LEN     | 8                         |
|PLD[0:3]| 32-bit UID, little-endian |
|PLD[4]  | board type                |
|PLD[5]  | firmware major version    |
|PLD[6]  | firmware minor version    |
|PLD[7]  | firmware patch version    |

### Description

The UID command is used to discover the unique ID (UID) for a board. It also
returns the board type and the firmware version.

**Board Types**

|Type|Description                                           |
|----|------------------------------------------------------|
| 1  |OSHPark v4 first prototype boards                     |
| 2  |Stuart v4.2 small batch proto (w/incorrect thermistor)|
| 3  |Stuart v4.2 small batch proto w/swapped thermistor    |

**NOTE:** A BMSNode will accept and respond to this command at address 0, if
the BMSNode does not have already assigned a bus address. This command can be
used to discover the UID of a fresh device that has never had a bus address
assigned. If there is more than one device on the bus that meets this criteria,
then the likely results will be garbled packets.

The expected usage is to attach a fresh device to a bus, use this command to
find the UID, then use the UID to set a new bus address using the ADDR command.

ADDR (4)
--------

### Command

|Byte    |Usage                      |
|--------|---------------------------|
|CMD     | 4                         |
|LEN     | 4                         |
|PLD[0:3]| 32-bit UID, little-endian |

### Response

With reply bit:

|Byte    |Usage                      |
|--------|---------------------------|
|CMD     | 4                         |
|LEN     | 4                         |
|PLD[0:3]| 32-bit UID, little-endian |

### Description

The ADDR command is used to set the bus address for a BMSNode. This command is
unlike other commands in that it uses the UID to match the node, and the
packet address field to specify the new bus address.

When a node receives a ADDR packet, it checks its own UID to match the packet.
If the UID does not match, *the node does not respond, even if it already has
a matching bus address*. This means that it is possible to assign the same bus
address to different nodes on the same bus. This behavior makes it possible for
a controller to assign bus addresses to all devices on a bus without regard
to any already-assigned bus addresses. To do this, the controller must know
the UID of all devices on the bus.

ADCRAW (5)
----------

### Version Notes

Introduced in `0.5`.

### Command

|Byte   |Usage |
|-------|------|
|CMD    | 5    |
|LEN    | 0    |
|PLD    | None |

### Response

With reply bit:

|Byte    |Usage                         |
|--------|------------------------------|
|CMD     | 5                            |
|LEN     | 6                            |
|PLD[1:0]| Cell voltage sample data     |
|PLD[3:2]| Onboard thermistor sample    |
|PLD[5:4]| External sensor sample       |

### Description

The ADCRAW command is used to retrieve the raw sample data from the ADC module.
The data is 10-bit unsigned, stored in two 8-bit values. The data is little-
endian. For example PLD[0] is the lower byte and PLD[1] is the upper byte of
the cell voltage sample.

### Notes

The length of this response packet could increase in the future if more analog
samples are collected. However, it should remain backwards-compatible.
