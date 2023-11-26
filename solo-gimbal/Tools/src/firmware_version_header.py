#!/usr/bin/env python2

"""
Utility for generating a version header file

"""

import argparse, subprocess


# Parse commandline arguments
parser = argparse.ArgumentParser(description="Utility for generating a version header file")
parser.add_argument("output", help="image output file")
args = parser.parse_args()

# Get the current git info
cmd = " ".join(["git", "describe", "--tags", "--dirty", "--long"])
p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).stdout
git_identity = '0.0.0-0-0'#str(p.read().strip())
p.close()

cmd = " ".join(["git", "branch", "--list"])
p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).stdout
git_branch = '0.0'#str(p.read().decode('utf-8').strip().split('*')[1].split('\n')[0].strip())
p.close()

#git_tag = git_identity.split('-')[0]
git_semver = ['0','0','0']#git_tag[1:].split('.')

git_identity = '0-0-0'
git_branch = '0'
git_tag = '0'

# Write the header file
with open(args.output, 'w') as f:
	f.write("#define GitVersionString \"%s\"\n" % git_identity)
	f.write("#define GitBranch \"%s\"\n" % git_branch)
	f.write("#define GitTag \"%s\"\n" % git_tag)
	f.write("#define GitCommit \"%s\"\n" % git_identity.split('-')[2])
	f.write("#define GitVersionMajor \"%s\"\n" % git_semver[0])
	f.write("#define GitVersionMinor \"%s\"\n" % git_semver[1])
	f.write("#define GitVersionRevision \"%s\"\n" % git_semver[2])
	f.write("#define GitVersionMajorInt %i\n" % int(git_semver[0]))
	f.write("#define GitVersionMinorInt %i\n" % int(git_semver[1]))
	f.write("#define GitVersionRevisionInt %i\n" % int(git_semver[2]))
