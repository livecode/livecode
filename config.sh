#!/bin/sh
set -e
base=$(dirname "$0")
exec python "${base}/config.py" "$@"
