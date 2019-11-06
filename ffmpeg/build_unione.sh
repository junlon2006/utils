#!/bin/bash
UNIONE=/opt/unione/arm-linux-hf-4.9
UNIONE_BIN=${UNIONE}/bin
UNIONE_INC=${UNIONE}/include
UNIONE_LIB=${UNIONE}/lib
export STAGING_DIR=${UNIONE_BIN}:${STAGING_DIR}
CROSS_COMPILE="${UNIONE_BIN}/arm-linux-"
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
        --enable-openssl \
        --cross-prefix=$CROSS_COMPILE \
        --target-os=linux \
        --arch=arm
#       --disable-decoder=aac \
#       --disable-decoder=aac_fixed \
#       --enable-decoder=libfdk_aac \
#       --enable-libfdk-aac \
#        --sysinclude=$UNIONE_INC \
#        --extra-cflags="-Os -marm" \
#        --extra-ldflags="$ADDI_LDFLAGS"
}
CPU=unione
PREFIX=$(pwd)/$CPU
ADDI_CFLAGS="-marm"
build_one && make clean && make -j8 && make install
