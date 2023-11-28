#!/bin/bash

git describe --long --tags --dirty | /bin/sed -e 's/.*/#define GitVersionString "&"/' > version.tmp
git branch | grep '*' | /bin/sed -e 's/\*\ \(.*\)/#define GitBranch "\1"/' >> version.tmp
git describe --long --tags | /bin/sed -e 's/\(v[0-9\w]*\).*/#define GitTag "\1"/' >> version.tmp
git describe --long --tags | /bin/sed -e 's/\(v[0-9\w]*\).*/#define GitCommit "\1"/' >> version.tmp
git describe --long --tags | /bin/sed -e 's/\(v[0-9\w]*\).*/#define GitCommitHash "\1"/' >> version.tmp
git describe --long --tags | /bin/sed -e 's/\(v[0-9\w]*\).*/#define GitCommitDirty "\1"/' >> version.tmp
git describe --long --tags | /bin/sed -e 's/\(v[0-9\w]*\).*/#define GitVersionMajor "\1"/' >> version.tmp
git describe --long --tags | /bin/sed -e 's/\(v[0-9\w]*\).*/#define GitVersionMinor "\1"/' >> version.tmp
git describe --long --tags | /bin/sed -e 's/\(v[0-9\w]*\).*/#define GitVersionRevision "\1"/' >> version.tmp

cmp -s "../3DRGimbalFactoryApplication/version.h" version.tmp && rm -f version.tmp || mv -f version.tmp "../3DRGimbalFactoryApplication/version.h"

