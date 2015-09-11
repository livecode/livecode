#!/bin/bash

# Settings letting users build iOS engines without having MacOSX 10.11 installed
OS_VERSIONS="8.2 8.4"
SIMULATOR_VERSIONS="5.1 6.1 7.1 8.2 8.4"


# clang++ --version returns something like
#    Apple LLVM version 7.0.0 (clang-700.0.72) \
#    Target: x86_64-apple-darwin15.0.0
#    Thread model: posix
CLANG_MAJOR_VERSION=$(clang++ --version | head -n1 | cut -d" " -f4 | cut -d"." -f1)

# If Clang major version is 7 or above, then we add iOS 9.0 in the list.
if [ ${CLANG_MAJOR_VERSION} -ge 7 ]; then
	OS_VERSIONS="$OS_VERSIONS 9.0"
	SIMULATOR_VERSIONS="$SIMULATOR_VERSIONS 9.0"
fi

# Return the version list we are asked for.
if [ "$1" = "os" ]; then
	echo "$OS_VERSIONS"
else
	echo "$SIMULATOR_VERSIONS"
fi

exit 0
