#!/bin/bash
set -e

for b in build/*; do
    echo
    echo "========="
    echo "Rendering $b..."
    echo "========="
    time "./$b"
done
echo
