#!/bin/bash

# This script exists to work around a Gyp bug that prevents passing the
# '-source' and '-target' options with the same version number - the number
# is interpreted as a filename and is unique-ified on the command line...

javac=$1
version=$2
shift 2

"$javac" -source $version -target $version $*

