BMSNode Blink Codes {#blink}
===================

TODO: this documents should be brought into a higher level app-level spec.

This is a placeholder document to keep track of the meanings of blink
patterns as the firmware is developed.

Patterns
--------

### Green

The green LED is used by the boot loader. It will be rapid blinking or solid
during boot loader operations. The boot loader always starts first when a node
is powered or reset, so you should see the green LED blinking for 4 seconds
when a node is first started.

The blue LED may be blinking or faintly on during boot loader operations and
should be ignored.

### Blue

The blue LED is controlled by the application and is used to communicated the
state of the main apllication. The green LED is not used by the application
(at the time of this writing - things are in flux).

|Name   |Blink Pattern      |Meaning                            |
|-------|-------------------|-----------------------------------|
|START  |50 ms for 5 sec    |Main app has just started due to reset, or power on. Follows boot loader. No response to commands during this time.|
|ACTIVE |200 ms for >= 1 sec|Handling serial bus activity or processing commands.|
|DFUWAKE|1 s for 8 seconds  |DFU wake. Node stay awake for 8 seconds to relay DFU messages.|
|SLEEP  | OFF               |Sleeping, in power saving mode.    |

State Transitions
-----------------

|PS      |NS      |Transition                                               |
|--------|--------|---------------------------------------------------------|
|off     |START   |Node is powered on or reset, or started from boot loader |
|START   |ACTIVE  |5 seconds delay                                          |
|ACTIVE  |SLEEP   |no bus activity or commands in process for 1 seconds     |
|ACTIVE  |DFUWAKE |any DFU command on the bus                               |
|SLEEP   |ACTIVE  |bus activity                                             |
|DFUWAKE |ACTIVE  |8 seconds delay                                          |

### Powering On/START

Upon being first powered, or hardware reset, the boot loader runs for 4
seconds. During this time the green LED will blink or be on.

After the boot loader is finished, the app will start. It will blink the blue
LED rapidly for 5 seconds while it is initializing, and cannot process any
commands during this time.

If a DFU update is performed (loading new firmware via boot loader), at the
end of the update process the boot loader will start the (new) firmware and
the 5 second blue LED blink pattern will occur.

The above implies that it will take 9 seconds from power on or reset before a
node will respond to commands.

### START

START always lasts 5 seconds with rapid blinking blue LED and then it will
unconditionally transition to ACTIVE state.

### ACTIVE

The firmware remains in active state for as long as there is any activity
detected on the serial bus, or if there is any command currently being
processed.

The blue LED blinks at 200 ms in this state.

After 1 seconds of no activity, the firmware will transition to the SLEEP
state.

If during active state a DFU command is observed on the serial bus (addressed
to any node), the firmware will transition to the DFUWAKE state.

### DFUWAKE

This state is essentially the same as ACTIVE state, except that it will remain
in this state for 8 seconds. The purpose of this state is to ensure that all
nodes remain active long enough for the DFU process to start up. This is
because the DFU process can take several seconds to initiate and if a node
goes to sleep during this time (after 1 second) then the DFU protocol will
not be properaly relayed by intermediate nodes.

Once DFU protocol is running, then there is enough bus activity to keep the
node in the active state.

After 8 seconds the firmware will transition back to the ACTIVE state and
remain there as long as there is any activity, including DFU protocol or
routine commands.

The blue LED blinks with 1 second toggle in DFUWAKE state.
