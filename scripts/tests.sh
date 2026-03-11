#!/bin/bash

set -e

pushd ./src/LB &> /dev/null
echo "Considering LB tests"
make test
popd &> /dev/null
