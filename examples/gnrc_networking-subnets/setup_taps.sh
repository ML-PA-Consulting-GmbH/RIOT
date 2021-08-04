#!/bin/bash

PREFIX=2001:db8::/32

RIOTBASE=../..
RIOTTOOLS=$RIOTBASE/dist/tools

# add address to loopback so we can test reaching an
# address outside the test net
sudo ip a a fd00:dead:beef::1/128 dev lo

# create topology
echo "create new interfaces"
sudo $RIOTTOOLS/tapsetup/tapsetup -b br0 -t tap_a -c 1
sudo $RIOTTOOLS/tapsetup/tapsetup -b br1 -t tap_b -c 3
sudo $RIOTTOOLS/tapsetup/tapsetup -b br2 -t tap_c -c 3
sudo $RIOTTOOLS/tapsetup/tapsetup -b br3 -t tap_d -c 2
sudo $RIOTTOOLS/tapsetup/tapsetup -b br4 -t tap_e -c 2

# start radvd with a large prefix
echo "starting daemons"
sudo $RIOTTOOLS/radvd/radvd.sh -c br0 $PREFIX
