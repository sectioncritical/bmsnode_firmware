BMS-2.0 (Next Gen) ==================

[This is just capturing some initial ideas and is very much WIP]

The BMS cell modules currently use existing firmware from diyBMSv4Code repo.
This is Arduino based code and for various reasons we would like to replace it.
These notes cover ideas for the replacement firmware.

General features ----------------

### Multi-drop

The present firmware uses a packet store and forward technique. Each node must
fully process the packet, take actions, and forward to the next node in a
physical daisy chain. A node can also modify the packet before forwarding it.

The new design will use a multidrop technique with node addressing. All data is
propagated to all nodes without modification, whether it appears to be valid
data or not. At the present time, the hardware requires that each node actively
receive and retransmit each byte. However, we are considering a hardware
modification that would make it a true multidrop bus and the "forwarding" is
just done as part of the hardware design. Then, even if a node firmware were to
stop functioning, the data would still pass to all the other nodes.

All transactions become point-to-point, command-response format. A node will
only reply when commanded.

There can also be a time-slot reporting method. This would allow the controller
to request that all nodes report, and reports issued according to a time-slot
schedule to prevent collisions.

Concerns: the present bus speed is quite slow. 4800 seems to be the reliable
rate. If the hardware is redesigned, then efforts should also be spent on
investigating ways to increase the data rate.

### Node identification

The present design has two-part address scheme, bank and device. The bank is
programmed into non-volatile memory but the device ID is ephemeral and can
change depending on how the device is initialized. In a completely functioning
and static system, the nodes should always have the same ID, but there is no
way to permanently identify nodes. For example if you removed some from the
system and re-installed later.

The new design proposes to have a permanent (tentative 16-bit) unique ID or
serial number per board. This would be provisioned when the board is assembled
and firmware installed the first time. This way, specific nodes can be
identified at any time regardless of where they are in the system.

For node addresses it is better to use no more than 8-bits, therefore there
should be a scheme by which either the 8-bit address is derived from the unique
ID, or is assigned by the controller, like a arp scheme (ID-to-address
mapping).

### Boot loader

It is desirable to be able to update a node in system. The current firmware is
too big to allow a boot loader. Also, because of the daisy-chain/store-forward
design, it is not possible to propagate boot loader protocol along the chain.
By using the method of propagating all data mentioned above in "multi-drop",
boot loader protocol (or any data) will always be carried to all nodes. This
would allow the controller to command a node in the middle, for example, to
enter boot loader mode while all the other nodes remain operating normally.
Then the boot loader protocol can be used to update the individual node.

Packet Format
-------------

Preliminary ideas about packet format:

|Byte| Field  | Description |
|----|--------|-------------|
| -1 |Preamble| One or more bytes used to wake up devices and establish sync|
|  0 | Sync   | Sync byte to indicate start of packet |
|  1 | Flags  | TBD flags to indicate features of packet: broadcast, reply, etc|
|  2 | Address| Node address for packet (command or response)(could use special address for broadcast)|
|  3 | Length | packet length (possibly combine flags and length)|
|  4 | Cmd/Rsp| command or response ID |
|  5+| Payload| packet payload contents|
|  N | Check  | check field (CRC or checksum)|

Notes:

Consider using COBS encoding as original firmware does. This is a good idea
because it makes  it easy to mark packet boundaries. However, it uses more code
space and compute time to perform the encode/decode.

The above is just an initial idea for packet layout. It should be refined and
optimized to reduce the number of bytes as much as possible.

We can also consider whether fixed packet length make sense, eliminating the
need for a length field. However, keeping adjustable length is more flexible
for future expansion.

Flow:

RX ISR receives all bytes and immediately copies to TX. (The copy will no be
necessary with multi-drop hardware modification).

Watch for preamble byte followed by sync byte to mark start of packet.

Check for address match. If no match then resume monitoring for preamble/sync
and store nothing.

If address match, then start copying incoming data into packet buffer. Once
entire packet is received and appears valid, pass to processing loop.

All of the above can be performed within RX ISR with little overhead.
