#!/bin/sh
# script to re-generate mavlink C code for APM

if ! which mavgen.py > /dev/null; then
    echo "mavgen.py must be in your PATH. Get it from http://github.com/mavlink/mavlink in the pymavlink/generator directory"
    exit 1
fi

echo "Removing old includes"
rm -rf "../Headers/MAVLink/*"

echo "Generating C code"
mavgen.py --c2000 --lang=C --output=../Headers/MAVLink/ ardupilotmega.xml
