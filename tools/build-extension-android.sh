# The folder containing this script
export TOOLS=`dirname $0`

export TRUNK=`cd $TOOLS/..; pwd`

# Global variables needed later
export MODE=release
export DEBUGGABLE_FLAG=false
export DEBUG_FLAG=0
export ANDROID_NDK=$TRUNK/sdks/android-ndk
export ANDROID_SDK=$TRUNK/sdks/android-sdk

# Local variables
export SRCROOT=$1
export JAVA_SDK=`/usr/libexec/java_home`
export JCOUNT=20
export EXTERNAL_NAME=$2

# Path variables
export CLASSPATH="$ANDROID_SDK/platforms/android-8/android.jar"
export NDKBUILD=$ANDROID_NDK/ndk-build
export DSTROOT=$SRCROOT/_build/android/$MODE
export RAWROOT=$SRCROOT/_build/android/$MODE/assets

export JAVAC=$JAVA_SDK/bin/javac
export JAR=$JAVA_SDK/bin/jar
export DX=$ANDROID_SDK/platforms/android-8/tools/dx
export AAPT=$ANDROID_SDK/platforms/android-8/tools/aapt
export APKBUILDER=$ANDROID_SDK/tools/apkbuilder
export ZIPALIGN=$ANDROID_SDK/tools/zipalign
export ADB=$ANDROID_SDK/platform-tools/adb
export AIDL=$ANDROID_SDK/platform-tools/aidl

##########

# Build the native code components
export NDK_PROJECT_PATH=$DSTROOT
echo "Building native code components..."
$NDKBUILD NDK_DEBUG=$DEBUG_FLAG NDK_APP_DEBUGGABLE=$DEBUGGABLE_FLAG NDK_APPLICATION_MK=$SRCROOT/Application.mk -j $JCOUNT -s
if [ $? != 0 ]; then
	exit $?
fi

mkdir -p "$SRCROOT/../binaries"
cp "$DSTROOT/libs/armeabi/lib$EXTERNAL_NAME.so" "$SRCROOT/../binaries"

mkdir -p "$SRCROOT/../binaries/Android"

cp "$DSTROOT/libs/armeabi/lib$EXTERNAL_NAME.so" "$SRCROOT/../binaries/Android/External-armeabi"
cd "$SRCROOT/../binaries"
zip -Rm "$EXTERNAL_NAME.lcext" "Android/*"
cd "$TOOLS"