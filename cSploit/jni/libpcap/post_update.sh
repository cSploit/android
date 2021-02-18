#!/bin/bash

# $1 Path to the new version.
# $2 Path to the old version.

cp -a -n $2/config.h $1/
cp -a $2/.gitignore $1/
cp -a -n $2/pcap-netfilter-linux-android.h $1/
cp -a -n $2/pcap-netfilter-linux-android.c $1/
cp -a -n $2/grammar.c $1/
cp -a -n $2/grammar.h $1/
cp -a -n $2/scanner.c $1/
cp -a -n $2/scanner.h $1/

