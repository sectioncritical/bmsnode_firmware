#!/usr/bin/env python

import time
import intelhex
import argparse
import sys

_epilog = """
This program generates a "provisioning" hex file that is meant to be used by
'avrdude' for programming into a device eeprom. The provisionig hex file
contains a board unique ID that is generated when this program runs, and a
board identified. Both of these values are used by the BMSNode firmware. For
example:

    ./provtool.py --write boardprov.hex --board oshparkv4

will generate a new provisioning file for an oshparkv4 board. The file must
be used by avrdude to program into the device.

This program can also be used to pretty print the contents of the provisioning
hex file. This file could either have been generated by this program, or come
from avrdude reading the contents of an existing board. For example:

    ./provtool.py --read boardprov.hex

will print the contents of the hex file, interpreted as provisioning data.

Either --write or --read is required. And --write also requires --board.

You can get a list of boards with:

    ./provtool.py --board ?

This program is meant to be used with a Makefile or higher level script that
also handles the avrdude steps.
 
"""

board_list = """
Board List
==========
oshparkv4   - original oshpark prototypes
stuartv42   - small lot of v4.2 from stuart
bmsnode1    - new board design
jlclot1     - v1 board from JLCPCB first lot"
"""

boards = { "oshparkv4": 1, "stuartv42": 2 , "bmsnode1": 3, "jlclot1": 4 }


parser = argparse.ArgumentParser(description="BMSNode Provisioning Tool",
                                 formatter_class=argparse.RawDescriptionHelpFormatter,
                                 epilog=_epilog)
parser.add_argument('-r', "--read", metavar="FILE",
                    help="provisioning hex file to read and display contents")
parser.add_argument('-w', "--write", metavar="FILE",
                    help="provisioning hex file to write for programming")
parser.add_argument('-b', "--board", metavar="NAME",
                    help="name of board (board type - '?' for list)")
args = parser.parse_args()

if args.board and args.board== "?":
    print(board_list)
    sys.exit(0)

if args.write:
    if not args.board:
        print("Write operation requires a board type")
        sys.exit(1)

    if not args.board in boards:
        print("Invalid board type '{}'".format(args.board))
        sys.exit(1)

    boardnum = boards[args.board]
    with open(args.write, "w") as hexfile:
        uid = round(time.time())
        uidbytes = uid.to_bytes(4, "little")

        ih = intelhex.IntelHex()

        ih[0x1FB] = boardnum
        ih[0x1FC] = uidbytes[0]
        ih[0x1FD] = uidbytes[1]
        ih[0x1FE] = uidbytes[2]
        ih[0x1FF] = uidbytes[3]

        ih.tofile(hexfile, format="hex")

        uidstr = '-'.join(format(x, "02X") for x in uidbytes)
        print("Generated UID: {:s}".format(uidstr))

elif args.read:
    with open(args.read, "r") as hexfile:
        ih = intelhex.IntelHex()
        ih.fromfile(hexfile, format="hex")
        boardnum = ih[0x1FB]
        uidbytes = bytes([ih[0x1FC], ih[0x1FD], ih[0x1FE], ih[0x1FF]])
        uid = int.from_bytes(uidbytes, "little")

        print("")

        board = [brd for brd,num in boards.items() if num==boardnum]
        if len(board) == 0:
            print("Invalid board type ({})".format(boardnum))
        else:
            print("Board type  : {}".format(board[0]))

        # reasonable range for uid validation
        # apr 1 2020 through dec 2021
        min_uid = round(time.mktime((2020,  4,  1, 0, 0, 0, 0, 0, -1)))
        max_uid = round(time.mktime((2021, 12, 31, 0, 0, 0, 0, 0, -1)))
        uidstr = '-'.join(format(x, "02X") for x in uidbytes)

        if (uid < min_uid) or (uid > max_uid):
            print("*** UID appears invalid *** :", uidstr)
        else:
            print("UID         : {:s}".format(uidstr))
            print("Birthday    : {:s} UTC".format(time.asctime(time.gmtime(uid))))
        print("")

