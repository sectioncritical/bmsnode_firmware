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

# TODO: future code size profiling, look at puncover or bloaty

# using bashisms
SHELL=/bin/bash

PROGNAME=bmsnode

# name of package for updating
PKG?=0.0.0

# default serial data rate
BAUD?=9600

# MCUPROG ports
# you will need to override these for your system. The easiest thing to do
# is to set an environment variable and then you wont need to type on command
# line each time you run this Makefile.
#
# MCUPROG_PORT is the serial port used for UPDI programmer pymcuprog.
# It is used for any Makefile targets that "program" something. If you are
# not using the programmer, then this does not matter.
#
MCUPROG_PORT?=/dev/cu.usbmodem14101

# LOAD_PORT is a device serial port used for using bmsloader with
# the serial boot loader. It is the serial port that is connected to your
# bmsnode boards. This is used with "upload" targets.
#
LOAD_PORT?=/dev/cu.usbserial-A107LYRO

# command to run pymcuprog with all the common args filled out
MCUPROG=venv/bin/pymcuprog -d attiny1614 -t uart -u $(MCUPROG_PORT)

# Atmel device pack location
PACK_ROOT?=/opt/avr/ATtiny_DFP

# application start address (to allow for 0-base bootloader)
APP_ADDRESS?=0x200

# bmsnode device address, for operations that need it
# you almost certainly will need to override this on command line
ADDR?=254

OUT=obj
SRC=../src

OBJS=$(OUT)/main.o
OBJS+=$(OUT)/pkt.o
OBJS+=$(OUT)/cmd.o
OBJS+=$(OUT)/ser.o
OBJS+=$(OUT)/cfg.o
OBJS+=$(OUT)/tmr.o
OBJS+=$(OUT)/adc.o
OBJS+=$(OUT)/ver.o
OBJS+=$(OUT)/thermistor_table.o
OBJS+=$(OUT)/shunt.o
OBJS+=$(OUT)/testmode.o
OBJS+=$(OUT)/led.o
OBJS+=$(OUT)/list.o
OBJS+=$(OUT)/kissm.o

# to run the versioning tool we need to switch around to different
# directories. So it is handy to be able to refer to directopries and files
# with full paths.
BUILD_DIR=$(shell pwd)
PROJ_DIR=$(shell cd ..; pwd)
BUILDLOG=buildlog.txt

# device remains unlocked
LOCKFUSE=0xff

# EESAVE (dont erase eeprom), BOD level is 1.8
HFUSE=0xd6

# disable CLKDIV8 (run at 8mhz). use 8MHz internal osc
LFUSE=0xe2

# BOD enabled in all modes, enable self programming
EFUSE=0xf4

HEXFILE:=$(OUT)/$(PROGNAME).hex
ELFFILE:=$(OUT)/$(PROGNAME).elf
UPLOADFILE?=$(HEXFILE)
PKGNAME:=$(PROGNAME)-$(PKG)

all: $(HEXFILE)

.PHONY: help
help:
	@echo ""
	@echo "BMS Node Makefile Help"
	@echo "======================"
	@echo ""
	@echo "Routine Use"
	@echo "-----------"
	@echo "all/(default)    - build the firmware hex file (BAUD)"
	@echo "clean            - delete all build products"
	@echo "program          - program dev build to target using programmer"
	@echo "upload           - upload dev build with serial boot loader (no dfu) (BAUD)"
	@echo "dfuload          - upload with dfu command first (ADDR, BAUD)"
	@echo "dfuping          - ping boot loader with dfu command first (ADDR, BAUD)"
	@echo "discover         - probe bus for connected devices (BAUD)"
	@echo "monitor          - run bus monitoring program (BAUD)"
	@echo "reset            - perform UPDI reset using programmer"
	@echo ""
	@echo "In-system firmware update (old firmware running)"
	@echo "-----------"
	@echo "dfupkg ADDR=1 PKG=1.0.0 - bootload released firmware onto node w/dfu command"
	@echo ""
	@echo "Production and Provisioning"
	@echo "---------------------------"
	@echo "fuses            - program the fuses using programmer"
	@echo "provision BOARD=oshparkv4 - provision the board info (one-time)"
	@echo "program_boot     - program the boot loader using programmer (BAUD)"
	@echo "erase            - erase the device using programmer (not usual)"
	@echo "boardinfo        - read board provisioning info"
	@echo "dumpfuses        - dump fuse contents to console, using programmer"
	@echo ""
	@echo "Release and Packaging"
	@echo "---------------------"
	@echo "package PRODUCTION=1 - create release package with release versioning"
	@echo "package              - create release package with dev versioning (0.0.0)"
	@echo "nextver              - generate next version (done by package)"
	@echo "changelog            - generate the CHANGELOG file (done by package)"
	@echo "release              - tag release (used by gitlab CI, not local)"
	@echo ""
	@echo "Testing"
	@echo "-------"
	@echo "test             - build and run the unit test"
	@echo "cleantest        - clean the unit test"
	@echo "(see also the Makefile in the test subdir)"
	@echo ""
	@echo "Python Virtual Environment"
	@echo "--------------------------"
	@echo "venv             - create the python virtual environment"
	@echo "cleanvenv        - clean the python virtual environment"
	@echo ""
	@echo "Defaults"
	@echo "--------"
	@echo "ADDR            = $(ADDR)"
	@echo "BAUD            = $(BAUD)"
	@echo "LOAD_PORT       = $(LOAD_PORT)"
	@echo "MCUPROG_PORT    = $(MCUPROG_PORT)"
	@echo ""
	@echo "NOTES"
	@echo "-----"
	@echo "* 'make clean' then 'make package' will build everything"
	@echo "* use 'PRODUCTION=1' to generate version number, otherwise"
	@echo "  the version will be 0.0.0"
	@echo "* 'make changelog' can be used to see what the generated CHANGELOG"
	@echo "  will look like. However this is done automatically for package."
	@echo "  Also, CHANGELOG may be updated every time 'changelog' or 'package'"
	@echo "  target is run, so be sure that CHANGELOG.md is clean before"
	@echo "  generating it for a real release."
	@echo ""

CC=avr-gcc
OBJCOPY=avr-objcopy
SIZE=avr-size

CFLAGS=-std=c99 -Os -Werror -Wall -ffunction-sections -fdata-sections -fshort-enums -flto -mmcu=attiny1614 -I $(PACK_ROOT)/include -B $(PACK_ROOT)/gcc/dev/attiny1614
LDFLAGS=-Wl,-Map,$(OUT)/$(PROGNAME).map -Wl,--gc-sections -Wl,--section-start=.text=$(APP_ADDRESS) -fuse-linker-plugin

$(OUT):
	mkdir -p $(OUT)

$(OUT)/%.o: $(SRC)/%.c $(BUILD_DIR)/next-version | $(OUT)
	$(eval VERSTR=$(shell cat $(BUILD_DIR)/next-version | tr '.' ','))
	$(CC) $(CFLAGS) -DFWVERSION="{$(VERSTR)}" -DBAUDRATE=$(BAUD) -o $@  -c $<

$(ELFFILE): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBFLAGS)
	echo "Commit used for building:" 2>&1 | tee -a $(BUILDLOG)
	git rev-parse HEAD 2>&1 | tee -a $(BUILDLOG)

$(HEXFILE): $(ELFFILE)
	$(OBJCOPY) -O ihex -R .eeprom $< $@
	$(SIZE) $< 2>&1 | tee -a $(BUILDLOG)

.PHONY: clean
clean:
	rm -rf $(OUT)
	rm -rf package
	rm -f next-version
	rm -f $(BUILDLOG)

# flash the firmware onto the target
.PHONY: program
program: $(HEXFILE) venv
	$(MCUPROG) write -f $<

# dump the flash contents into a file
.PHONY: dumpflash
dumpflash: venv
	$(MCUPROG) read -m flash -f flash_dump.hex

.PHONY: program_boot
program_boot: bootloader/optiboot_x/optiboot_bmsnodev3.hex venv
	echo "Using $(BAUD) boot loader"
	$(MCUPROG) write -f $<

.PHONY: upload
upload: $(UPLOADFILE) venv
	venv/bin/bmsloader -d $(LOAD_PORT) -b $(BAUD) -f $< upload
	venv/bin/bmsloader -d $(LOAD_PORT) -b $(BAUD) -f $< verify

# fuses - most of the fuses are fine in the default state.
# the only fuse to change is fuse 8 - BOOTEND. It is set to 2 to
# allocate 2 pages (0x200) to the boot loader. The app will start at 0x200
.PHONY: fuses
fuses: venv
	$(MCUPROG) write -m fuses -o 8 -l 2

# dumps fuses to screen
.PHONY: dumpfuses
dumpfuses: venv
	$(MCUPROG) read -m fuses

.PHONY: erase
erase: venv
	$(MCUPROG) erase

.PHONY: reset
reset: venv
	$(MCUPROG) reset

.PHONY: test
test:
	make -C ../test report

$(PKGNAME)/$(PROGNAME).hex: $(PKGNAME).tar.gz
	tar -xvf $<

.PHONY: dfuload
dfuload: $(UPLOADFILE) venv
	venv/bin/bmsutil -d $(LOAD_PORT) -b $(BAUD) dfu -a $(ADDR)
	venv/bin/bmsloader -d $(LOAD_PORT) -b $(BAUD) -f $< upload
	venv/bin/bmsloader -d $(LOAD_PORT) -b $(BAUD) -f $< verify

.PHONY: dfupkg
dfupkg: $(PKGNAME)/$(PROGNAME).hex venv
	venv/bin/bmsutil -d $(LOAD_PORT) -b $(BAUD) dfu -a $(ADDR)
	venv/bin/bmsloader -d $(LOAD_PORT) -b $(BAUD) -f $< upload
	venv/bin/bmsloader -d $(LOAD_PORT) -b $(BAUD) -f $< verify

.PHONY: dfuping
dfuping: $(UPLOADFILE) venv
	venv/bin/bmsutil -d $(LOAD_PORT) -b $(BAUD) dfu -a $(ADDR)
	venv/bin/bmsloader -d $(LOAD_PORT) -b $(BAUD) ping

.PHONY: discover
discover: venv
	venv/bin/bmsutil -d $(LOAD_PORT) -b $(BAUD) discover

.PHONY: monitor
monitor: venv
	venv/bin/bmsutil -d $(LOAD_PORT) -b $(BAUD) monitor

.PHONY: cleantest
cleantest:
	make -C ../test realclean

# system should have python3 available as either python3 or python
PY3?=$(shell command -v python3 || command -v python)

# set up python virtual environment needed for platformio
venv: venv/bin/activate

venv/bin/activate: requirements.txt
	test -d venv || $(PY3) -m venv venv
	. venv/bin/activate; pip install -U pip setuptools wheel; pip install -Ur $<
	venv/bin/safety check
	touch venv/bin/activate

.PHONY: cleanvenv
cleanvenv:
	rm -rf venv

.PHONY: safety
safety: venv
	venv/bin/safety check

######################
#### PROVISIONING ####

# generate provisioning file and write to eeprom of device
.PHONY: provision
provision: venv
	mkdir -p provisioning
	venv/bin/python provtool.py --write provisioning/provision.hex --board $(BOARD)
	$(MCUPROG) write -f provisioning/provision.hex

# read provisioning info from board
.PHONY: boardinfo
boardinfo: venv
	mkdir -p provisioning
	$(MCUPROG) read -m user_row -f provisioning/provinfo.hex
	venv/bin/python provtool.py --read provisioning/provinfo.hex

###################
#### RELEASING ####

# tool is originally called "release" but that is a little common so rename
# it to "vertool"
VERTOOL:=$(BUILD_DIR)/vertool

# versioning tool expects certain GitLab CI environment variables to be
# defined. They will already be in the environment if this is run from CI.
# But to run locally they must be defined. These will be overridden if
# run from CI
CI_PROJECT_DIR?=$(PROJ_DIR)
CI_COMMIT_REF_NAME?=$(shell git rev-parse --abbrev-ref HEAD)
CI_COMMIT_REF_SLUG?=$(subst /,-,$(CI_COMMIT_REF_NAME))
CI_PIPELINE_IID?=0
export CI_COMMIT_REF_NAME
export CI_COMMIT_REF_SLUG
export CI_PIPELINE_IID

# get the release versioning tool
# we are downloading for now because it is a little large to put in repo
# plus, we need different binary depending on OS
ifeq ($(shell uname),Darwin)
    VERTOOL_URL:=https://juhani.gitlab.io/go-semrel-gitlab/download/v0.21.1/darwin/release
endif
ifeq ($(shell uname),Linux)
    VERTOOL_URL:=https://juhani.gitlab.io/go-semrel-gitlab/download/v0.21.1/release
endif

$(VERTOOL):
	curl --output $@ $(VERTOOL_URL)
	chmod +x $@

vertool: $(VERTOOL)

.PHONY: nextver
nextver: $(BUILD_DIR)/next-version

$(BUILD_DIR)/next-version: $(VERTOOL)
	if [ -z "$$PRODUCTION" ] ; then echo "0.0.0" > $@; else \
	cd $(PROJ_DIR); $(VERTOOL) next-version --allow-current > $@; fi
	cat $@

.PHONY: changelog
changelog: $(VERTOOL)
	git checkout -- ../CHANGELOG.md
	-cd $(PROJ_DIR); $(VERTOOL) changelog -f CHANGELOG.md
	-cat $(PROJ_DIR)/CHANGELOG.md

.PHONY: package
package: $(HEXFILE) $(BUILD_DIR)/next-version changelog
	rm -rf package
	mkdir -p package
	cp $(BUILDLOG) package/.
	cp $(HEXFILE) package/.
	cp $(ELFFILE) package/.
	cp $(OUT)/$(PROGNAME).map package/.
	cp $(PROJ_DIR)/CHANGELOG.md package/.
	RELVER=$$(<$(BUILD_DIR)/next-version); echo $$RELVER; \
	PKGNAME=$(PROGNAME)-$$RELVER; rm -rf $$PKGNAME; \
	mv package $$PKGNAME; \
	tar -zcvf $$PKGNAME.tar.gz $$PKGNAME/*

.PHONY: ci-status
ci-status:
	cd $(CI_PROJECT_DIR); $(VERTOOL) --version
	-cd $(CI_PROJECT_DIR); $(VERTOOL) test-git
	cd $(CI_PROJECT_DIR); $(VERTOOL) test-api
	cd $(CI_PROJECT_DIR); git status

# versioning tool must be run from top level project directory (where .git is)
.PHONY: release
release: nextver changelog
	cd $(PROJ_DIR); $(VERTOOL) tag-and-commit CHANGELOG.md
	RELEASE_VERSION=$$(<$(BUILD_DIR)/next-version); \
	ARTIFACT=$$(realpath --relative-to $(PROJ_DIR) $(BUILD_DIR))/bmsnode-$$RELEASE_VERSION.tar.gz; \
	cd $(PROJ_DIR); echo $$ARTIFACT; \
	$(VERTOOL) --ci-commit-tag v$$RELEASE_VERSION add-download -f $$ARTIFACT -d "BMSNode Firmware Release Package"
