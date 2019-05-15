#!/bin/bash

# Turns a series of arguments into a path string

for item in $*; do
  path="${path:+${path}:}$(echo "$item" | tr -d '"')"
done

echo $path

