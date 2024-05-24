#!/bin/bash
if [ ! -d libDaisy ]; then
    git clone --recurse-submodules -j8 https://github.com/electro-smith/libDaisy
fi

(cd libDaisy; git pull origin master; make -s clean; make -j -s)
