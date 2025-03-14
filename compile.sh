#!/bin/bash


function usage() {
    cat <<EOF
Usage: $0  [--type [BUILD_TYPE]]  [--ut [UT]]  [-h|--help]

Options:
    --type          build type: rel/dbg
    --ut            build test: 0/1
    -h|--help       print help message
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --type)
            if [[ -n "$2" && "$2" != "-"* ]]; then
                type=$2
                shift 2
            else
                echo "Warning: --clean is null."
                type="rel"
                shift
            fi
            ;;
        --ut)
            if [[ -n "$2" && "$2" != "-"* ]]; then
                ut=$2
                shift 2
            else
                echo "Warning: --ut is null."
                ut=0
                shift
            fi
            ;;
        -h|--help)
            usage
            ;;
        *) # Unknown option
            echo "Error: Unknown option $1"
            usage
            ;;
    esac
done


if [ ! -d build ]; then
    mkdir build
fi
cd build

echo ">>>>> build_type = ${type}, ut = ${ut}, ndk_bit = ${ndk_bit}"

if [ "$(expr substr $(uname -s) 1 5)" == "Linux" ];then
    echo "platform: Linux"
    cmake -DBUILD_TYPE=${type} -DUT=${ut} -DBUILD_NDK=${ndk_bit} ../
    make install

else
    echo "platform: Windows"
    cmake -DBUILD_TYPE=${type} ../
    cmake --build . --config ${type}
fi
