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

VPATH=./ ../src avr/

TESTS=bmstest_pkt bmstest_serial

PKT_OBJS=test_main.o test_pkt.o pkt.o
SER_OBJS=test_main.o test_serial.o serial.o io.o

TEST_PKT_OBJS=$(addprefix $(OBJDIR)/, $(PKT_OBJS))
TEST_SER_OBJS=$(addprefix $(OBJDIR)/, $(SER_OBJS))

TESTBINS=$(addprefix $(BINDIR)/, $(TESTS))
REPORTS=$(addprefix $(REPORTDIR)/, $(addsuffix -junit.xml, $(TESTS)))

CC=gcc
CXX=g++

CFLAGS=-std=c99 -O0 --coverage -Werror -Wall -DUNIT_TEST
CXXFLAGS=-g -std=c++11 -O0 -Werror -Wall

LDFLAGS=--coverage

all: $(TESTBINS)

# packet test dependencies
$(BINDIR)/bmstest_pkt: $(TEST_PKT_OBJS) | $(BINDIR)

# serial test dependencies
$(BINDIR)/bmstest_serial: $(TEST_SER_OBJS) | $(BINDIR)

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
	$< -r junit > $@

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

report: $(REPORTDIR)/bmstest-junit.html $(REPORTDIR)/bmstest-coverage.html

.PHONY: runtest
runtest: $(TESTBINS)
	@for bin in $(TESTBINS); do echo "TEST: $$bin"; $$bin; done

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(BINDIR) $(REPORTDIR)

# set up python virtual environment
venv: venv/bin/activate

venv/bin/activate: requirements.txt
	test -d venv || python -m venv venv
	. venv/bin/activate; pip install -U pip; pip install -Ur $<
	touch venv/bin/activate

$(OBJDIR):
	mkdir -p $@

$(BINDIR):
	mkdir -p $@

$(REPORTDIR):
	mkdir -p $@

