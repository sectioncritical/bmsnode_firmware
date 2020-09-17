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
index 871d843..7fcc7d2 100644
--- a/avr/bootloaders/optiboot/Makefile.custom
+++ b/avr/bootloaders/optiboot/Makefile.custom
@@ -16,3 +16,26 @@ ifndef PRODUCTION
 	mv $(PROGRAM)_$(CHIP).lst $(PROGRAM)_$(TARGET).lst
 endif

+# custom target for the BMS board boot loader
+# we need baud rate to be 4800
+# delay 4s before starting app to allow time to initiate a boot loader
+# (since there is no hardware reset line)
+# always start boot loader after POR to avoid need for hw reset
+# the LED is A6 which is different from arduino default
+
+.PRECIOUS: %.elf
+
+bms_8_4800:
+	$(MAKE) attiny841	 AVR_FREQ=8000000L BAUD_RATE=4800 LED=A6 TIMEOUT=4 NO_START_APP_ON_POR=1 CUSTOM_VERSION=60
+	mv $(PROGRAM)_attiny841.hex $(PROGRAM)_bms_8_4800.hex
+ifndef PRODUCTION
+	mv $(PROGRAM)_attiny841.lst $(PROGRAM)_bms_8_4800.lst
+endif
+
+bms_8_4800_hd:
+	$(MAKE) attiny841	 AVR_FREQ=8000000L BAUD_RATE=4800 LED=A6 TIMEOUT=4 NO_START_APP_ON_POR=1 CUSTOM_VERSION=61 DEFS=-DHALF_DUPLEX
+	mv $(PROGRAM)_attiny841.hex $(PROGRAM)_bms_8_4800_hd.hex
+ifndef PRODUCTION
+	mv $(PROGRAM)_attiny841.lst $(PROGRAM)_bms_8_4800_hd.lst
+endif
+
diff --git a/avr/bootloaders/optiboot/optiboot.c b/avr/bootloaders/optiboot/optiboot.c
index 76935b4..67a732c 100644
--- a/avr/bootloaders/optiboot/optiboot.c
+++ b/avr/bootloaders/optiboot/optiboot.c
@@ -160,6 +160,12 @@
 /* the start of upload correctly to communicate with the  */
 /* bootloader. Shorter timeouts are not viable.           */
 /*                                                        */
+/* HALF_DUPLEX                                            */
+/* Like RS485 except switching is done enabling/disabling */
+/* TX and RX pins when needed and does not use external   */
+/* switch or transceiver for switching. Not implemented   */
+/* for SOFT_UART.                                         */
+/*                                                        */
 /**********************************************************/

 /**********************************************************/
@@ -196,6 +202,8 @@
 /**********************************************************/
 /* Edit History:                                          */
 /*                                                        */
+/* July 2020 Joe Kroesche github.com/kroesche/ATTinyCore  */
+/*      added HALF_DUPLEX option                          */
 /* Mar 2020 Spence Konde for ATTinyCore                   */
 /*           github.com/SpenceKonde                       */
 /* 58.0 Pull in RS485 support by Vladimir Dronnikov       */
@@ -749,7 +757,11 @@ int main(void) {
   #ifndef SINGLESPEED
   UCSRA = _BV(U2X); //Double speed mode USART
   #endif //singlespeed
+  #ifdef HALF_DUPLEX
+  UCSRB = _BV(RXEN);  // enable Rx
+  #else
   UCSRB = _BV(RXEN) | _BV(TXEN);  // enable Rx & Tx
+  #endif
   UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);  // config USART; 8N1
   UBRRL = (uint8_t)BAUD_SETTING;
   #else // mega8/etc
@@ -765,7 +777,11 @@ int main(void) {
       #ifndef SINGLESPEED
   UART_SRA = _BV(U2X0); //Double speed mode USART0
       #endif
+  #ifdef HALF_DUPLEX
+  UART_SRB = _BV(RXEN0);
+  #else
   UART_SRB = _BV(RXEN0) | _BV(TXEN0);
+  #endif
   UART_SRC = _BV(UCSZ00) | _BV(UCSZ01);
   UART_SRL = (uint8_t)BAUD_SETTING;
     #endif // LIN_UART
@@ -1051,8 +1067,29 @@ void putch(char ch) {
       RS485_PORT &= ~_BV(RS485);
       #endif
     #else //not RS485
+      #ifdef HALF_DUPLEX
+      // HALF_DUPLEX is like RS485 except with internal TX/RX enable
+      // instead of internal signal
+      //
+      // initial check for UDRE is not necessary here because HALF_DUPLEX
+      // requires entire byte transmitted before exiting. Therefore upon
+      // entry here we are guaranteed that TX can take a byte. This saves
+      // some boot loader code space
+      //while (!(UART_SRA & _BV(UDRE0)) { /* wait for tx ready */ }
+      // disable RX, enable TX
+      UART_SRB = _BV(TXEN0);
+      // clear transmit complete flag
+      UART_SRA |= _BV(TXC0);
+      // write character to output
+      UART_UDR = ch;
+      // wait for transmit complete
+      while (!(UART_SRA & _BV(TXC0))) { /* wait */ }
+      // disable TX, re-enable RX
+      UART_SRB = _BV(RXEN0);
+      #else // not HALF_DUPLEX
       while (!(UART_SRA & _BV(UDRE0))) {  /* Spin */ }
         UART_UDR = ch;
+      #endif
     #endif
   #else //is LIN UART
     while (!(LINSIR & _BV(LTXOK)))   {  /* Spin */ }
```

Building
--------

Assuming you have checked out the `feature/bms_optiboot` branch from my fork,
or made the necessary modifications in your own local copy, build it like
this:

(from path `ATTinyCore/avr/bootloaders/optiboot`)

    make bms_8_4800_hd

This will produce the hex file `optiboot_bms_8_4800_hd.hex`.

This file is committed here so unless the boot loader changes, there is
nothing else to do here.

**NOTE:** the prior version of the BMS boot loader was called bms_8_4800.
(without the *hd*). The current (new) version supports the half duplex serial
used by the latest BMSNode boards. The prior version (without hd), is for the
original "Stuart" design.

**NOTE:** UPDATE - there is now a 2400 baud option. Use target `bms_8_2400_hd`
to generate the 2400 boot loader. The reason for 2400 is for boards that have
a hardware serial rate problem.
