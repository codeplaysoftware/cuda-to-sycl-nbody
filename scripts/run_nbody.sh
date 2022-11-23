#!/bin/bash

# Copyright (C) 2022 Codeplay Software Limited
# This work is licensed under the terms of the MIT license.
# For a copy, see https://opensource.org/licenses/MIT.

# This script runs a particular version of the nbody simulation
# depending on the -b flag. All subsequent positional args are
# passed on to nbody. See ../README.md for a description of these
# positional args.
#
# ./scripts/run_nbody.sh -b dpcpp 50 5 0.999 0.001 1.0e-3 2.0

while getopts b: flag
do
    case "${flag}" in
    b) backend=${OPTARG};;
    esac
done

shift 2;

case "$backend" in
    cuda) ./nbody_cuda "$@";;
    dpcpp) SYCL_DEVICE_FILTER=cuda ./nbody_dpcpp_no_gui "$@";;
    computecpp) COMPUTECPP_TARGET=host ./nbody_computecpp "$@";;
    *) echo "Bad backend"; exit 1;;
esac
