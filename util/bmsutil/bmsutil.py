#!/usr/bin/env python3

import serial
import argparse
import re
import time
from blessed import Terminal

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

def ping(serport, addr, verbose=False):
    saved_timeout = serport.timeout
    serport.timeout = 0.02
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
                return rpkt
        else:
            return None
    serport.timeout = saved_timeout

def discover(serport, verbose=False):
    print("Searching for devices")
    for addr in range(1, 17):
        resp = ping(serport, addr, verbose)
        if resp:
            print("{} ".format(addr), end="")
        else:
            print(".", end="", flush=True)
    print("")

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

_board_types = ["unknown", "oshparkv4", "stuartv42", "bmsnode1", "jlclot1"]

# return the uid, board type, and firmware version of a particular address
# return is a tuple with formatted string
# ("xx-xx-xx-xx", "oshpark", "1.2.3")
def boardinfo(serport, addr, verbose=False):
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
                # get the uid and format as string
                uid = rpkt.payload[0:4]
                uidstr = '-'.join(format(x, "02X") for x in uid)
                # get the board type and convert to string
                boardnum = rpkt.payload[4]
                boardstr = _board_types[boardnum] if boardnum <= len(_board_types) else "invalid"
                # get the firmware version and format as string
                verbytes = rpkt.payload[5:8]
                verstr = "{}.{}.{}".format(verbytes[0], verbytes[1], verbytes[2])
                return uidstr, boardstr, verstr
        else:
            return None
    serport.timeout = saved_timeout

def uid(serport, addr, verbose=False):
    brdinfo = boardinfo(serport, addr, verbose)
    if brdinfo:
        print("board type: {}".format(brdinfo[1]))
        print("device address: {} / UID: {}".format(addr, brdinfo[0]))
        print("firmware version: {}".format(brdinfo[2]))
    else:
        print("Board {} was not found".format(addr))

    print("")

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

# return tuple of status
# (999, 99, "OFF") # mv, C, str
# first two are integers
def getstatus(serport, addr, verbose=False):
    statusnames = [ "OFF", "IDLE", "ON", "UNDERVOLT", "OVERTEMP" ]
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
                shunt_status = rpkt.payload[4]
                shuntstr = statusnames[shunt_status]
                return mvolts, tempc, shuntstr
        else:
            return None
    serport.timeout = saved_timeout

def status(serport, addr, verbose=False):
    brdstatus = getstatus(serport, addr, verbose)
    if brdstatus:
        print("")
        print("Cell voltage {:4} mV".format(brdstatus[0]))
        print("Temperature  {:4} C".format(brdstatus[1]))
        print("Shunt        {:s}".format(brdstatus[2]))
        print("")
    else:
        print("Board {} was not found".format(addr))
    print("")

#             (id, bytes, name)
parmtable = [ (1, 1, "ADDR"),
              (2, 2, "VSCALE"),
              (3, 2, "VOFFSET"),
              (4, 2, "TSCALE"),
              (5, 2, "TOFFSET"),
              (6, 2, "XSCALE"),
              (7, 2, "XOFFSET"),
              (8, 2, "SHUNTON"),
              (9, 2, "SHUNTOFF"),
              (10, 2, "SHUNTTIME"),
              (11, 1, "TEMPHI"),
              (12, 1, "TEMPLO"),
              (13, 2, "TEMPADJ")]

def show_parmlist():
    for parm in parmtable:
        print(parm[2])

def setparm(serport, addr, parmname, parmval, verbose=False):
    # get the parameter first
    parmlist = [parm for parm in parmtable if parm[2] == parmname]
    if len(parmlist) != 1:
        print("problem with parameter name: {}".format(parmname))
        return
    theparm = parmlist[0]
    parmid = theparm[0]
    parmsize = theparm[1]
    print("setting parameter {} with ID {}, size {} bytes, to value {}".format(
        parmname, parmid, parmsize, parmval))

    # construct payload with parameter id and value
    # TODO add validation of values here
    payload = [parmid]
    if parmsize == 1:
        payload.extend([parmval])
    elif parmsize == 2:
        payload.extend([parmval & 0xFF, parmval >> 8])
    else:
        print("there is some unknown problem with parameter length")
        return

    saved_timeout = serport.timeout
    serport.timeout = 1

    pkt = NodePacket(address=addr, command=9, payload=payload)
    pkt.serport = serport
    pkt.verbose= verbose
    pkt.send()

    while (True):
        rpkt = NodePacket()
        rpkt.verbose = verbose
        rpkt.serport = serport
        rpkt = rpkt.recv()
        if rpkt:
            if rpkt.address == addr and rpkt.command == 9 and rpkt.reply:
                print("")
                print("command acknowledged, you should read back to verify")
                print("")
                break
        else:
            print("no reply")
            break
    serport.timeout = saved_timeout

def getparms(serport, addr, verbose=False):
    saved_timeout = serport.timeout
    serport.timeout = 1

    print("")
    print("Parameters for node: {}".format(addr))
    print("----------------------")

    for parm in parmtable:
        # generate a GETPARM packet for each parameter
        pkt = NodePacket(address=addr, command=10, payload=[parm[0]])
        pkt.serport = serport
        pkt.verbose = verbose
        pkt.send()

        # process reply
        while (True):
            rpkt = NodePacket()
            rpkt.verbose = verbose
            rpkt.serport = serport
            rpkt = rpkt.recv()
            if rpkt:
                if rpkt.address == addr and rpkt.command == 10 and rpkt.reply:
                    parmid = rpkt.payload[0] # parameter ID from reply

                    # check that parm ID in reply matches what we asked for
                    if parmid != parm[0]:
                        print("parameter ID mismatch: expected({}), found ({})".format(parm[0], parmid))
                        break

                    # make sure the reply payload length matches our expectation
                    if (rpkt.length - 1) != parm[1]:
                        print("parameter value length mismatch: expected ({}) found ({})".format(parm[1] + 1, rpkt.length))
                        break

                    # contruct payload value from one or two bytes
                    if rpkt.length == 2:
                        parmval = rpkt.payload[1]
                    elif rpkt.length == 3:
                        parmval = rpkt.payload[1] + (rpkt.payload[2] << 8)
                    else:
                        # somehow our length is still bad
                        print("unexpected parameter packet length ({})".format(rpkt.length))
                        break

                    # print parm name and value
                    print("{:10s}: {}".format(parm[2], parmval))

                    break
            else:
                print("{}: no reply".format(parm[2]))
                break

    print("")
    serport.timeout = saved_timeout

def shunt(serport, addr, shunt_state, verbose=False):
    saved_timeout = serport.timeout
    serport.timeout = 1

    shunt_command = 7 if shunt_state == 1 else 8

    pkt = NodePacket(address=addr, command=shunt_command)
    pkt.serport = serport
    pkt.verbose = verbose
    pkt.send()

    while (True):
        rpkt = NodePacket()
        rpkt.verbose = verbose
        rpkt.serport = serport
        rpkt = rpkt.recv()
        if rpkt:
            if rpkt.address == addr and rpkt.command == shunt_command and rpkt.reply:
                break
        else:
            print("no reply")
            break

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

layout ="""
┌────┬─────────┬───────┬───────────┬─────────────┬────────────────┬─────────┐
│ ID │ VOLTAGE │ TEMP  │   SHUNT   │     UID     │     TYPE       │ VERSION │
├────┼─────────┼───────┼───────────┼─────────────┼────────────────┼─────────┤
│999 │ 9999 mV │-999 C │ SSSSSSSSS │ XX-XX-XX-XX │ TTTTTTTTTTTTTT │99.99.99 │
└────┴─────────┴───────┴───────────┴─────────────┴────────────────┴─────────┘
"""
heading = """┌────┬─────────┬───────┬───────────┬─────────────┬────────────────┬─────────┐
│ ID │ VOLTAGE │ TEMP  │   SHUNT   │     UID     │     TYPE       │ VERSION │
├────┼─────────┼───────┼───────────┼─────────────┼────────────────┼─────────┤"""

footing = "└────┴─────────┴───────┴───────────┴─────────────┴────────────────┴─────────┘"
infoline = "│{:3d} │ {:4d} mV │{:4d} C │ {:9s} │ {:11s} │ {:14s} │{:^8s} │"

def monitor(serport):
    nodelist = [] # (id(int), uid(str), type(str), ver(str))
    print("Searching for devices")
    flush_bus(serport)
    for addr in range(1, 32):
        resp = ping(serport, addr)
        if resp:
            brdinfo = boardinfo(serport, addr)
            if brdinfo:
                nodeinfo = (addr, brdinfo[0], brdinfo[1], brdinfo[2])
                nodelist.append(nodeinfo)
                print("found node {}".format(addr))

    # nodelist now has all the nodes that were found
    if len(nodelist) == 0:
        print("no devices found")
        return

    term = Terminal()
    print(term.clear)
    print(heading)

    userkey = None

    with term.cbreak(), term.hidden_cursor():
        while userkey not in ('q', 'Q'):
            with term.location(0, 4):
                for node in nodelist:
                    flush_bus(serport)
                    sts = getstatus(serport, node[0]) # get realtime status from node
                    if sts:
                        print(infoline.format(node[0], sts[0], sts[1], sts[2], node[1], node[2], node[3]))
                    else:
                        continue # if no response from this node, restart loop
                print(footing)
            time.sleep(0.5)
            userkey = term.inkey(timeout=0)

def cli():
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

    pshunt = subp.add_parser("shunt", help="Turn shunt off or on")
    pshunt.add_argument('-a', "--address", type=int, required=True, help="device address to set")
    pshunt.add_argument('-s', "--state", type=int, required=True, help="0 or 1 (off or on)")

    pget = subp.add_parser("get", help="Get configuration values")
    pget.add_argument('-a', "--address", type=int, required=True, help="device address to query")

    pset = subp.add_parser("set", help="Set configuration value")
    pset.add_argument('-a', "--address", type=int, help="device address to set parameter")
    pset.add_argument('-p', "--parameter", type=str, required=True, help="name of parameter to set (? for list)")
    pset.add_argument('-v', "--value", type=int, help="value of parameter")

    pmon = subp.add_parser("monitor", help="Continuous status (q to exit)")

    args = parser.parse_args()

    # init the serial port
    ser = serial.Serial("/dev/tty.usbserial-A107LYRO", baudrate=4800)
    ser.timeout = 3

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

    elif args.cmdname == "shunt":
        flush_bus(ser)
        shunt(ser, args.address, shunt_state=args.state, verbose=args.verbose)

    elif args.cmdname == "get":
        flush_bus(ser)
        getparms(ser, args.address, verbose=args.verbose)

    elif args.cmdname == "set":
        if args.parameter == "?":
            show_parmlist()
        else:
            flush_bus(ser)
            setparm(ser, args.address, args.parameter, args.value, verbose=args.verbose)

    elif args.cmdname == "monitor":
        monitor(ser)

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
