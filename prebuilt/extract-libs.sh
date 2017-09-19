#!/bin/bash

# Extract all prebuilts archives
find ../ -maxdepth 1 -name '*-prebuilts.tar.*' -exec tar -xvf '{}' ';'
