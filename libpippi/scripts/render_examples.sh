#!/bin/bash

for b in build/*; do
    echo
    echo "========="
    echo "Rendering $b..."
    echo "========="
    (time ./$b) |& sed 's/^/  /'
done
echo
