#!/usr/bin/env python

import sys
import serial
import argparse
from intelhex import IntelHex
"""

SYNC
OUT: STK_GET_SYNC 0x30, CRC_EOP 0x20
RESP: STK_INSYNC 0x14, STK_OK 0x10

GET_PARAMETER:
--> STK_GET_PARAMETER 0x40, STK_SW_MINOR 0x82, CRC_EOP 0x20
<-- STK_INSYNC, value, STK_OK

"""

_verbose = False
_ser = None

# send a command byte string. append CRC_EOP
# expect every byte to be echoed
def send_command(cmdbytes):
    cmd = bytes(bytes(cmdbytes) + bytes([0x20]))
    if _verbose:
        print("  -->", ''.join('{:02X} '.format(x) for x in cmd))
    write_count = _ser.write(cmd)
    cmdecho = _ser.read(len(cmd))
    if _verbose:
        print("  <--", ''.join('{:02X} '.format(x) for x in cmdecho))
    assert cmdecho == cmd

# send GET_SYNC and check result
# return True if good else False
def get_sync():
    if _verbose:
        print("GET_SYNC")
    send_command([0x30])
    resp = _ser.read(2)
    if _verbose:
        print("  <==", ''.join('{:02X} '.format(x) for x in resp))
    if len(resp) == 2 and resp[0] == 0x14 and resp[1] == 0x10:
        if _verbose:
            print("RESPONSE OK")
        return True
    else:
        if _verbose:
            print("BAD RESPONSE ****")
        return False

# send sync commands multiple times to attempt to establish sync
def do_sync():
    _ser.reset_input_buffer()
    _ser.reset_output_buffer()
    print("Establishing SYNC")
    for attempt in range(1, 6):
        print("{}...".format(attempt), end="", flush=True)
        insync = get_sync()
        if insync:
            print("OK")
            return
        # serial port is using 3 second timeout
        # do not add any additional delay here. it means it will retry
        # every 3 seconds if there is no response from target. this gives
        # time for human to press reset button on the board, or whatever.
        # however if there is data received but not expected then there will
        # not be any delay. The normal path is that the board is not responding
        # to boot loader sync

    # getting here means that no valid sync was received
    print("💩")
    print("SYNC was not established")
    sys.exit(1)

# query the BL for its version and print to console
def get_version():
    print("Get Bootloader Version")
    send_command([0x41, 0x82])
    resp = _ser.read(3)
    if _verbose:
        print("  <==", ''.join('{:02X} '.format(x) for x in resp))
    if resp[0] != 0x14 or resp[2] != 0x10:
        print("BAD RESPONSE ****")
        sys.exit(1)
    vermin = resp[1]
    send_command([0x41, 0x81])
    resp = _ser.read(3)
    if _verbose:
        print("  <==", ''.join('{:02X} '.format(x) for x in resp))
    if resp[0] != 0x14 or resp[2] != 0x10:
        print("BAD RESPONSE ****")
        sys.exit(1)
    vermaj = resp[1]
    print("Version {}.{}".format(vermaj, vermin))

# establish sync and get the version
def do_ping():
    do_sync()
    get_version()

# set the load address. applies to reading or writing blocks
# returns True or False
def load_address(addr):
    if _verbose:
        print("Set address 0x{:04X}".format(addr))
    # address is "word" address so /2
    addr = addr >> 1
    send_command([0x55, addr & 0xFF, addr >> 8])
    resp = _ser.read(2)
    if _verbose:
        print("  <==", ''.join('{:02X} '.format(x) for x in resp))
    if len(resp) == 2 and resp[0] == 0x14 and resp[1] == 0x10:
        if _verbose:
            print("RESPONSE OK")
        return True
    else:
        if _verbose:
            print("BAD RESPONSE ****")
        return False

# read a block of 16 bytes from the load address
# returns 16 bytes or None
def read_block():
    if _verbose:
        print("Read block of 16")
    # always reads 16 bytes
    send_command([0x74, 0, 16, 0x46])
    resp = _ser.read(2 + 16)
    if _verbose:
        print("  <==", ''.join('{:02X} '.format(x) for x in resp))
    if len(resp) == 18 and resp[0] == 0x14 and resp[17] == 0x10:
        if _verbose:
            print("Read OK")
        return resp[1:17]
    else:
        if _verbose:
            print("Read BAD")
        return None

# write/program a block of 16 bytes at the load address
# returns True or False
def prog_block(progdata):
    assert len(progdata) == 16
    if _verbose:
        print("Write block of 16")
    # write 16 bytes
    send_command(bytes([0x64, 0, 16, 0x46]) + bytes(progdata))
    _ser.timeout = 10
    resp = _ser.read(2)
    _ser.timeout = 3
    if _verbose:
        print("  <==", ''.join('{:02X} '.format(x) for x in resp))
    if len(resp) == 2 and resp[0] == 0x14 and resp[1] == 0x10:
        if _verbose:
            print("Write OK")
        return True
    else:
        if _verbose:
            print("Bad response")
        return False

# check if the 16-byte block contains any data
# this meant to be used on hexfile data to determine if
# a 16-byte block should be programmed or verified
def block_has_data(datablock):
    assert len(datablock) == 16
    for byteaddr in range(0, 16):
        if datablock[byteaddr] != 255:
            return True
    return False

# program flash from a hex file
def program_flash(filename):
    # assume file is hex file
    ih = IntelHex()
    ih.fromfile(filename, format="hex")

    print("Programming file {}".format(filename))

    do_sync()

    # iterate over all of hex file in 16 byte chunks to look for
    # sections to program
    # go from 0 to 1D80, the start of boot loader
    # TODO validate hex file has reasonable contents
    for address in range(0, 0x1d80, 16):
        print(".", end="", flush=True)
        blockdata = ih[address:address+16].tobinstr()
        if len(blockdata) < 16:
            blockdata = bytes(blockdata + bytes([255] * (16 - len(blockdata))))
        if block_has_data(blockdata):
            ret = load_address(address)
            if not ret:
                print("\nLoad address failed at 0x{:04X}".format(address))
                sys.exit(1)
            ret = prog_block(blockdata)
            if not ret:
                print("\nBlock programming failed at 0x{:04X}".format(address))
                sys.exit(1)
    print("\nProgramming complete")

# verify contents of flash against a hex file
def verify_flash(filename):
    # assume file is hex file
    ih = IntelHex()
    ih.fromfile(filename, format="hex")

    print("Verifying file {}".format(filename))

    do_sync()

    # iterate over all of hex file in 16 byte chunks to look for
    # sections to verify
    # go from 0 to 1D80, the start of boot loader
    # TODO validate hex file has reasonable contents
    for address in range(0, 0x1d80, 16):
        print(".", end="", flush=True)
        blockdata = ih[address:address+16].tobinstr()
        if len(blockdata) < 16:
            blockdata = bytes(blockdata + bytes([255] * (16 - len(blockdata))))
        if block_has_data(blockdata):
            ret = load_address(address)
            if not ret:
                print("\nLoad address failed at 0x{:04X}".format(address))
                sys.exit(1)
            verdata = bytes(read_block())
            if verdata != blockdata:
                print("\nmismatch for block {:04X}".format(address))
                print("file : ", ''.join('{:02X} '.format(x) for x in blockdata))
                print("flash: ", ''.join('{:02X} '.format(x) for x in verdata))
    print("\nVerify complete")

def dump_flash(ser, filename):
    do_sync()

    with open(filename, "wb") as binfile:
        for address in range(0, 8192, 16):
            load_address(ser, address)
            block = read_block(ser)
            binfile.write(block)

def cli():
    global _verbose
    global _ser

    ser_default = "/dev/tty.usbserial-A107LYRO"
    parser = argparse.ArgumentParser(description="BMS Node Firmware Loader")
    parser.add_argument('-v', "--verbose", action="store_true",
                        help="turn on some debug output")
    parser.add_argument('-f', "--file", help="hex file to upload, verify, or save")
    parser.add_argument('-d', "--device", default=ser_default,
                        help="serial device port (/dev/tty.NNN)")
    subp = parser.add_subparsers(title="operation", dest="opname")

    pping = subp.add_parser("ping", help="sync and verify bootloader comms")
    pload = subp.add_parser("upload", help="load hex file to BMSNode")
    pver = subp.add_parser("verify", help="verify hex file loaded in BMSNode")
    psave = subp.add_parser("save", help="save BMSNode memory into local file")

    args = parser.parse_args()

    # init the serial port
    ser = serial.Serial(args.device, baudrate=4800)
    ser.timeout = 3
    _ser = ser
    if not ser:
        print("Could not open serial device {}".format(args.device))
        sys.exit(1)

    # set verbosity level
    _verbose = True if args.verbose else False

    # process operations
    if args.opname == "ping":
        do_ping()

    elif args.opname == "save":
        do_save(ser, args.file)

    # remaining are upload or verify
    elif args.opname == "upload":
        program_flash(args.file)

    elif args.opname == "verify":
        verify_flash(args.file)

    else:
        print("Invalid operation")
        sys.exit(1)

"""
    # open the hex file
    # we assume it is a hex file
    ih = IntelHex()
    ih.fromfile(args.file, format="hex")

    get_sync(ser)
    get_version(ser)
    program_flash(ser, args.file)
    verify_flash(ser, args.file)
"""
if __name__ == "__main__":
    cli()
