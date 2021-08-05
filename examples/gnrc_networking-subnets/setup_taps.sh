#!/bin/bash

PREFIX=2001:db8::/60

RIOTBASE=../..
RIOTTOOLS=$RIOTBASE/dist/tools
TOPOLOGY=tapology.txt

setup() {
	# add address to loopback so we can test reaching an
	# address outside the test net
	sudo ip a a fd00:dead:beef::1/128 dev lo

	echo "creating tap interfaces"
	i=0
	while read -r level num; do
		$(printf -- "sudo %s/tapsetup/tapsetup -b br%s -t tap_%s -c %s\n" $RIOTTOOLS $i $level $num) > /dev/null;
		i=$((i+1))
	done < $1

	# start radvd with a large prefix
	sudo $RIOTTOOLS/radvd/radvd.sh -c br0 $PREFIX
}

teardown() {
	# remove loopback address
	sudo ip a d fd00:dead:beef::1/128 dev lo

	echo "deleting tap interfaces"
	i=0
	while read -r level num; do
		$(printf -- "sudo %s/tapsetup/tapsetup -b br%s -t tap_%s -d\n" $RIOTTOOLS $i $level) > /dev/null;
		i=$((i+1))
	done < $1

	# stop radvd
	sudo $RIOTTOOLS/radvd/radvd.sh -d
}

if [ $# -gt 1 ]; then
	TOPOLOGY=$2
fi

if [ ! -f "$TOPOLOGY" ]; then
	echo "no such file: $TOPOLOGY"
	exit 1
fi

if [ $# -gt 0 ]; then
	case $1 in
	-c)
		;;
	-d)
		teardown $TOPOLOGY
		exit
		;;
	*)
		echo "usage: $0 [-c <topology>] [-d topology]"
		exit 1
		;;
	esac
fi

setup $TOPOLOGY
