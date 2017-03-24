#!/usr/bin/env python
# Copyright (C) 2017 LiveCode Ltd.
#
# This file is part of LiveCode.
#
# LiveCode is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3 as published by the Free
# Software Foundation.
#
# LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with LiveCode.  If not see <http://www.gnu.org/licenses/>.

import sys
import platform
import re
import os

KNOWN_PLATFORMS = (
    'linux-x86', 'linux-x86_64', 'android-armv6',
    'mac', 'ios', 'win-x86', 'win-x86_64', 'emscripten', 'linux-armv7'
)

def usage(exit_status):
    print(
"""Use gyp to generate project files when compiling LiveCode.

Usage:
  config.py [--platform PLATFORM] [OPTION...] [GYP_OPTION ...]

Options:
  -p, --platform PLATFORM
                    Choose which platform to build for
  -h, --help        Print this message

gyp options:
  --generator-output DIR
                    Put generated build files under DIR
  --depth PATH      Set DEPTH gyp variable to a relative path to PATH
  -f, --format FORMATS
                    Output formats to generate
  -Gmsvs_version=WIN_MSVS_VERSION
                    Version of Microsoft Visual Studio to use
  -Gandroid_ndk_version=ANDROID_NDK_VERSION
                    Version of Android Native Development Kit to use
  -DOS=OS           Target operating system
  -Dtarget_arch=TARGET_ARCH
                    Target LiveCode to run on ARCH processors
  -Dtarget_sdk=XCODE_TARGET_SDK
                    Compile LiveCode using the specified SDK in Xcode
  -Dhost_sdk=XCODE_HOST_SDK
                    Compile build tools using the specified SDK in Xcode

All unrecognised options get passed directly to gyp.  If you don't specify
a required option, config.py will try to guess a suitable value.

The currently-supported PLATFORMs are:
""")
    for p in KNOWN_PLATFORMS:
        print("  " + p)
    sys.exit(exit_status)

def error(message):
    print('ERROR: ' + message)
    sys.exit(1)

def guess_platform():
    system = platform.system()
    arch = platform.machine()
    if system == 'Darwin':
        return 'mac'
    if system == 'Linux':
        if re.match('^(x|i.?)86$', arch) is not None:
            return 'linux-x86'
        else:
            return 'linux-' + arch
    if system == 'Windows':
        if arch == 'AMD64':
            return 'win-x86_64'
        else:
            return 'win-x86'

def exec_gyp(args):
    gyp_lib = os.path.join(os.path.dirname(sys.argv[0]), 'gyp', 'pylib')
    sys.path.insert(0, gyp_lib)
    import gyp
    print('gyp ' + ' '.join(args))
    sys.exit(gyp.main(args))

################################################################
# Parse command-line options
################################################################

def process_env_options(opts):
    vars = ('OS', 'PLATFORM', 'GENERATOR_OUTPUT', 'FORMATS', 'DEPTH',
        'WIN_MSVS_VERSION', 'XCODE_TARGET_SDK', 'XCODE_HOST_SDK',
        'TARGET_ARCH', 'PERL', 'ANDROID_NDK_VERSION', 'ANDROID_PLATFORM',
        'ANDROID_SDK', 'ANDROID_NDK', 'ANDROID_BUILD_TOOLS',
        'ANDROID_TOOLCHAIN', 'AR', 'CC', 'CXX', 'LINK', 'OBJCOPY', 'OBJDUMP',
        'STRIP', 'JAVA_SDK', 'NODE_JS', 'BUILD_EDITION',
        'MS_SPEECH_SDK4', 'MS_SPEECH_SDK5', 'QUICKTIME_SDK',
        )
    for v in vars:
        opts[v] = os.getenv(v)

    if opts['FORMATS'] is not None:
        opts['FORMATS'] = opts['FORMATS'].split()
    else:
        opts['FORMATS'] = []

def process_arg_options(opts, args):
    gyp_options = []
    offset = 0
    while offset < len(args):
        key = args[offset]
        if offset + 1 < len(args):
            value = args[offset + 1]
        else:
            value = None

        if key in ('-h', '--help'):
            usage(0)
        if key in ('-p', '--platform'):
            opts['PLATFORM'] = value
            offset += 2
            continue
        if key in ('--generator-output'):
            opts['GENERATOR_OUTPUT'] = value
            offset += 2
            continue
        if key in ('--depth'):
            opts['DEPTH'] = value
            offset += 2
            continue
        if key in ('-f', '--format'):
            opts['FORMATS'].insert(0, value)
            offset += 2
            continue

        # Intercept -D & -G options that config.py tries to generate
        intercepted_options = {
            '-Dhost_sdk': 'XCODE_HOST_SDK',
            '-Dtarget_sdk': 'XCODE_TARGET_SDK',
            '-Dtarget_arch': 'TARGET_ARCH',
            '-DOS': 'OS',
            '-Dperl': 'PERL',
            '-Dms_speech_sdk4': 'MS_SPEECH_SDK4',
            '-Dms_speech_sdk5': 'MS_SPEECH_SDK5',
            '-Dquicktime_sdk': 'QUICKTIME_SDK',
            '-Gmsvs_version': 'WIN_MSVS_VERSION',
            '-Gandroid_ndk_version': 'ANDROID_NDK_VERSION',
        }
        if key.startswith('-D') or key.startswith('-G'):
            prefix, suffix = key.split('=',2)
            if prefix in intercepted_options:
                opts[intercepted_options[prefix]] = suffix
            else:
                gyp_options.append(key)
            offset += 1
            continue

        # Unrecognised option
        error("Unrecognised option '{}'".format(key))

    opts['GYP_OPTIONS'] = gyp_options


################################################################
# Guess and validate platform and OS
################################################################

def validate_platform(opts):
    platform = opts['PLATFORM']
    if platform is None:
        platform = guess_platform()
    if platform is None:
        error("Cannot guess platform; specify '--platform <name>-'")

    if not platform in KNOWN_PLATFORMS:
        error("Unrecognised platform: '{}'".format(platform))

    opts['PLATFORM'] = platform

def validate_os(opts):
    validate_platform(opts)

    # Windows systems may have $OS set automatically in the
    # environment
    if opts['OS'] == 'Windows_NT':
        opts['OS'] = 'win'

    if opts['OS'] is None:
        opts['OS'] = opts['PLATFORM'].split('-')[0]

def guess_xcode_arch(target_sdk):
    sdk, ver = re.match('^([^\d]*)(\d*)', target_sdk).groups()
    if sdk == 'macosx':
        return 'i386'
    if sdk == 'iphoneos':
        if int(ver) < 8:
            return 'armv7'
        else:
            return 'armv7 arm64'
    if sdk == 'iphonesimulator':
        if int(ver) < 8:
            return 'i386'
        else:
            return 'i386 x86_64'

def validate_target_arch(opts):
    if opts['TARGET_ARCH'] is None:
        validate_platform(opts)

        platform = opts['PLATFORM']
        if platform == 'emscripten':
            opts['TARGET_ARCH'] = 'js'
            return

        platform_arch = re.search('-(x86|x86_64|armv6|armv7)$', platform)
        if platform_arch is not None:
            opts['TARGET_ARCH'] = platform_arch.group(1)
            return

        if re.match('^(ios|mac)', platform) is not None:
            validate_xcode_sdks(opts)
            arch = guess_xcode_arch(opts['XCODE_TARGET_SDK'])
            if arch is not None:
                opts['TARGET_ARCH'] = arch
                return

        error("Couldn't guess target architecture for '{}'".format(platform))

################################################################
# Guess other general options
################################################################

def validate_gyp_settings(opts):
    if opts['GENERATOR_OUTPUT'] is None:
        validate_platform(opts)

        opts['GENERATOR_OUTPUT'] = \
            os.path.join('build-' + opts['PLATFORM'], 'livecode')

    if len(opts['FORMATS']) < 1:
        validate_os(opts)

        build_os = opts['OS']
        if build_os in ('linux', 'android', 'emscripten'):
            format = 'make-linux'
        elif build_os in ('mac', 'ios'):
            format = 'xcode'
        elif build_os in ('win'):
            format = 'msvs'
        opts['FORMATS'] = [format,]

    if opts['DEPTH'] is None:
        opts['DEPTH'] = '.'

    if opts['BUILD_EDITION'] is None:
        opts['BUILD_EDITION'] = 'community'

def guess_java_home(os):
    try:
        return subprocess.check_output('/usr/libexec/java_home')
    except CalledProcessError as e:
        pass
    for d in ('/usr/lib/jvm/default', '/usr/lib/jvm/default-java'):
        if os.path.isdir(d):
            return d

def validate_java_tools(opts):
    if opts['JAVA_SDK'] is None:
        validate_os(opts)
        sdk = guess_java_home(os)
        if sdk is None:
            error('Java SDK not found; set $JAVA_SDK')
        opts['JAVA_SDK'] = sdk

################################################################
# Windows-specific options
################################################################

def get_program_files_x86():
    return os.environ.get('ProgramFiles(x86)',
                          os.environ.get('ProgramFiles',
                                         'C:\\Program Files\\'))

def guess_windows_perl():
    # Check the PATH first
    if (any(os.access(os.path.join(p, 'perl.exe'), os.X_OK)
            for p in os.environ['PATH'].split(os.pathsep))):
        return 'perl.exe'

    for p in ('C:\\perl64\\bin', 'C:\\perl\\bin'):
        perl = os.path.join(p, 'perl.exe')
        if os.access(perl, os.X_OK):
            return perl

    # If this is running on a non-Windows platform, default to "perl"
    if platform.system() != 'windows':
        return 'perl'

    error('Perl not found; set $PERL')

def guess_ms_speech_sdk4():
    d = os.path.join(get_program_files_x86(), 'Microsoft Speech SDK')
    if not os.path.isdir(d):
        return None
    return d

def guess_ms_speech_sdk5():
    d = os.path.join(get_program_files_x86(), 'Microsoft Speech SDK 5.1')
    if not os.path.isdir(d):
        return None
    return d

def guess_quicktime_sdk():
    d = os.path.join(get_program_files_x86(), 'QuickTime SDK')
    if not os.path.isdir(d):
        return None
    return d

def validate_windows_tools(opts):
    if opts['PERL'] is None:
        opts['PERL'] = guess_windows_perl()

    if opts['MS_SPEECH_SDK4'] is None:
        opts['MS_SPEECH_SDK4'] = guess_ms_speech_sdk4()

    if opts['MS_SPEECH_SDK5'] is None:
        opts['MS_SPEECH_SDK5'] = guess_ms_speech_sdk5()

    if opts['QUICKTIME_SDK'] is None:
        opts['QUICKTIME_SDK'] = guess_quicktime_sdk()

    if opts['WIN_MSVS_VERSION'] is None:
        opts['WIN_MSVS_VERSION'] = '2010'

################################################################
# Mac & iOS-specific options
################################################################

def validate_xcode_sdks(opts):
    if opts['XCODE_TARGET_SDK'] is None:
        validate_os(opts)
        if opts['OS'] == 'mac':
            opts['XCODE_TARGET_SDK'] = 'macosx10.8'
        elif opts['OS'] == 'ios':
            opts['XCODE_TARGET_SDK'] = 'iphoneos'

    if opts['XCODE_HOST_SDK'] is None:
        validate_os(opts)
        if opts['OS'] == 'mac':
            opts['XCODE_HOST_SDK'] = opts['XCODE_TARGET_SDK']
        elif opts['OS'] == 'ios':
            opts['XCODE_HOST_SDK'] = 'macosx'

################################################################
# Android-specific options
################################################################

# We suggest some symlinks for Android toolchain components in the
# INSTALL-android.md file.  This checks if a directory is present
def guess_android_tooldir(name):
    dir = os.path.join(os.path.expanduser('~'), 'android', 'toolchain', name)
    if os.path.isdir(dir):
        return dir
    return None

# Attempt to guess the Android build tools version by looking for directories
# in the SDK's build-tools subdirectory.  This is pretty fragile if someone
# has (potentially?) multiple sets of build tools installed.
def guess_android_build_tools(sdkdir):
    dirs = listdir(os.path.join(sdkdir, 'build-tools'))
    if len(dirs) == 1:
        return dirs[0]
    return None

def validate_android_tools(opts):
    if opts['ANDROID_NDK_VERSION'] is None:
        opts['ANDROID_NDK_VERSION'] = 'r10d'

    ndk_ver = opts['ANDROID_NDK_VERSION']
    if opts['ANDROID_PLATFORM'] is None:
        opts['ANDROID_PLATFORM'] = 'android-17'

    if opts['ANDROID_NDK'] is None:
        ndk = guess_android_tooldir('android-ndk')
        if ndk is None:
            error('Android NDK not found; set $ANDROID_NDK')
        opts['ANDROID_NDK'] = ndk

    if opts['ANDROID_SDK'] is None:
        sdk = guess_android_tooldir('android-sdk')
        if sdk is None:
            error('Android SDK not found; set $ANDROID_SDK')
        opts['ANDROID_SDK'] = sdk

    if opts['ANDROID_BUILD_TOOLS'] is None:
        tools = guess_android_build_tools(opts['ANDROID_SDK'])
        if tools is None:
            error('Android build tools not found; set $ANDROID_BUILD_TOOLS')
        opts['ANDROID_BUILD_TOOLS'] = tools

    if opts['ANDROID_TOOLCHAIN'] is None:
        dir = guess_android_tooldir('standalone')
        if dir is None:
            error('Android toolchain not found; set $ANDROID_TOOLCHAIN')
        opts['ANDROID_TOOLCHAIN'] = os.path.join(dir,'bin','arm-linux-androideabi-')

    def android_tool(name, env, extra=""):
        if opts[env] is None:
            tool = opts['ANDROID_TOOLCHAIN'] + name
            if extra is not None:
                tool += ' ' + extra
            opts[env] = tool

    android_tool('ar', 'AR')
    android_tool('clang', 'CC',
                 '-target arm-linux-androideabi -march=armv6 -integrated-as')
    android_tool('clang++', 'CXX',
                 '-target arm-linux-androideabi -march=armv6 -integrated-as')
    android_tool('clang++', 'LINK',
                 '-target arm-linux-androideabi -march=armv6 -integrated-as -fuse-ld=bfd')
    android_tool('objcopy', 'OBJCOPY')
    android_tool('objdump', 'OBJDUMP')
    android_tool('strip', 'STRIP')

################################################################
# Emscripten-specific options
################################################################

def validate_emscripten_tools(opts):
    if opts['NODE_JS'] is None:
        opts['NODE_JS'] = 'node'

################################################################
# Linux-specific options
################################################################

################################################################
# Gyp invocation
################################################################

def core_gyp_args(opts):
    validate_gyp_settings(opts)
    validate_os(opts)

    args = []
    for f in opts['FORMATS']:
        args += ['--format', f]

    args += ['--depth', opts['DEPTH'],
             '--generator-output', opts['GENERATOR_OUTPUT'],
             '-DOS=' + opts['OS']]

    if opts['PERL'] is not None:
        args.append('-Dperl=' + opts['PERL'])

    if opts['BUILD_EDITION'] == 'commercial':
        args.append(os.path.join('..', 'livecode-commercial.gyp'))

    return args

def export_opts(opts, names):
    for n in names:
        print (n + '=' + opts[n])
        os.environ[n] = opts[n]

def gyp_define_args(opts, names):
    return ['-D{}={}'.format(key, opts[value])
            for key, value in names.iteritems()
            if opts[value] is not None]

def configure_linux(opts):
    validate_target_arch(opts)
    args = core_gyp_args(opts) + ['-Dtarget_arch=' + opts['TARGET_ARCH']]
    exec_gyp(args + opts['GYP_OPTIONS'])

def configure_emscripten(opts):
    validate_target_arch(opts)
    validate_emscripten_tools(opts)

    export_opts(opts, ('NODE_JS',))
    args = core_gyp_args(opts) + ['-Dtarget_arch=' + opts['TARGET_ARCH']]
    exec_gyp(args + opts['GYP_OPTIONS'])

def configure_android(opts):
    validate_target_arch(opts)
    validate_android_tools(opts)
    validate_java_tools(opts)

    export_opts(opts, ('ANDROID_BUILD_TOOLS', 'ANDROID_NDK',
                       'ANDROID_PLATFORM', 'ANDROID_SDK',
                       'JAVA_SDK', 'AR', 'CC', 'CXX', 'LINK', 'OBJCOPY',
                       'OBJDUMP', 'STRIP'))
    args = core_gyp_args(opts) + ['-Dtarget_arch=' + opts['TARGET_ARCH'],
                                  '-Dcross_compile=1',
                                  '-Gandroid_ndk_version=' + opts['ANDROID_NDK_VERSION']]
    exec_gyp(args + opts['GYP_OPTIONS'])

def configure_win(opts):
    validate_windows_tools(opts)

    args = core_gyp_args(opts) + ['-Gmsvs_version=' + opts['WIN_MSVS_VERSION']]
    args += gyp_define_args(opts, {'ms_speech_sdk4': 'MS_SPEECH_SDK4',
                                   'ms_speech_sdk5': 'MS_SPEECH_SDK5',
                                   'quicktime_sdk':  'QUICKTIME_SDK', })

    if platform.system() != 'Windows':
        args.append('-Dunix_configure=1')

    exec_gyp(args + opts['GYP_OPTIONS'])

def configure_mac(opts):
    validate_target_arch(opts)
    validate_xcode_sdks(opts)

    args = core_gyp_args(opts) + ['-Dtarget_sdk=' + opts['XCODE_TARGET_SDK'],
                                  '-Dhost_sdk=' + opts['XCODE_HOST_SDK'],
                                  '-Dtarget_arch=' + opts['TARGET_ARCH']]
    exec_gyp(args + opts['GYP_OPTIONS'])

def configure_ios(opts):
    configure_mac(opts)

def configure(args):
    opts = {}
    process_env_options(opts)
    process_arg_options(opts, args)

    validate_os(opts)
    configure_procs = {
        'linux': configure_linux,
        'emscripten': configure_emscripten,
        'android': configure_android,
        'win': configure_win,
        'mac': configure_mac,
        'ios': configure_ios,
    }
    configure_procs[opts['OS']](opts)

if __name__ == '__main__':
    configure(sys.argv[1:])
