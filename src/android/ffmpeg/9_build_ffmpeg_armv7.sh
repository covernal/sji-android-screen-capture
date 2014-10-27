#!/bin/sh
OLD_DIR=$PWD
printenv ANDROID_NDK_ROOT > /dev/null || { echo please export ANDROID_NDK_ROOT=root_dir_of_your_android_ndk; exit 1; }
cd ./ffmpeg_src            || { echo please download ffmpeg source to [./ffmpeg_src];            exit 1; }

SYS_ROOT="$ANDROID_NDK_ROOT/platforms/android-8/arch-arm"
TOOL_CHAIN_DIR=`ls -d $ANDROID_NDK_ROOT/toolchains/arm-linux-androideabi-4.*/prebuilt/* | tail -n 1` || exit 1
LIBGCC_DIR=`ls -d $TOOL_CHAIN_DIR/lib/gcc/arm-linux-androideabi/4.* | tail -n 1` || exit 1
LIBEXEC_DIR=`ls -d $TOOL_CHAIN_DIR/libexec/gcc/arm-linux-androideabi/4.* | tail -n 1` || exit 1
CPP_ROOT=`ls -d $ANDROID_NDK_ROOT/sources/cxx-stl/gnu-libstdc++/4.* | tail -n 1` || exit 1
MAKE_DIR=`ls -d $ANDROID_NDK_ROOT/prebuilt/*/bin | tail -n 1` || exit 1
export  CFLAGS="-O3 --sysroot=$SYS_ROOT -I$SYS_ROOT/usr/include -I$LIBGCC_DIR/include -I$CPP_ROOT/include"
export LDFLAGS="-B$SYS_ROOT/usr/lib -B$LIBGCC_DIR -B$TOOL_CHAIN_DIR/arm-linux-androideabi/bin -B$LIBEXEC_DIR -B$CPP_ROOT/libs/armeabi"
export PATH="$TOOL_CHAIN_DIR/arm-linux-androideabi/bin:$LIBEXEC_DIR:$MAKE_DIR:$PATH"
export CC=gcc

echo ---------------patch some files--------------------
cp -fv $OLD_DIR/patch_ffmpeg_x11grab.c ./libavdevice/x11grab.c || exit 1
cp -fv $OLD_DIR/patch_ffmpeg_configure ./configure || exit 1
#cp -fv $OLD_DIR/patch_ffmpeg_ffmpeg.c ./ffmpeg.c || exit 1
echo ""; echo ok; echo ""

echo ---------------config ffmpeg [armv7]--------------------
export CPPFLAGS="--sysroot=$SYS_ROOT"   #ubuntu NDK need this flag when check assembler by gcc \$CPPFLAGS \$ASFLAGS xxx.S which in turn cause "GNU assembler not found, install gas-preprocessor" due to "No include path for stdc-predef.h"

./configure --arch=armv7 --cpu=armv7-a --target-os=linux --enable-cross-compile --enable-static --disable-doc \
	--disable-ffplay --disable-ffprobe --disable-ffserver \
	--disable-symver --disable-debug \
	--disable-everything \
	\
	--enable-protocol=pipe \
	\
	--enable-filter=scale --enable-filter=crop --enable-filter=transpose \
	\
	--enable-demuxer=rawvideo --enable-decoder=rawvideo \
	\
	--enable-muxer=image2 --enable-muxer=image2pipe --enable-muxer=mjpeg --enable-encoder=mjpeg --enable-encoder=png \
	\
	--enable-x11grab --enable-indev=x11grab \
	\
	|| exit 1

echo ---------------make ffmpeg [armv7]--------------------
make clean
make all || exit 1

cp -fv ./ffmpeg $OLD_DIR/../../../bin/android/ffmpeg.armv7 || exit 1

echo ""; echo ok; echo ""