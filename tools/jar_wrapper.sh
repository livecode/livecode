#!/bin/bash

# This script exists to work around a Gyp bug that prevents two folders
# being jarred together when they contain the same class prefixes

jar=$1
output=$2
first=$3
second=$4

"$jar" cf "$output" -C "$first" .
"$jar" uf "$output" -C "$second" .
