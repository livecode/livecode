#/bin/bash

./config.sh --cross --platform linux-armv6hf --cc-prefix ${HOME}/x-tools/arm-rpi-linux-gnueabihf/bin/arm-rpi-linux-gnueabihf- --sysroot ${HOME}/x-tools/arm-rpi-linux-gnueabihf/arm-rpi-linux-gnueabihf/sysroot --aux-sysroot /var/lib/schroot/chroots/jessie-armhf --triple arm-linux-gnueabihf
