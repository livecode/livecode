#!/bin/bash

MODE=debug
#MODE=release

SOLUTION_DIR=`dirname $0`
ARCH=x86_64
BUILD_DIR=$SOLUTION_DIR/_build/linux/$ARCH/$MODE
MODULE_DIR=$BUILD_DIR/modules
PRODUCT_DIR=$BUILD_DIR

function build_widget {
    WIDGET_DIR=$1
    WIDGET_NAME=$(basename $1)
    TARGET_DIR="com.livecode.extensions.livecode.${WIDGET_NAME}"

    echo $WIDGET_DIR
    echo $WIDGET_NAME
    echo $TARGET_DIR

    $BUILD_DIR/lc-compile \
            --modulepath $MODULE_DIR \
            --manifest $WIDGET_DIR/manifest.xml \
            --output $WIDGET_DIR/module.lcm \
            $WIDGET_DIR/$WIDGET_NAME.lcb

    pushd $WIDGET_DIR
    zip -r "${TARGET_DIR}.lce" *
    popd

    mkdir -p "${BUILD_DIR}/packaged_extensions/${TARGET_DIR}"

    # Unzip to the packaged extensions folder
    unzip \
        -o \
        "${WIDGET_DIR}/${TARGET_DIR}.lce" \
        -d "${BUILD_DIR}/packaged_extensions/${TARGET_DIR}"

    rm $WIDGET_DIR/$TARGET_DIR.lce

    return 0
}


for WIDGET in extensions/libraries/* extensions/widgets/*; do
    build_widget $WIDGET
done
