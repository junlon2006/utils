#!/bin/bash
RK3308=/opt/rk3308/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu
RK3308_BIN=${RK3308}/bin
RK3308_INC=${RK3308}/include
RK3308_LIB=${RK3308}/lib
export STAGING_DIR=${RK3308_BIN}:${STAGING_DIR}

CROSS_COMPILE="${RK3308_BIN}/aarch64-linux-gnu-"

function build_one
{
    ./configure \
        --prefix=$PREFIX \
        --enable-cross-compile \
        --enable-nonfree \
        --enable-shared \
        --disable-static \
        --disable-debug \
        --disable-doc \
        --disable-programs \
        --disable-parsers \
        --disable-filters \
        --disable-avfilter \
        --disable-swscale \
        --disable-avdevice \
        --disable-bsfs \
        --disable-encoders \
        --disable-muxers \
        --disable-outdevs \
        --disable-devices \
        --cross-prefix=$CROSS_COMPILE \
        --target-os=linux \
        --arch=aarch64
}
CPU=rk3308
PREFIX=$(pwd)/$CPU
build_one && make clean && make -j8 && make install
