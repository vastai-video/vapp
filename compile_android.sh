#!/bin/bash


set -x


usage() {
	cat <<EOF
Usage: ./compile.sh [options]

Options:
	-h	print this message
	-d	debug mode
	-l	local install
	-e	no copy elf files
    -p  platform: arm32/arm64
EOF
	exit 0
}

enter_script_dir() {
	cd $(dirname ${BASH_SOURCE[0]})

	D_FLAG="release"
	PREFIX=
	E_FLAG=1
    PLATFORM="arm64"
    API=24

	local OPTIND
	while getopts "hdlep:" opt
	do
		case $opt in
		h)	usage ;;
		d)	D_FLAG="debug" ;;
		l)	PREFIX=${PWD} ;;
		e)	E_FLAG=0 ;;
        p)  PLATFORM="$OPTARG" ;;
		esac
	done

	echo "D_FLAG = ${D_FLAG}"
	echo "PREFIX = ${PREFIX}"
    echo "E_FLAG = ${E_FLAG}"
    echo "PLATFORM = ${PLATFORM}"
    echo "thread = $(nproc)"

	export PTHREAD_LIB=
    export ANDROID="-DANDROID"
    export NDK=/opt/android-ndk-r21e
    export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/linux-x86_64

	export VASTAI_STREAM_INSTALL=${PREFIX}/vendor
}

#armv8-a aarch64
function build_arm64() {
	export ABI=arm64-v8a
	export ARCH=arm64
	export CPU=armv8-a
	export TARGET=aarch64-linux-android
	export CC=$TOOLCHAIN/bin/$TARGET$API-clang
	export CXX=$TOOLCHAIN/bin/$TARGET$API-clang++
    export CROSS_PREFIX=$TOOLCHAIN/bin/$TARGET-
	export AR=$TOOLCHAIN/bin/llvm-ar
	export NM=$TOOLCHAIN/bin/llvm-nm
	export AS=$TOOLCHAIN/bin/llvm-as
	export LD=$TOOLCHAIN/bin/$TARGET-ld
	export RANLIB=$TOOLCHAIN/bin/llvm-ranlib
	export STRIP=$TOOLCHAIN/bin/llvm-strip

	echo "* build for $ABI start !!!"
	echo "*************************"
}

#armeabi-v7a
function build_arm() {
	export ABI=armeabi-v7a
	export ARCH=arm
	export CPU=armv7-a
	export TARGET=armv7a-linux-androideabi
	export CC=$TOOLCHAIN/bin/$TARGET$API-clang
	export CXX=$TOOLCHAIN/bin/$TARGET$API-clang++
	export CROSS_PREFIX=$TOOLCHAIN/bin/arm-linux-androideabi
	export AR=$TOOLCHAIN/bin/llvm-ar
	export NM=$TOOLCHAIN/bin/llvm-nm
	export AS=$TOOLCHAIN/bin/llvm-as
	export LD=$CROSS_PREFIX-ld
	export RANLIB=$TOOLCHAIN/bin/llvm-ranlib
	export STRIP=$TOOLCHAIN/bin/llvm-strip

	echo "* build for $ABI start !!!"
	echo "*************************"
}

compile() {
	enter_script_dir "$@"

	if [ -d lib ] ; then
		rm -rf lib/
	fi

	if [ -d bin ] ; then
		rm -rf bin/
	fi

    mkdir -p lib
    mkdir -p bin

    if [ "$PLATFORM" = "arm64" ];then
        echo "start build ABI=arm64-v8a CPU=armv8-a"
        build_arm64
    # unsupport arm32
    # elif [ "$PLATFORM" = "arm32" ];then
    #     echo "start build ABI=armeabi-v7a CPU=armv7-a"
    #     build_arm
    fi

	cd ./src
	make clean;make ${D_FLAG} -j$(nproc)
	make install

	cd ../test
	make clean;make ${D_FLAG} -j$(nproc)
	make install
	cd ..

	pwd
	mkdir -p ${VASTAI_STREAM_INSTALL}/lib64
	if [ "${D_FLAG}" = "release" ] ; then
		$STRIP ./lib/libvappi.so
	fi
	cp ./lib/libvappi.so ${VASTAI_STREAM_INSTALL}/lib64

	# copy files to install path
	if [ "${E_FLAG}" -eq 1 ] ; then
		mkdir -p ${VASTAI_STREAM_INSTALL}/etc/op
		cp ./elf_sg100/* ${VASTAI_STREAM_INSTALL}/etc/op/
	fi
}

[[ "${BASH_SOURCE[0]}" == "${0}" ]] && compile "$@"