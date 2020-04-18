Optiboot Bootloader for BMS Boards
==================================

We are using the optiboot bootloader from the ATTinyCore project.
This is a custom build to accomodate the custom hardware. The main
differences are baud rate of 4800 and the location of the LED.

TLDR
----

The bootloader for the BMS board is committed here, and can be loaded into
flash using the Makefile in the parent directory. So there is nothing that
needs to be done here unless you need to rebuild the boot loader for some
reason. The rest of the text below explains where the boot loader came from.

How to reproduce it
-------------------

The original repo is here: https://github.com/SpenceKonde/ATTinyCore

I forked it, here: https://github.com/kroesche/ATTinyCore

Then I made branch for my modification, named `feature/bms_optiboot`:

https://github.com/kroesche/ATTinyCore/tree/feature/bms_optiboot

The branch is based from commit in the original repo:

`4470b8a82a8179fb2e769dbc6f3937ed4d81cf71`

Here is the diff of my changes to create the BMS optiboot:

```
diff --git a/avr/bootloaders/optiboot/Makefile.custom b/avr/bootloaders/optiboot/Makefile.custom
index 871d843..6d586eb 100644
--- a/avr/bootloaders/optiboot/Makefile.custom
+++ b/avr/bootloaders/optiboot/Makefile.custom
@@ -16,3 +16,17 @@ ifndef PRODUCTION
 	mv $(PROGRAM)_$(CHIP).lst $(PROGRAM)_$(TARGET).lst
 endif

+# custom target for the BMS board boot loader
+# we need baud rate to be 4800
+# delay 4s before starting app to allow time to initiate a boot loader
+# (since there is no hardware reset line)
+# always start boot loader after POR to avoid need for hw reset
+# the LED is A6 which is different from arduino default
+
+bms_8_4800:
+	$(MAKE) attiny841	 AVR_FREQ=8000000L BAUD_RATE=4800 LED=A6 TIMEOUT=4 NO_START_APP_ON_POR=1 OPTIBOOT_CUSTOMVER=60
+	mv $(PROGRAM)_attiny841.hex $(PROGRAM)_bms_8_4800.hex
+ifndef PRODUCTION
+	mv $(PROGRAM)_attiny841.lst $(PROGRAM)_bms_8_4800.lst
+endif
+
```

Building
--------

Assuming you have checked out the `feature/bms_optiboot` branch from my fork,
or made the necessary modifications in your own local copy, build it like
this:

(from path `ATTinyCore/avr/bootloaders/optiboot`)

    make bms_8_4800

This will produce the hex file `optiboot_bms_8_4800.hex`.

This file is committed here so unless the boot loader changes, there is
nothing else to do here.
