
PROGNAME=test

# settings for avrdude. These default settings assume you have platformio
# version of avrdude in the default location. These can be overidden in the
# environment or on make command line
AVRDUDE?=$(HOME)/.platformio/packages/tool-avrdude/bin/avrdude
AVRDUDE_CONF?=$(HOME)/.platformio/packages/tool-avrdude/avrdude.conf
AVRDUDE_PORT?=/dev/cu.usbmodem14101

OUT=obj
SRC=../src

OBJS=$(OUT)/main.o

HEXFILE:=$(OUT)/$(PROGNAME).hex
ELFFILE:=$(OUT)/$(PROGNAME).elf

CC=avr-gcc
OBJCOPY=avr-objcopy
SIZE=avr-size

CFLAGS=-std=c99 -Os -Werror -Wall -ffunction-sections -fdata-sections -flto -mmcu=attiny841
LDFLAGS=-Wl,-Map,$(OUT)/test.map -Wl,--gc-sections -fuse-linker-plugin

all: $(HEXFILE)


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

