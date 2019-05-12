#!/bin/bash

IFACE="tap0"

if [ $UID -ne 0 ]; then
	sudo $0 $@
	exit
fi

ip tuntap add dev $IFACE mode tap
ip address add 192.168.99.5/24 dev $IFACE
ip address show $IFACE
ifconfig $IFACE promiscup
