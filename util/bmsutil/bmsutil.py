#!/usr/bin/env python3

import serial
import argparse
import re

def crc8_ccitt(pktbytes):
    crc = 0

    for inbyte in pktbytes:

        databyte = crc ^ inbyte

        for idx in range(8):
            if (databyte & 0x80) != 0:
                databyte <<= 1
                databyte ^= 0x07
            else:
                databyte <<= 1

        crc = databyte & 0xFF

    return crc

class NodePacket(object):
    def __init__(self, address=0, command=0, reply=False, payload=None):
        self._addr = address
        self._cmd = command
        self._reply = reply
        self._dat = payload
        self._len = len(payload) if payload else 0
        self._flags = 0x80 if reply else 0
        self._serial = None
        self._verbose = False

    def __repr__(self):
        return str(vars(self))

    def __str__(self):
        info = "address: {} command: {} flags: 0x{:02X} payload len: {}\n".format(
                self._addr, self._cmd, self._flags, self._len)
        if self._dat:
            for datitem in self._dat:
                info += "{:02X} ".format(datitem)
            info += "\n"
        return info

    @property
    def verbose(self):
        return self._verbose

    @verbose.setter
    def verbose(self, isverbose):
        if not isinstance(isverbose, bool):
            raise TypeError("verbose property should be a bool")
        self._verbose = isverbose

    @property
    def address(self):
        return self._addr

    @address.setter
    def address(self, address):
        self._addr = address

    @property
    def command(self):
        return self._cmd

    @command.setter
    def command(self, command):
        self._cmd = command

    # length is read-only. it gets set through payload
    @property
    def length(self):
        return self._len

    # flags is read-only. it gets set via flag properties
    @property
    def flags(self):
        return self._flags

    @property
    def reply(self):
        return self._reply

    @reply.setter
    def reply(self, reply):
        if not isinstance(reply, bool):
            raise TypeError("reply property should be a bool")
        self._reply = reply
        self._flags = 0x80 if reply else 0

    @property
    def payload(self):
        return self._dat

    @payload.setter
    def payload(self, payload):
        if payload:
            if not isinstance(payload, list):
                raise TypeError("payload should be a list")
        self._dat = payload
        self._len = len(payload) if payload else 0

    @property
    def serport(self):
        return self._serial

    @serport.setter
    def serport(self, serport):
        if not isinstance(serport, serial.Serial):
            raise TypeError("serport property should be class serial.Serial")
        self._serial = serport

    def send(self):
        packet = [self._flags, self._addr, self._cmd, self._len]
        if self._dat:
            packet += self._dat
        crc = crc8_ccitt(packet)
        packet.append(crc)
        presync = [ 0x55, 0x55, 0x55, 0x55, 0xF0 ]
        packet = bytes(presync + packet)
        if self._verbose:
            print("SEND", ''.join('{:02X} '.format(x) for x in packet))
        self._serial.write(packet)

    # TODO: need to rewrite as state machine with singe byte read at top
    # with timeout used to exit
    def recv(self):
        pktbytes = None
        nextbytes = self._serial.read(1)
        if len(nextbytes) != 1:
            return None

        nextbyte = nextbytes[0]
        if self._verbose:
            print("RECV: {:02X} ".format(nextbyte), end="")

        # wait for preamble to show up
        while nextbyte != 0x55:
            nextbytes = self._serial.read(1)
            if len(nextbytes) != 1:
                return None
            nextbyte = nextbytes[0]
            if self._verbose:
                print("{:02X} ".format(nextbyte), end="")

        # while receiving preables, look for sync
        while True:
            nextbytes = self._serial.read(1)
            if len(nextbytes) != 1:
                return None
            nextbyte = nextbytes[0]
            if self._verbose:
                print("{:02X} ".format(nextbyte), end="")
            if nextbyte == 0xF0: # found sync, proceed
                break
            if nextbyte != 0x55: # not preamble byte, some error
                return None

        # next 4 bytes should be header
        pktbytes = self._serial.read(4)
        if len(pktbytes) != 4:
            return None
        if self._verbose:
            print(''.join('{:02X} '.format(x) for x in pktbytes), end="")

        # read in payload, if any
        if pktbytes[3] != 0:
            pktbytes += self._serial.read(pktbytes[3])
            if self._verbose:
                print(''.join('{:02X} '.format(x) for x in pktbytes), end="")

        # read in crc
        crcbyte = self._serial.read(1)[0]
        crccalc = crc8_ccitt(pktbytes)
        if self._verbose:
            print("{:02X} ".format(crcbyte))

        if crcbyte != crccalc:
            print("CRC mismatch: calc=0x{:02X} pkt=0x{:02X}".format(crccalc, crcbyte))
            return None

        newpkt = type(self)()
        newpkt.address = pktbytes[1]
        newpkt.command = pktbytes[2]
        newpkt.reply = True if pktbytes[0] == 0x80 else False

        length = pktbytes[3]
        newpkt.payload = list(pktbytes[4:4+length]) if length > 0 else None

        return newpkt

def flush_bus(serport):
    preambles = bytes([0x55] * 20)
    serport.write(preambles)

def discover(serport, verbose=False):
    saved_timeout = serport.timeout
    serport.timeout = 0.02
    print("Searching for devices")
    for addr in range(1, 17):
        pkt = NodePacket(address=addr, command=1)
        pkt.verbose = verbose
        pkt.serport = serport
        pkt.send()

        while (True):
            rpkt = NodePacket()
            rpkt.verbose = verbose
            rpkt.serport = serport
            rpkt = rpkt.recv()
            if rpkt:
                if rpkt.address == addr and rpkt.command == 1 and rpkt.reply:
                    # print("")
                    print("{} ".format(addr), end="")
                    break
            else:
                print(".", end="", flush=True)
                break
    print("")
    serport.timeout = saved_timeout

def dfu(serport, addr, verbose=False):
    saved_timeout = serport.timeout
    serport.timeout = 0.2

    print("Setting device {} into DFU mode".format(addr))
    pkt = NodePacket(address=addr, command=2)
    pkt.serport = serport
    pkt.send()

    # flush incoming packets before returning
    while (True):
        rpkt = NodePacket()
        rpkt.serport = serport
        rpkt = rpkt.recv()
        if not rpkt:
            break
    serport.timeout = saved_timeout

_board_types = ["unkown", "oshparkv4", "stuartv42"]

def uid(serport, addr, verbose=False):
    saved_timeout = serport.timeout
    serport.timeout = 1

    pkt = NodePacket(address=addr, command=3)
    pkt.serport = serport
    pkt.verbose = verbose
    pkt.send()

    while (True):
        rpkt = NodePacket()
        rpkt.verbose = verbose
        rpkt.serport = serport
        rpkt = rpkt.recv()
        if rpkt:
            if rpkt.address == addr and rpkt.command == 3 and rpkt.reply:
                uid = rpkt.payload[0:4]
                uidstr = '-'.join(format(x, "02X") for x in uid)
                print("")
                boardnum = rpkt.payload[4]
                boardstr = _board_types[boardnum] if boardnum <= len(_board_types) else "invalid"
                print("board type: {}".format(boardstr))
                print("device address: {} / UID: {}".format(addr, uidstr))
                verbytes = rpkt.payload[5:8]
                print("firmware version: {}.{}.{}".format(verbytes[0], verbytes[1], verbytes[2]))
                print("")
                break
        else:
            print("no reply")
            break

    print("")
    serport.timeout = saved_timeout

def adcraw(serport, addr, verbose=False):
    saved_timeout = serport.timeout
    serport.timeout = 1

    pkt = NodePacket(address=addr, command=5)
    pkt.serport = serport
    pkt.verbose = verbose
    pkt.send()

    while (True):
        rpkt = NodePacket()
        rpkt.verbose = verbose
        rpkt.serport = serport
        rpkt = rpkt.recv()
        if rpkt:
            if rpkt.address == addr and rpkt.command == 5 and rpkt.reply:
                adc0 = rpkt.payload[0] | (rpkt.payload[1] << 8)
                adc1 = rpkt.payload[2] | (rpkt.payload[3] << 8)
                adc2 = rpkt.payload[4] | (rpkt.payload[5] << 8)
                print("")
                print("Raw ADC data for node", addr)
                print("")
                print("cell        : 0x{:03X} {:4d}".format(adc0, adc0))
                print("thermistor  : 0x{:03X} {:4d}".format(adc1, adc1))
                print("sensor      : 0x{:03X} {:4d}".format(adc2, adc2))
                print("")
                break
        else:
            print("no reply")
            break

    print("")
    serport.timeout = saved_timeout

def status(serport, addr, verbose=False):
    saved_timeout = serport.timeout
    serport.timeout = 1

    pkt = NodePacket(address=addr, command=6)
    pkt.serport = serport
    pkt.verbose = verbose
    pkt.send()

    while (True):
        rpkt = NodePacket()
        rpkt.verbose = verbose
        rpkt.serport = serport
        rpkt = rpkt.recv()
        if rpkt:
            if rpkt.address == addr and rpkt.command == 6 and rpkt.reply:
                mvolts = rpkt.payload[0] | (rpkt.payload[1] << 8)
                tempc = rpkt.payload[2] | (rpkt.payload[3] << 8)
                print("")
                print("Cell voltage {:4} mV".format(mvolts))
                print("Temperature  {:4} C".format(tempc))
                print("")
                break
        else:
            print("no reply")
            break

    print("")
    serport.timeout = saved_timeout

def setaddr(serport, addr, uid, verbose=False):
    # validate the UID format first
    r = re.compile("^[0-9a-zA-Z]{2}-[0-9a-zA-Z]{2}-[0-9a-zA-Z]{2}-[0-9a-zA-Z]{2}$")
    if not r.match(uid):
        print("Improperly formatted UID string. Should be like '12-34-56-AB'")
        return

    # get rid of dashes so we can convert to bytes
    uidstr = uid.replace('-', ' ')
    uidbytes = bytes.fromhex(uidstr)

    saved_timeout = serport.timeout
    serport.timeout = 1

    pkt = NodePacket(address=addr, command=4, payload=list(uidbytes))
    pkt.serport = serport
    pkt.verbose= verbose
    pkt.send()

    while (True):
        rpkt = NodePacket()
        rpkt.verbose = verbose
        rpkt.serport = serport
        rpkt = rpkt.recv()
        if rpkt:
            if rpkt.address == addr and rpkt.command == 4 and rpkt.reply:
                uid = rpkt.payload
                uidstr = '-'.join(format(x, "02X") for x in uid)
                print("")
                print("device address: {} set for UID: {}".format(addr, uidstr))
                print("")
                break
        else:
            print("no reply")
            break
    serport.timeout = saved_timeout

def cli():
    # init the serial port
    ser = serial.Serial("/dev/tty.SLAB_USBtoUART", baudrate=4800)
    ser.timeout = 3

    parser = argparse.ArgumentParser(description="BMS cell board utility")
    parser.add_argument('-v', "--verbose", action="store_true",
                        help="turn on some debug output")
    subp = parser.add_subparsers(title="commands", dest="cmdname")

    pdiscover = subp.add_parser("discover", help="Scan for devices")

    pboot = subp.add_parser("dfu", help="Set device into DFU mode")
    pboot.add_argument('-a', "--address", type=int, required=True, help="device address to bootload")

    puid = subp.add_parser("uid", help="Get device UID")
    puid.add_argument('-a', "--address", type=int, default=0, help="device address to query (0)")

    padc = subp.add_parser("adc", help="Get raw ADC values")
    padc.add_argument('-a', "--address", type=int, default=0, help="device address to query (0)")

    psts = subp.add_parser("status", help="Get node status")
    psts.add_argument('-a', "--address", type=int, default=0, help="device address to query (0)")

    paddr = subp.add_parser("addr", help="Set device address")
    paddr.add_argument('-a', "--address", type=int, required=True, help="new bus address for device")
    paddr.add_argument('-u', "--uid", type=str, required=True, help="UID of device to set")

    args = parser.parse_args()

    if args.cmdname == "discover":
        flush_bus(ser)
        discover(ser, verbose=args.verbose)

    elif args.cmdname == "dfu":
        flush_bus(ser)
        dfu(ser, args.address)

    elif args.cmdname == "uid":
        flush_bus(ser)
        uid(ser, args.address, verbose=args.verbose)

    elif args.cmdname == "addr":
        flush_bus(ser)
        setaddr(ser, args.address, args.uid, verbose=args.verbose)

    elif args.cmdname == "adc":
        flush_bus(ser)
        adcraw(ser, args.address, verbose = args.verbose)

    elif args.cmdname == "status":
        flush_bus(ser)
        status(ser, args.address, verbose = args.verbose)

"""
    pmonitor = subp.add_parser("monitor", help="Continuous monitoring, q to quit")
    pdump = subp.add_parser("dump", help="Dump configuration(s)")
    pdump.add_argument('-a', "--address", type=int, required=True, help="specific device address")
    pdump.add_argument('-j', "--json", help="name of json file, if any")
    pvcal = subp.add_parser("vcal", help="Voltage calibration")
"""
"""
    # ping packet
    pkt = NodePacket(address=1, command=1, reply=False)
    pkt.serport = ser
    print("")
    print("Outgoing packet")
    print(pkt)
    print("")

    pkt.send()

    # should receive two packet, the original and the reply
    rpkt = NodePacket()
    rpkt.serport = ser
    rpkt = pkt.recv()

    print("")
    print("Incoming packet")
    print(rpkt)
    print("")

    rpkt = pkt.recv()

    print("")
    print("Incoming packet")
    print(rpkt)
    print("")

    # boot packet
    pkt = NodePacket(address=1, command=2, reply=False)
    pkt.serport = ser
    print("")
    print("Outgoing packet")
    print(pkt)
    print("")

    pkt.send()

    rpkt = NodePacket()
    rpkt.serport = ser
    rpkt = pkt.recv()

    print("")
    print("Incoming packet")
    print(rpkt)
    print("")

"""

if __name__ == '__main__':
    cli()
