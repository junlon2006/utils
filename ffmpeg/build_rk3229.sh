#!/bin/bash
UNIONE=/opt/unione/arm-none-linux-gnueabi-4.6.4_linux-3.3
UNIONE_BIN=${UNIONE}/bin
UNIONE_INC=${UNIONE}/include
UNIONE_LIB=${UNIONE}/lib
export STAGING_DIR=${UNIONE_BIN}:${STAGING_DIR}

CROSS_COMPILE="${UNIONE_BIN}/arm-none-linux-gnueabi-"
ADDI_LDFLAGS="-L${UNIONE_LIB} -L$(pwd)/aac/lib/"

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
        --disable-neon \
        --disable-vfp \
        --cross-prefix=$CROSS_COMPILE \
        --target-os=linux \
        --arch=arm
}
CPU=rk3308
PREFIX=$(pwd)/$CPU
ADDI_CFLAGS="-marm"
build_one && make clean && make -j8 && make install
