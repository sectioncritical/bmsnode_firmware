BMSNode Blink Codes
===================

TODO: this documents should be brought into a higher level app-level spec.

This is a placeholder document to keep track of the meanings of blink
patterns as the firmware is developed.

Patterns
--------

### Green

The green LED is used by the boot loader. It will be rapid blinking or solid
during boot loader operations. The boot loader always starts first when a node
is powered or reset, so you should see the green LED blink rapidly for a brief
moment when the boot loader starts. It may remain dark for the remainder of
the boot loader delay, or it may be blinking. The device runs the boot loader
for 4 seconds.

The green LED is also used to identify a node by turning on for 1 second in
response to a PING command.

The blue LED may be blinking or faintly on during boot loader operations and
should be ignored.

### Blue

The blue LED is controlled by the application and is used to communicated the
state of the main apllication. The green LED is not used by the application
except for the PING command.

|Name   |Blink Pattern      |Meaning                            |
|-------|-------------------|-----------------------------------|
|START  |50 ms for 1 sec    |Main app has just started due to reset, or power on. Follows boot loader. No response to commands during this time.|
|ACTIVE |200 ms for >= 1 sec|Handling serial bus activity or processing commands.|
|SHUNT  |800ms on 200ms off |Operating in shunt mode            |
|TESTMODE|200ms on 800ms off|Operating in test mode             |
|DFUWAKE|1 s for 8 seconds  |DFU wake. Node stay awake for 8 seconds to relay DFU messages.|
|SLEEP  | OFF               |Sleeping, in power saving mode.    |

### Red

The red LED is controlled by the resistive load circuit. It turns on whenever
the resistive shunt load is turned on, and likewise off. It cannot be
controlled independently. Therefore, the red LED provides a visual indicator
of the state of shunting.

If the shunt load is using PWM, then the red LED may appear dim.

State Transitions
-----------------

TODO: integrate this information into a comprehensive design document.

The following section is an abstraction to explain the sleep/wake operation
of BMS Node. It does not reflect the actual internal state machine
implementation.

|PS      |NS      |Transition                                               |
|--------|--------|---------------------------------------------------------|
|off     |START   |Node is powered on or reset, or started from boot loader |
|START   |ACTIVE  |1 seconds delay                                          |
|ACTIVE  |SLEEP   |no bus activity or commands in process for 1 seconds     |
|ACTIVE  |DFUWAKE |any DFU command on the bus                               |
|SLEEP   |ACTIVE  |bus activity                                             |
|DFUWAKE |ACTIVE  |8 seconds delay                                          |

### Powering On/START

Upon being first powered, or hardware reset, the boot loader runs for 4
seconds. At the start of this time the green LED will blink rapidly for a brief
amount of time. After that it may remain on or off. (current bootloader leaves
it off, old bootloader leaves it on).

After the boot loader is finished, the app will start. It will blink the blue
LED rapidly for 1 second while it is initializing, and cannot process any
commands during this time.

If a DFU update is performed (loading new firmware via boot loader), at the
end of the update process the boot loader will start the (new) firmware and
the 1 second blue LED blink pattern will occur.

The above implies that it will take 5 seconds from power on or reset before a
node will respond to commands.

### START

START always lasts 1 second with rapid blinking blue LED and then it will
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
in this state for 8 seconds. This is a legacy state that was needed for the
original hardware design.

The original purpose of this state was to ensure that all nodes remain active
long enough for the DFU process to start up. This was because the DFU process
can take several seconds to initiate and if a node goes to sleep during this
time (after 1 second) then the DFU protocol will not be properly relayed by
intermediate nodes.

Once DFU protocol is running, then there is enough bus activity to keep the
node in the active state.

After 8 seconds the firmware will transition back to the ACTIVE state and
remain there as long as there is any activity, including DFU protocol or
routine commands.

The current hardware design does not require nodes to actively relay DFU
protocol bytes and so this state is no longer needed. It remains for the moment
on the chance that old hardware needs to be supported. It will eventually be
removed.

The blue LED blinks with 1 second toggle in DFUWAKE state.

### SLEEP

On entering this state, all IO is disabled and peripherals powered down. The
MCU is placed in as low a power mode as possible. The low power mode is ended
when any activity occurs on the serial bus, or the device has a hardware reset.
When the MCU wakes due to serial activity, the firmware then re-enables the IO
and powers up the peripherals and resumes running in the ACTIVE state.
