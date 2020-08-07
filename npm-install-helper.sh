#!/bin/bash -xe

test -d src/openslp/openslp || git submodule update --init --recursive --remote
test -f src/openslp/openslp/config.h || (cd src/openslp/openslp && ./autogen.sh && ./configure)
node-gyp rebuild
