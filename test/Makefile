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

PROGNAME=bmstest

OBJDIR=obj

C_SRCS=../src/pkt.c
CPP_SRCS=test_main.cpp test_pkt.cpp

C_OBJS:=$(C_SRCS:%.c=%.o)
CPP_OBJS:=$(CPP_SRCS:%.cpp=%.o)
C_OBJS:=$(addprefix $(OBJDIR)/, $(notdir $(C_OBJS)))
CPP_OBJS:=$(addprefix $(OBJDIR)/, $(notdir $(CPP_OBJS)))

INCS=-I ./ -I../src

VPATH=./ ../src

CC=gcc
CXX=g++

CFLAGS=-std=c99 -Os -Werror -Wall 
CXXFLAGS=-std=c++11 -O0 -Werror -Wall

all: $(PROGNAME)


$(OBJDIR):
	mkdir -p $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCS) -o $@  -c $<

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INCS) -o $@  -c $<

$(PROGNAME): $(C_OBJS) $(CPP_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -rf $(OBJDIR)
	rm -f $(PROGNAME)

$(PROGNAME)-junit.xml: $(PROGNAME)
	./$(PROGNAME) -r junit > $@
	./$(PROGNAME) -s

runtest: $(PROGNAME)-junit.xml
