#!/bin/bash
set -e

SOURCE=$1
PREFIX=$2
INSTALL_DIR=$PREFIX/server

echo "Installing LiveCode server"
echo "   Source: " $SOURCE
echo "   Prefix: " $PREFIX

invoke () {
    if [ -n $V ]; then
        echo "$@"
    fi
    "$@"
}

FILES=$(find $SOURCE -not -type d)
for FILE in $FILES; do
    case $FILE in
        $SOURCE/server-community)
            invoke install -D $FILE $INSTALL_DIR/livecode-server
            ;;

        $SOURCE/server-db*.so)
            # DB drivers
            invoke install -D $FILE $INSTALL_DIR/drivers/${FILE#$SOURCE/server-}
            ;;

        $SOURCE/server-*.so)
            # Externals
            invoke install -D $FILE $INSTALL_DIR/externals/${FILE#$SOURCE/server-}
            ;;

        *.dbg)
            # debug symbols
            # don't copy
            ;;
        
        *)
            if [ -x $FILE ]; then
                # Other execuatables
                invoke install -D $FILE $INSTALL_DIR/${FILE#$SOURCE/}
            else
                # non-executables
                invoke install -m 0644 -D $FILE $INSTALL_DIR/${FILE#$SOURCE/}
            fi
            ;;
    esac
done
