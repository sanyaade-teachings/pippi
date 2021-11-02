#!/bin/bash
rm -rf DaisyExamples
git clone --recursive https://github.com/electro-smith/DaisyExamples
(cd DaisyExamples; bash ci/build_libs.sh)
