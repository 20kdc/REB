#!/bin/sh
gcc rebhelpc.c -o rebhelpc
./rebhelpc > buildhelp2.sh
chmod +x buildhelp2.sh
./buildhelp2.sh
