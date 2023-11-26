#!/usr/bin/env python

"""
Utility for building a release firmware bundle

"""

import argparse, base64, json, os, subprocess, time, zlib


firmware_prefix = "gimbal_firmware_"
firmware_extension = "ax"

# Parse commandline arguments
parser = argparse.ArgumentParser(description="Firmware generator for the Axon Gimbal.")
parser.add_argument("--board_revision", required=True, action="store", help="set the board revision required")
parser.add_argument("--release", help="name of the release")
parser.add_argument("--image", required=True, action="store", help="the firmware image")
parser.add_argument("--outdir", help="image output directory")
args = parser.parse_args()

# Object to hold the packged info
desc = {}

# Empty release name
desc['release'] = ""
if args.release != None:
	desc['release']	 = str(args.release)

# Get the current git info
cmd = " ".join(["git", "describe", "--tags", "--dirty"])
p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).stdout
desc['git_identity'] = str(p.read().strip())
p.close()

# Version is extracted from the git identity
desc['version'] = str(desc['git_identity'].split('-')[0][2:])

# Build time is the current time
desc['build_time'] = int(time.time())

# Convert the image to be JSON safe
with open(args.image, "rb") as f:
	data = f.read()
	desc['image_size'] = len(data)
	desc['image'] = base64.b64encode(zlib.compress(data, 9)).decode('utf-8')

# Write the output
outputfile = os.path.join(args.outdir, "%s%s.%s" % (firmware_prefix, desc['version'], firmware_extension))
with open(outputfile, 'w') as f:
	f.write(json.dumps(desc, indent=4))

print('\n\n\nFirmware released as '+ str(outputfile))
