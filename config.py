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
import subprocess
import shutil

# The set of platforms for which this branch supports automated builds
BUILDBOT_PLATFORM_TRIPLES = (
    'x86-linux-debian8',
    'x86_64-linux-debian8',
    'armv7-android-ndk16r15',
    'arm64-android-ndk16r15',
    'x86-android-ndk16r15',
    'x86_64-android-ndk16r15',
    'universal-mac-macosx10.9', # Minimum deployment target
    'universal-ios-iphoneos13.5',
    'universal-ios-iphoneos13.2',
    'universal-ios-iphoneos12.1',
    'universal-ios-iphoneos11.2',
    'universal-ios-iphoneos10.2',
    'universal-ios-iphonesimulator13.5',
    'universal-ios-iphonesimulator13.2',
    'universal-ios-iphonesimulator12.1',
    'universal-ios-iphonesimulator11.2',
    'universal-ios-iphonesimulator10.2',
    'x86-win32', # TODO[2017-03-23] More specific ABI
    'x86_64-win32',
    'js-emscripten-sdk1.35',
)

KNOWN_PLATFORMS = (
    'linux-x86', 'linux-x86_64', 'linux-armv6hf', 'linux-armv7',
    'android-armv6', 'android-armv7', 'android-arm64', 'android-x86', 'android-x86_64',
    'mac', 'ios', 
    'win-x86', 'win-x86_64', 
    'emscripten'
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
  --sysroot SYSROOT
                    Use the given folder for system headers and libraries (only
                    useful when cross-compiling).
  --aux-sysroot SYSROOT
                    Like --sysroot but --sysroot's headers and libraries
                    come first. Passed to the compiler as -I/-L options
                    instead of a --sysroot option.
  --triple TRIPLE
                    Required if --aux-sysroot has a `multilib' setup where
                    some headers and libraries are under a subdirectory named
                    with the target triple. This is often the case for Debian/
                    Ubuntu derived distributions.
  --cc-prefix PREFIX
                    Compiler prefix path e.g.
                    ${HOME}/toolchain/bin/arm-linux-gnueabi-
  --cross
                    Indicates cross-compilation (you probably want to specify
                    --sysroot/--aux-sysroot and --cc-prefix too).
  --use-lto
                    [EXPERIMENTAL] Use link-time optimisation when building.

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
        'TARGET_ARCH', 'PERL', 'ANDROID_NDK_VERSION', 'ANDROID_LIB_PATH',
        'ANDROID_NDK_PLATFORM_VERSION', 'ANDROID_PLATFORM',
        'ANDROID_SDK', 'ANDROID_NDK', 'ANDROID_BUILD_TOOLS', 'LTO',
        'ANDROID_TOOLCHAIN_DIR', 'ANDROID_TOOLCHAIN', 'ANDROID_API_VERSION',
        'AR', 'CC', 'CXX', 'LINK', 'OBJCOPY', 'OBJDUMP',
        'STRIP', 'JAVA_SDK', 'NODE_JS', 'BUILD_EDITION', 'CC_PREFIX', 'CROSS',
        'SYSROOT', 'AUX_SYSROOT', 'TRIPLE', 'MS_SPEECH_SDK5', 'QUICKTIME_SDK',
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
        if key in ('--use-lto'):
             opts['LTO'] = True
             offset += 1
             continue
        if key in ('--cross'):
             opts['CROSS'] = True
             offset += 1
             continue
        if key in ('--sysroot'):
            opts['SYSROOT'] = value
            offset += 2
            continue
        if key in ('--aux-sysroot'):
            opts['AUX_SYSROOT'] = value
            offset += 2
            continue
        if key in ('--triple'):
            opts['TRIPLE'] = value
            offset += 2
            continue
        if key in ('--cc-prefix'):
            opts['CC_PREFIX'] = value
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

def host_platform(opts):
    opts['HOST_PLATFORM'] = guess_platform()

def guess_xcode_arch(target_sdk):
    sdk, ver = re.match('^([^\d]*)(\d*)', target_sdk).groups()
    if sdk == 'macosx':
        return 'i386 x86_64'
    if sdk == 'iphoneos':
        if int(ver) < 8:
            return 'armv7'
        else:
            return 'armv7 arm64'
    if sdk == 'iphonesimulator':
        return 'x86_64'

def validate_target_arch(opts):
    if opts['TARGET_ARCH'] is None:
        validate_platform(opts)

        platform = opts['PLATFORM']
        if platform == 'emscripten':
            opts['TARGET_ARCH'] = 'js'
            opts['UNIFORM_ARCH'] = opts['TARGET_ARCH']
            return

        platform_arch = re.search('-(x86|x86_64|arm(64|v(6(hf)?|7)))$', platform)
        if platform_arch is not None:
            opts['TARGET_ARCH'] = platform_arch.group(1)
            opts['UNIFORM_ARCH'] = opts['TARGET_ARCH']
            return

        if re.match('^(ios|mac)', platform) is not None:
            validate_xcode_sdks(opts)
            arch = guess_xcode_arch(opts['XCODE_TARGET_SDK'])
            if arch is not None:
                opts['TARGET_ARCH'] = arch
                opts['UNIFORM_ARCH'] = opts['TARGET_ARCH']
                return

        error("Couldn't guess target architecture for '{}'".format(platform))
    else:
        opts['UNIFORM_ARCH'] = opts['TARGET_ARCH']

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

def guess_java_home(platform):
    if platform.startswith('linux'):
        try:
            javac_str = '/bin/javac'
            javac_path = subprocess.check_output(['/usr/bin/env',
                         'readlink', '-f', '/usr' + javac_str]).strip()
            if (os.path.isfile(javac_path) and
                javac_path.endswith(javac_str)):
                return javac_path[:-len(javac_str)]
        except subprocess.CalledProcessError as e:
            print(e)
            pass # Fall through to other ways of guessing

    # More guesses
    try:
        if os.path.isfile('/usr/libexec/java_home'):
            return subprocess.check_output('/usr/libexec/java_home').strip()
    except subprocess.CalledProcessError as e:
        print(e)
        pass
    for d in ('/usr/lib/jvm/default', '/usr/lib/jvm/default-java'):
        if os.path.isdir(d):
            return d

def validate_java_tools(opts):
    if opts['JAVA_SDK'] is None:
        validate_platform(opts)
        sdk = guess_java_home(opts['HOST_PLATFORM'])
        if sdk is None:
            error('Java SDK not found; set $JAVA_SDK')
        opts['JAVA_SDK'] = sdk

def configure_toolchain(opts):
    ccprefix = ''
    if opts['CC_PREFIX'] is not None:
        ccprefix = opts['CC_PREFIX']
    if opts['CC'] is None:
        opts['CC'] = ccprefix + 'cc'
    if opts['CXX'] is None:
        opts['CXX'] = ccprefix + 'c++'
    if opts['AR'] is None:
        opts['AR'] = ccprefix + 'ar'
    if opts['LINK'] is None:
        opts['LINK'] = opts['CXX']
    if opts['STRIP'] is None:
        opts['STRIP'] = ccprefix + 'strip'
    if opts['OBJCOPY'] is None:
        opts['OBJCOPY'] = ccprefix + 'objcopy'
    if opts['OBJDUMP'] is None:
        opts['OBJDUMP'] = ccprefix + 'objdump'

    # Enable LTO if requested
    if opts['LTO'] is True:
        for key in ('CC', 'CXX', 'LINK'):
            opts[key] += ' -flto -ffunction-sections -fdata-sections -fuse-linker-plugin -fuse-ld=gold'

    # If cross-compiling, link libgcc and libstdc++ statically.
    # This is done because the cross-compilers are likely to be different
    # versions to those available on the target system.
    if opts['CROSS'] is True:
        for key in ('CC', 'CXX', 'LINK'):
            opts[key] += ' -static-libgcc -static-libstdc++'

    # Append a --sysroot option for the compilers and linker
    if opts['SYSROOT'] is not None:
        for key in ('CC', 'CXX', 'LINK'):
            opts[key] += ' --sysroot="' + opts['SYSROOT'] + '"'
            opts[key] += ' -Wl,--sysroot,"' + opts['SYSROOT'] + '"'

    # Configure an auxiliary sysroot, if one is specified.
    #
    # The -L options beginning '-L=/' are required to ensure the original
    # sysroot gets searched before the auxiliary one. Similarly with
    # -idirafter; these come after the sysroot include directories.
    if opts['AUX_SYSROOT'] is not None:
        for key in ('CC', 'CXX', 'LINK'):
            opts[key] += ' -idirafter "' + opts['AUX_SYSROOT'] + '/usr/include"'
            opts[key] += ' -L=/lib'
            opts[key] += ' -L=/usr/lib'
            if opts['TRIPLE'] is not None:
                opts[key] += ' -L"=/lib/' + opts['TRIPLE'] + '"'
                opts[key] += ' -L"=/usr/lib/' + opts['TRIPLE'] + '"'
            opts[key] += ' -L"' + opts['AUX_SYSROOT'] + '/lib"'
            opts[key] += ' -L"' + opts['AUX_SYSROOT'] + '/usr/lib"'
            opts[key] += ' -Wl,-rpath-link,"' + opts['AUX_SYSROOT'] + '/lib"'
            opts[key] += ' -Wl,-rpath-link,"' + opts['AUX_SYSROOT'] + '/usr/lib"'
            if opts['TRIPLE'] is not None:
                opts[key] += ' -idirafter "' + opts['AUX_SYSROOT'] + '/usr/include/' + opts['TRIPLE'] + '"'
                opts[key] += ' -L"' + opts['AUX_SYSROOT'] + '/lib/' + opts['TRIPLE'] + '"'
                opts[key] += ' -L"' + opts['AUX_SYSROOT'] + '/usr/lib/' + opts['TRIPLE'] + '"'
                opts[key] += ' -Wl,-rpath-link,"' + opts['AUX_SYSROOT'] + '/lib/' + opts['TRIPLE'] + '"'
                opts[key] += ' -Wl,-rpath-link,"' + opts['AUX_SYSROOT'] + '/usr/lib/' + opts['TRIPLE'] + '"'

    # Add LD as an alias for LINK
    opts['LD'] = opts['LINK']

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

    if opts['MS_SPEECH_SDK5'] is None:
        opts['MS_SPEECH_SDK5'] = guess_ms_speech_sdk5()

    if opts['QUICKTIME_SDK'] is None:
        opts['QUICKTIME_SDK'] = guess_quicktime_sdk()

    if opts['WIN_MSVS_VERSION'] is None:
        # TODO [2017-04-11]: This should be 2017, but it is not 
        # compatible with our gyp as is.
        opts['WIN_MSVS_VERSION'] = '2015'

################################################################
# Mac & iOS-specific options
################################################################

def validate_xcode_sdks(opts):
    if opts['XCODE_TARGET_SDK'] is None:
        validate_os(opts)
        if opts['OS'] == 'mac':
            opts['XCODE_TARGET_SDK'] = 'macosx10.9'
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
def guess_android_tooldir(toolchain, name):
    if toolchain is None:
        dir = os.path.join(os.path.expanduser('~'), 'android', 'toolchain', name)
    else:
        dir = os.path.join(toolchain, name)
    if os.path.isdir(dir):
        return dir
    return None

# Attempt to guess the Android build tools version by looking for directories
# in the SDK's build-tools subdirectory.  This is pretty fragile if someone
# has (potentially?) multiple sets of build tools installed.
def guess_android_build_tools(sdkdir):
    dirs = os.listdir(os.path.join(sdkdir, 'build-tools'))
    if len(dirs) == 1:
        return dirs[0]
    return None

# Guess the standalone toolchain directory name.
def guess_standalone_toolchain_dir_name(target_arch):
    if target_arch == 'armv6' or target_arch == 'armv7':
        return 'standalone-arm'
    else:
        return 'standalone-' + target_arch

# Guess the triple to use for a given Android target architecture.
def guess_android_triple(target_arch):
    if target_arch == 'armv6':
        return 'arm-linux-androideabi'
    elif target_arch == 'armv7':
        return 'armv7-linux-androideabi'
    elif target_arch == 'arm64':
        return 'aarch64-linux-android'
    elif target_arch == 'x86':
        return 'i686-linux-android'
    elif target_arch == 'x86_64':
        return 'x86_64-linux-android'
    else:
        return target_arch

# Guess the value to pass with the -march flag for Android builds.
def guess_android_march(target_arch):
    if target_arch == 'armv7':
        return 'armv7-a'
    elif target_arch == 'arm64':
        # The -march flag is not used for baseline arm64.
        return ''
    elif target_arch == 'x86':
        return 'i686'
    elif target_arch == 'x86_64':
        # The -march flag is not used for baseline x86_64
        return ''
    return target_arch

# Guess the prefix used on the compiler's name.
def guess_compiler_prefix(target_arch):
    if target_arch == 'armv7':
        # The ARMv7 triple is different from its compiler's prefix.
        triple = guess_android_triple('armv6')
    else:
        triple = guess_android_triple(target_arch)
    if triple == target_arch:
        return ''
    return triple + '-'

# Returns any extra C/C++ compiler flags required when targeting Android on the
# given architecture.
def android_extra_cflags(target_arch):
    if target_arch == 'armv7':
        # The first three flags are required in order to guarantee ABI compatibility.
        # Additionally, Google recommends generating thumb instructions on Android.
        return '-mfloat-abi=softfp -mfpu=vfpv3-d16 -Wl,--fix-cortex-a8 -mthumb'
    elif target_arch == 'armv6':
        # Google recommends generating thumb instructions on Android.
        return '-mthumb'
    return ''

# Returns any extra linker flags required when targeting Android on the given
# architecture.
def android_extra_ldflags(target_arch):
    if target_arch == 'armv7':
        return '-Wl,--fix-cortex-a8'
    return ''

def validate_android_tools(opts):
    if opts['ANDROID_NDK_VERSION'] is None:
        opts['ANDROID_NDK_VERSION'] = 'r15'

    ndk_ver = opts['ANDROID_NDK_VERSION']

    toolchain_dir = opts['ANDROID_TOOLCHAIN_DIR']

    if opts['ANDROID_NDK'] is None:
        ndk = guess_android_tooldir(toolchain_dir, 'android-ndk')
        if ndk is None:
            error('Android NDK not found; set $ANDROID_NDK')
        opts['ANDROID_NDK'] = ndk

    if opts['ANDROID_NDK_PLATFORM_VERSION'] is None:
        opts['ANDROID_NDK_PLATFORM_VERSION'] = '16'

    if opts['ANDROID_API_VERSION'] is None:
        opts['ANDROID_API_VERSION'] = '28'
     
    api_ver = opts['ANDROID_API_VERSION']

    if opts['ANDROID_PLATFORM'] is None:
        opts['ANDROID_PLATFORM'] = 'android-' + api_ver

    if opts['ANDROID_SDK'] is None:
        sdk = guess_android_tooldir(toolchain_dir, 'android-sdk')
        if sdk is None:
            error('Android SDK not found; set $ANDROID_SDK')
        opts['ANDROID_SDK'] = sdk

    if opts['ANDROID_BUILD_TOOLS'] is None:
        tools = guess_android_build_tools(opts['ANDROID_SDK'])
        if tools is None:
            error('Android build tools not found; set $ANDROID_BUILD_TOOLS')
        opts['ANDROID_BUILD_TOOLS'] = tools

    if opts['ANDROID_TOOLCHAIN'] is None:
        dir = guess_android_tooldir(toolchain_dir, guess_standalone_toolchain_dir_name(opts['TARGET_ARCH']))
        if dir is None:
            error('Android toolchain not found for architecture {}; set $ANDROID_TOOLCHAIN'.format(opts['TARGET_ARCH']))
        prefix = guess_compiler_prefix(opts['TARGET_ARCH'])
        opts['ANDROID_TOOLCHAIN'] = os.path.join(dir,'bin',prefix)

    def android_tool(name, env, extra=""):
        if opts[env] is None:
            tool = opts['ANDROID_TOOLCHAIN'] + name
            if extra is not None:
                tool += ' ' + extra
            opts[env] = tool

    target_arch = opts['TARGET_ARCH']
    march = guess_android_march(target_arch)
    triple = guess_android_triple(target_arch)
    cflags = android_extra_cflags(target_arch)
    ldflags = android_extra_ldflags(target_arch)

    if opts['ANDROID_LIB_PATH'] is None:
        dir =guess_standalone_toolchain_dir_name(opts['TARGET_ARCH'])
        if dir is None:
            error('Android standalone toolchain not found for architecture {}'.format(opts['TARGET_ARCH']))
        
        opts['ANDROID_LIB_PATH'] = os.path.join(dir,triple,'lib')

    # All Android builds use Clang and make a lot of noise about unused
    # arguments (e.g. linker-specific arguments). Suppress them.
    cflags += ' -Qunused-arguments'

    if march != '':
        march = '-march=' + march
    android_tool('ar', 'AR')
    android_tool('clang', 'CC',
                 '-target {} {} -integrated-as {}'.format(triple,march,cflags))
    android_tool('clang++', 'CXX',
                 '-target {} {} -integrated-as {}'.format(triple,march,cflags))
    android_tool('clang++', 'LINK',
                 '-target {} {} -integrated-as -fuse-ld=bfd {}'.format(triple,march,ldflags))
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
        
    if opts['CROSS'] is not None:
        args.append('-Dcross_compile=1')
        
    args.append('-Dbuild_edition=' + opts['BUILD_EDITION'])

    args.append('-Duniform_arch=' + opts['UNIFORM_ARCH'])

    return args

def export_opts(opts, names):
    for n in names:
        if opts[n] is not None:
            print (n + '=' + opts[n])
            os.environ[n] = opts[n]

def gyp_define_args(opts, names):
    return ['-D{}={}'.format(key, opts[value])
            for key, value in names.iteritems()
            if opts[value] is not None]

def configure_linux(opts):
    host_platform(opts)
    validate_target_arch(opts)
    validate_java_tools(opts)
    
    configure_toolchain(opts)
    export_opts(opts, ('CC', 'CXX', 'AR', 'LINK', 'OBJCOPY', 'OBJDUMP', 'STRIP', 'LD'))
    
    args = core_gyp_args(opts) + ['-Dtarget_arch=' + opts['TARGET_ARCH'],
                                  '-Djavahome=' + opts['JAVA_SDK']]
    exec_gyp(args + opts['GYP_OPTIONS'])

def configure_emscripten(opts):
    host_platform(opts)
    validate_target_arch(opts)
    validate_emscripten_tools(opts)

    export_opts(opts, ('NODE_JS',))
    args = core_gyp_args(opts) + ['-Dtarget_arch=' + opts['TARGET_ARCH']]
    exec_gyp(args + opts['GYP_OPTIONS'])

def configure_android(opts):
    host_platform(opts)
    validate_target_arch(opts)
    validate_android_tools(opts)
    validate_java_tools(opts)

    export_opts(opts, ('ANDROID_BUILD_TOOLS', 'ANDROID_NDK',
                       'ANDROID_PLATFORM', 'ANDROID_SDK',
                       'ANDROID_NDK_VERSION', 'ANDROID_NDK_PLATFORM_VERSION',
                       'ANDROID_API_VERSION', 'ANDROID_LIB_PATH',
                       'JAVA_SDK', 'AR', 'CC', 'CXX', 'LINK', 'OBJCOPY',
                       'OBJDUMP', 'STRIP'))
    args = core_gyp_args(opts) + ['-Dtarget_arch=' + opts['TARGET_ARCH'],
                                  '-Dcross_compile=1',
                                  '-Gandroid_ndk_version=' + opts['ANDROID_NDK_VERSION'],
                                  '-Djavahome=' + opts['JAVA_SDK']]
    exec_gyp(args + opts['GYP_OPTIONS'])

def configure_win(opts):
    host_platform(opts)
    validate_target_arch(opts)
    validate_windows_tools(opts)

    # Make sure we strictly enforce TARGET_ARCH being x86 or x86_64
    if opts['TARGET_ARCH'] != 'x86' and opts['TARGET_ARCH'] != 'x86_64':
        error("TARGET_ARCH must be x86 or x86_64")

    # Map target_arch for gyp - x86_64 -> x64
    if opts['TARGET_ARCH'] == 'x86_64':
        opts['TARGET_ARCH'] = 'x64'

    args = core_gyp_args(opts) + ['-Gmsvs_version=' + opts['WIN_MSVS_VERSION']]
    args += gyp_define_args(opts, {'target_arch':    'TARGET_ARCH',
                                   'ms_speech_sdk5': 'MS_SPEECH_SDK5',
                                   'quicktime_sdk':  'QUICKTIME_SDK', })

    if platform.system() != 'Windows':
        args.append('-Dunix_configure=1')

    exec_gyp(args + opts['GYP_OPTIONS'])

def configure_mac(opts):
    host_platform(opts)
    validate_target_arch(opts)
    validate_xcode_sdks(opts)
    validate_java_tools(opts)
    copy_workspace_settings(opts)

    args = core_gyp_args(opts) + ['-Dtarget_sdk=' + opts['XCODE_TARGET_SDK'],
                                  '-Dhost_sdk=' + opts['XCODE_HOST_SDK'],
                                  '-Dtarget_arch=' + opts['TARGET_ARCH'],
                                  '-Djavahome=' + opts['JAVA_SDK']]
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

def copy_workspace_settings(opts):
    validate_gyp_settings(opts)
    if opts['BUILD_EDITION'] == 'commercial':
        project = os.path.join(opts['GENERATOR_OUTPUT'], '..', 'livecode-commercial.xcodeproj')
    else:
        project = os.path.join(opts['GENERATOR_OUTPUT'], 'livecode.xcodeproj')

    xcshareddata = os.path.join(project, 'project.xcworkspace', 'xcshareddata')

    if not os.path.exists(xcshareddata):
        os.makedirs(xcshareddata)

    workspacesettingsdest= os.path.join(xcshareddata, 'WorkspaceSettings.xcsettings')
    workspacesettingssource = os.path.join('config', 'WorkspaceSettings.xcsettings')

    if not os.path.exists(workspacesettingsdest):
        shutil.copyfile(workspacesettingssource, workspacesettingsdest)

if __name__ == '__main__':
    configure(sys.argv[1:])
