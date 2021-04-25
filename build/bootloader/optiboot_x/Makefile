# SPDX-License-Identifier: MIT
#
# Copyright 2021 Joseph Kroesche
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

all: help

.PHONY: help
help:
	@echo ""
	@echo "optiboot_x  - build optiboot for bmsnodev3 and copy here"
	@echo "dev         - clone using ssh for development (allow pushing)"
	@echo "realclean   - remove the optiboot repo"

OPTIBOOT_X_PATH=megaTinyCore/megaavr/bootloaders/optiboot_x
OPTIBOOT_BMS_HEX=optiboot_bmsnodev3.hex
PACKS_DIR=/opt/avr/ATtiny_DFP

megaTinyCore:
	git clone https://github.com/kroesche/megaTinyCore.git
	git -C $@ switch feature/bmsnodev3

# used for development - for pushing back
megaTinyCore_dev:
	git clone git@github.com:kroesche/megaTinyCore.git $@
	git -C $@ remote add upstream https://github.com/SpenceKonde/megaTinyCore.git

.PHONY: dev
dev: megaTinyCore_dev

$(OPTIBOOT_BMS_HEX): $(OPTIBOOT_X_PATH)/$(OPTIBOOT_BMS_HEX)
	cp $< $@

optiboot_x: $(OPTIBOOT_BMS_HEX)

$(OPTIBOOT_X_PATH)/$(OPTIBOOT_BMS_HEX): megaTinyCore
	make -C $(OPTIBOOT_X_PATH) clean
	make -C $(OPTIBOOT_X_PATH) bmsnodev3 PACKS=$(PACKS_DIR)

realclean:
	rm -rf megaTinyCore megaTinyCore_dev
	git restore $(OPTIBOOT_BMS_HEX)
