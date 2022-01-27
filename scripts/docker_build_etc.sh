# Copyright (C) 2022 Codeplay Software Limited
# This work is licensed under the terms of the MIT license.
# For a copy, see https://opensource.org/licenses/MIT.

# non-functional code! This is a sketch of how to do the dpct conversion properly in a container
#
# Should be run with something like:
#
# docker run --rm \
#     -v /opt/intel/oneapi/:/opt/intel/oneapi/ \
#     -v $PWD:$PWD \
#     -u $UID \
#     -i joeatodd/onednn-cuda \
#     bash < scripts/docker_build_etc.sh


# Navigate to relevant directory

cd $SRC_DIR

# Call cmake on it
bash scripts/build_cuda.sh

# Call "intercept-build make" in build dir
cd build
make clean
intercept-build make

# Do conversion w/ -p
