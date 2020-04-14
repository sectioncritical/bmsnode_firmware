# SPDX-License-Identifier: MIT
#
# Copyright 2020 Joseph Kroesche
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

PROGNAME=bmsnode

# settings for avrdude. These default settings assume you have platformio
# version of avrdude in the default location. These can be overidden in the
# environment or on make command line
AVRDUDE?=$(HOME)/.platformio/packages/tool-avrdude/bin/avrdude
AVRDUDE_CONF?=$(HOME)/.platformio/packages/tool-avrdude/avrdude.conf
AVRDUDE_PORT?=/dev/cu.usbmodem14101

OUT=obj
SRC=../src

OBJS=$(OUT)/main.o $(OUT)/pkt.o $(OUT)/serial.o

HEXFILE:=$(OUT)/$(PROGNAME).hex
ELFFILE:=$(OUT)/$(PROGNAME).elf

all: $(HEXFILE)

.PHONY: help
help:
	@echo ""
	@echo "BMS Node Makefile Help"
	@echo "======================"
	@echo ""
	@echo "all/(default)    - build the firmware hex file"
	@echo "clean            - delete all build products"
	@echo "program          - program target using avrdude"
	@echo ""
	@echo "test             - build and run the unit test"
	@echo "cleantest        - clean the unit test"
	@echo ""

CC=avr-gcc
OBJCOPY=avr-objcopy
SIZE=avr-size

CFLAGS=-std=c99 -Os -Werror -Wall -ffunction-sections -fdata-sections -fshort-enums -flto -mmcu=attiny841
LDFLAGS=-Wl,-Map,$(OUT)/$(PROGNAME).map -Wl,--gc-sections -fuse-linker-plugin

$(OUT):
	mkdir -p $(OUT)

$(OUT)/%.o: $(SRC)/%.c $(OUT)
	$(CC) $(CFLAGS) -o $@  -c $<

$(ELFFILE): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBFLAGS)

$(HEXFILE): $(ELFFILE)
	$(OBJCOPY) -O ihex -R .eeprom $< $@
	$(SIZE) $<

.PHONY: clean
clean:
	rm -rf $(OUT)

# flash the firmware onto the target
.PHONY: program
program: $(HEXFILE)
	$(AVRDUDE) -P$(AVRDUDE_PORT) -b19200 -v -p attiny841 -C $(AVRDUDE_CONF) -c stk500v1 -U flash:w:$<:i

.PHONY: test
test:
	make -C ../test report

.PHONY: cleantest
cleantest:
	make -C ../test clean
