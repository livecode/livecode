#!/bin/bash

if [ $# -ne 2 ] ; then
    exit 1
fi

volume="$1"
background=$(cd $(dirname "$2") && pwd)/$(basename "$2")

script=$(dirname ${BASH_SOURCE[0]})/make-dmg-pretty.applescript

osascript "${script}" "${volume}" "${background}"
