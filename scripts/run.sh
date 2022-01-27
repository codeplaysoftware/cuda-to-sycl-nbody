#!/bin/bash

# Copyright (C) 2022 Codeplay Software Limited
# This work is licensed under the terms of the MIT license.
# For a copy, see https://opensource.org/licenses/MIT.

# Run the simulation with args:
# 50*256 particles
# 4 simulation steps per render frame
# 0.99 velocity damping factor
./nbodygl 50 4 0.99
