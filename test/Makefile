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

OBJDIR=obj
BINDIR=bin
REPORTDIR=reports

INCS=-I ./ -I../src

VPATH=./ ../src avr/ util/

TESTS=bmstest_main bmstest_pkt bmstest_ser bmstest_cmd bmstest_cfg bmstest_tmr bmstest_adc bmstest_shunt bmstest_testmode bmstest_led bmstest_list bmstest_kissm

MAIN_OBJS=test_main.o test_app.o main.o io.o
PKT_OBJS=test_main.o test_pkt.o pkt.o crc16.o
SER_OBJS=test_main.o test_ser.o ser.o io.o
CMD_OBJS=test_main.o test_cmd.o cmd.o io.o crc16.o ver.o
CFG_OBJS=test_main.o test_cfg.o cfg.o crc16.o
TMR_OBJS=test_main.o test_tmr.o tmr.o io.o list.o
ADC_OBJS=test_main.o test_adc.o adc.o io.o thermistor_table.o
SHUNT_OBJS=test_main.o test_shunt.o shunt.o io.o
TESTMODE_OBJS=test_main.o test_testmode.o testmode.o io.o
LED_OBJS=test_main.o test_led.o led.o io.o
LIST_OBJS=test_main.o test_list.o list.o
KISSM_OBJS=test_main.o test_kissm.o kissm.o

TEST_MAIN_OBJS=$(addprefix $(OBJDIR)/, $(MAIN_OBJS))
TEST_PKT_OBJS=$(addprefix $(OBJDIR)/, $(PKT_OBJS))
TEST_SER_OBJS=$(addprefix $(OBJDIR)/, $(SER_OBJS))
TEST_CMD_OBJS=$(addprefix $(OBJDIR)/, $(CMD_OBJS))
TEST_CFG_OBJS=$(addprefix $(OBJDIR)/, $(CFG_OBJS))
TEST_TMR_OBJS=$(addprefix $(OBJDIR)/, $(TMR_OBJS))
TEST_ADC_OBJS=$(addprefix $(OBJDIR)/, $(ADC_OBJS))
TEST_SHUNT_OBJS=$(addprefix $(OBJDIR)/, $(SHUNT_OBJS))
TEST_TESTMODE_OBJS=$(addprefix $(OBJDIR)/, $(TESTMODE_OBJS))
TEST_LED_OBJS=$(addprefix $(OBJDIR)/, $(LED_OBJS))
TEST_LIST_OBJS=$(addprefix $(OBJDIR)/, $(LIST_OBJS))
TEST_KISSM_OBJS=$(addprefix $(OBJDIR)/, $(KISSM_OBJS))

TESTBINS=$(addprefix $(BINDIR)/, $(TESTS))
REPORTS=$(addprefix $(REPORTDIR)/, $(addsuffix -junit.xml, $(TESTS)))

CC=gcc
CXX=g++

CFLAGS=-std=c99 -O0 --coverage -Werror -Wall -DUNIT_TEST -DFWVERSION="{4,9,94}"
CXXFLAGS=-g -std=c++11 -O0 -Werror -Wall

LDFLAGS=--coverage

all: $(TESTBINS)

# main test dependencies
$(BINDIR)/bmstest_main: $(TEST_MAIN_OBJS) | $(BINDIR)

# packet test dependencies
$(BINDIR)/bmstest_pkt: $(TEST_PKT_OBJS) | $(BINDIR)

# serial test dependencies
$(BINDIR)/bmstest_ser: $(TEST_SER_OBJS) | $(BINDIR)

# command test dependencies
$(BINDIR)/bmstest_cmd: $(TEST_CMD_OBJS) | $(BINDIR)

# config test dependencies
$(BINDIR)/bmstest_cfg: $(TEST_CFG_OBJS) | $(BINDIR)

# timer test dependencies
$(BINDIR)/bmstest_tmr: $(TEST_TMR_OBJS) | $(BINDIR)

# ADC test dependencies
$(BINDIR)/bmstest_adc: $(TEST_ADC_OBJS) | $(BINDIR)

# SHUNT test dependencies
$(BINDIR)/bmstest_shunt: $(TEST_SHUNT_OBJS) | $(BINDIR)

# TESTMODE test dependencies
$(BINDIR)/bmstest_testmode: $(TEST_TESTMODE_OBJS) | $(BINDIR)

# LED test dependencies
$(BINDIR)/bmstest_led: $(TEST_LED_OBJS) | $(BINDIR)

# List test dependencies
$(BINDIR)/bmstest_list: $(TEST_LIST_OBJS) | $(BINDIR)

# Kissm test dependencies
$(BINDIR)/bmstest_kissm: $(TEST_KISSM_OBJS) | $(BINDIR)

# compile a .c file
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCS) -o $@  -c $<

# compile a .cpp file
$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INCS) -o $@  -c $<

# create test binary
$(BINDIR)/bmstest_%:
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

# create a junit report
$(REPORTDIR)/bmstest_%-junit.xml: $(BINDIR)/bmstest_% | $(REPORTDIR)
	-$< -r junit > $@

# combine all junit xml files
$(REPORTDIR)/bmstest-junit.xml: venv $(REPORTS)
	venv/bin/junit2html --merge $@ $(REPORTDIR)

# generate html junit report
$(REPORTDIR)/bmstest-junit.html: $(REPORTDIR)/bmstest-junit.xml
	venv/bin/junit2html $< $@

# generate code coverage report
$(REPORTDIR)/bmstest-coverage.html: $(REPORTS)
	venv/bin/gcovr -r ../src obj
	venv/bin/gcovr -r ../src --html-details $@ obj

# generate cppcheck xml report
$(REPORTDIR)/check.xml: | $(REPORTDIR)
	cppcheck --std=c99 --platform=avr8 --enable=all -UUNIT_TEST --suppress=missingIncludeSystem -v ../src --xml-version=2 2> $@

# generate cppcheck html report
$(REPORTDIR)/check/index.html: $(REPORTDIR)/check.xml venv | $(REPORTDIR)/check
	. venv/bin/activate; cppcheck-htmlreport --file=$< --report-dir=$(REPORTDIR)/check --source-dir=../src

# generate cppcheck badge
$(REPORTDIR)/check-badge.svg: $(REPORTDIR)/check.xml venv
	CHECK_COUNT=$$(grep -o "error id" $< | wc -l | tr -d [:blank:]); \
	venv/bin/anybadge -l cppcheck -v $$CHECK_COUNT -s " issues" -u -o -f $@ 1=green 6=yellow 7=brightred

# run cppcheck with plain output - useful for local running, and no reports
.PHONY: check
check:
	cppcheck --std=c99 --platform=avr8 --enable=all -UUNIT_TEST --suppress=missingIncludeSystem -v ../src

.PHONY: report
report: $(REPORTDIR)/bmstest-junit.html $(REPORTDIR)/bmstest-coverage.html $(REPORTDIR)/check/index.html $(REPORTDIR)/check-badge.svg checkunit

.PHONY: runtest
runtest: $(TESTBINS)
	@for bin in $(TESTBINS); do echo "TEST: $$bin"; $$bin; done

.PHONY: checkunit
checkunit: $(REPORTDIR)/bmstest-junit.xml
	@if $$(grep -q FAILED $<); then printf "\n*** There are unit test errors ***\n\n"; false; fi

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(BINDIR) $(REPORTDIR)

# set up python virtual environment
venv: venv/bin/activate

PY3=$(shell command -v python3 || command -v python)

venv/bin/activate: requirements.txt
	test -d venv || $(PY3) -m venv venv
	. venv/bin/activate; pip install -U pip setuptools wheel; pip install -Ur $<
	touch venv/bin/activate

.PHONY: realclean
realclean: clean
	rm -rf venv

$(OBJDIR):
	mkdir -p $@

$(BINDIR):
	mkdir -p $@

$(REPORTDIR):
	mkdir -p $@

$(REPORTDIR)/check:
	mkdir -p $@
