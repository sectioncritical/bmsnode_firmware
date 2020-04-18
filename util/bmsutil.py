#!/usr/bin/env python3

import serial
import argparse
# from crccheck.crc import Crc8Itu

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
        print("SENDING", ''.join('{:02X} '.format(x) for x in packet))
        self._serial.write(packet)

    def recv(self):
        pktbytes = None
        nextbyte = self._serial.read(1)[0]
        print("RECV: {:02X} ".format(nextbyte), end="")

        # wait for preamble to show up
        while nextbyte != 0x55:
            nextbyte = self._serial.read(1)[0]
            print("{:02X} ".format(nextbyte), end="")

        # while receiving preables, look for sync
        while True:
            nextbyte = self._serial.read(1)[0]
            print("{:02X} ".format(nextbyte), end="")
            if nextbyte == 0xF0: # found sync, proceed
                break
            if nextbyte != 0x55: # not preamble byte, some error
                return None

        # next 4 bytes should be header
        pktbytes = self._serial.read(4)
        print(''.join('{:02X} '.format(x) for x in pktbytes), end="")

        # read in payload, if any
        if pktbytes[3] != 0:
            pktbytes += self._serial.read(pktbytes[3])
            print(''.join('{:02X} '.format(x) for x in pktbytes), end="")

        # read in crc
        crcbyte = self._serial.read(1)[0]
        crccalc = crc8_ccitt(pktbytes)
        print("{:02X} ".format(crcbyte), end="")

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

def cli():
    ser = serial.Serial("/dev/tty.SLAB_USBtoUART", baudrate=4800)
    ser.timeout = 3

    flush_bus(ser)

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


if __name__ == '__main__':
    cli()
