#!/bin/sh

set -x

nmap -sn 192.168.0.0/24
arp -n
