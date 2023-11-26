#!/bin/bash

git describe --long --tags --dirty | /bin/sed -e 's/.*/#define GitVersionString "&"/' > version.tmp
git branch | grep '*' | /bin/sed -e 's/\*\ \(.*\)/#define GitBranch "\1"/' >> version.tmp
git describe --long --tags | /bin/sed -e 's/\(v[0-9]*\.[0-9]*\.[0-9]*\).*/#define GitTag "\1"/' >> version.tmp
git describe --long --tags | /bin/sed -e 's/v[0-9]*\.[0-9]*\.[0-9]*-\([0-9]*\)-.*/#define GitCommit "\1"/' >> version.tmp
git describe --long --tags | /bin/sed -e 's/v[0-9]*\.[0-9]*\.[0-9]*-[0-9]*-\([a-z0-9]*\)/#define GitCommitHash "\1"/' >> version.tmp
git describe --long --tags --dirty | /bin/sed -e 's/v[0-9]*\.[0-9]*\.[0-9]*-[0-9]*-[a-z0-9]*\(-.*\)*/#define GitCommitDirty "\1"/' >> version.tmp
git describe --long --tags | /bin/sed -e 's/v\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/#define GitVersionMajor "\1"\n#define GitVersionMinor "\2"\n#define GitVersionRevision "\3"/' >> version.tmp
cmp -s "../3DRGimbalFactoryApplication/version.h" version.tmp && rm -f version.tmp || mv -f version.tmp "../3DRGimbalFactoryApplication/version.h"
