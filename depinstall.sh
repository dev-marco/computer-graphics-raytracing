#!/bin/bash

(
    sudo add-apt-repository ppa:ubuntu-toolchain-r/test
    sudo apt-get update
    sudo apt-get install libopencv-dev g++-5 -y
)
