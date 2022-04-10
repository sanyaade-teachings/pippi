#!/bin/bash
if [ ! -d libDaisy ]; then
    git clone https://github.com/electro-smith/libDaisy
fi

(cd libDaisy; git pull origin master; make -s clean; make -j -s)
