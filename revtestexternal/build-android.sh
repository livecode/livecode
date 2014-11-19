export NAME=revtestexternal
WORKSPACE=`cd ..; pwd`
PROJECT=$WORKSPACE/$NAME

export MODE=debug
export MAC_MODE=Debug
export DEBUGGABLE_FLAG=false
export DEBUG_FLAG=0
export ANDROID_NDK=$WORKSPACE/sdks/android-ndk
export ANDROID_SDK=$WORKSPACE/sdks/android-sdk

SRCROOT=$PROJECT
JAVA_SDK=`/usr/libexec/java_home`
JCOUNT=4

ENGINE_CLASSPATH=$PROJECT/../_build/android/$MODE/LiveCode-Community.jar

CLASSPATH="$ENGINE_CLASSPATH:$ANDROID_SDK/platforms/android-8/android.jar"
NDKBUILD=$ANDROID_NDK/ndk-build
DSTROOT=$SRCROOT/_build/android/$MODE/$NAME

JAVAC=$JAVA_SDK/bin/javac
JAR=$JAVA_SDK/bin/jar

echo "Building LCIDL for mac..."
sdks/Xcode_5_1/usr/bin/xcodebuild -project lcidlc/lcidlc.xcodeproj -configuration $MAC_MODE

echo "Building external stubs…"
mkdir -p "$PROJECT/derived_src"
echo "$PROJECT/../_build/mac/$MAC_MODE/lcidlc"
"$PROJECT/../_build/mac/$MAC_MODE/lcidlc" "$PROJECT/$NAME.lcidl" "$PROJECT/derived_src/"

echo "Building native code components…"
export NDK_PROJECT_PATH=$DSTROOT
$NDKBUILD NDK_DEBUG=$DEBUG_FLAG NDK_APP_DEBUGGABLE=$DEBUGGABLE_FLAG NDK_APPLICATION_MK=$SRCROOT/Application.mk -j $JCOUNT -s
if [ $? != 0 ]; then
	exit $?
fi

echo "Building java classes components… $PROJECT/../_build/android/$MODE/LiveCode-Community.jar"
mkdir -p "$DSTROOT/classes"
"$JAVAC" -d "$DSTROOT/classes" -cp "$CLASSPATH" -sourcepath "$SRCROOT/src" "$SRCROOT/src/$NAME.java" "$PROJECT/derived_src/LC.java"
if [ $? != 0 ]; then
	exit $?
fi

echo "Building jar…"
mkdir -p "$DSTROOT/$NAME/Android"
"$JAR" cf "$DSTROOT/$NAME/Android/Classes" -C "$DSTROOT/classes" .
if [ $? != 0 ]; then
	exit $?
fi

echo "Building lcext archive…"
mkdir -p $DSTROOT/$NAME/Android
cp $DSTROOT/../libs/armeabi/lib$NAME.so $DSTROOT/$NAME/Android/External-armeabi

cd $DSTROOT/$NAME
rm ../$NAME.lcext
zip -rD ../$NAME.lcext Android
