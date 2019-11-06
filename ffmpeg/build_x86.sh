#!/bin/bash

function build_one
{
    ./configure \
        --prefix=$PREFIX \
        --enable-shared \
        --disable-static \
        --disable-doc \
        --disable-parsers \
        --disable-filters \
        --disable-avfilter \
        --disable-swscale \
        --disable-avdevice \
        --disable-bsfs \
        --disable-encoders \
        --disable-debug \
        --enable-small
}

CPU=x86
PREFIX=$(pwd)/$CPU
#ADDI_CFLAGS="-marm"
build_one && make clean && make -j8 && make install
