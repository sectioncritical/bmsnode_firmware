#!/bin/bash

# a simple script to dump all info about a node

DEVICE=/dev/cu.SLAB_USBtoUART

busid=$1

venv/bin/bmsutil -b 9600 -d $DEVICE ping -a $busid
venv/bin/bmsutil -b 9600 -d $DEVICE uid -a $busid
venv/bin/bmsutil -b 9600 -d $DEVICE status -a $busid
venv/bin/bmsutil -b 9600 -d $DEVICE get -a $busid

