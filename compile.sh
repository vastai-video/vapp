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
EOF
	exit 0
}

enter_script_dir() {
	cd $(dirname ${BASH_SOURCE[0]})

	D_FLAG="release"
	PREFIX=
	E_FLAG=1

	local OPTIND
	while getopts "hdle" opt
	do
		case $opt in
		h)	usage ;;
		d)	D_FLAG="debug" ;;
		l)	PREFIX=${PWD} ;;
		e)	E_FLAG=0 ;;
		esac
	done

	echo "D_FLAG = ${D_FLAG}"
	echo "PREFIX = ${PREFIX}"
    echo "E_FLAG = ${E_FLAG}"
    echo "thread = $(nproc)"

	export CC=gcc
	export PTHREAD_LIB=-lpthread

	export VASTAI_STREAM_INSTALL=${PREFIX}/opt/vastai/vaststream
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

	cd ./src
	make clean;make ${D_FLAG} -j$(nproc)
	make install

	cd ../test
	make clean;make ${D_FLAG} -j$(nproc)
	make install
	cd ..

	pwd
	mkdir -p ${VASTAI_STREAM_INSTALL}/lib/
	if [ "${D_FLAG}" = "release" ] ; then
		strip ./lib/libvappi.so
	fi
	cp ./lib/libvappi.so ${VASTAI_STREAM_INSTALL}/lib

	# copy elf files to install path
	if [ "${E_FLAG}" -eq 1 ] ; then
		mkdir -p ${VASTAI_STREAM_INSTALL}/lib/op/ext_op/video/
		cp ./elf/* ${VASTAI_STREAM_INSTALL}/lib/op/ext_op/video/
	fi
}

[[ "${BASH_SOURCE[0]}" == "${0}" ]] && compile "$@"