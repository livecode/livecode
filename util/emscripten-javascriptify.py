#!/usr/bin/env python

# This script runs the command that converts the LLVM bitcode files
# produced by the emscripten compiler to JavaScript.

import sys
import os
import subprocess

# Compiler settings
# -----------------
#
# These should generally match the settings in config/emscripten-settings.gypi
settings = {
    'ALLOW_MEMORY_GROWTH': 1,
    'ASSERTIONS': 1,
    'DEMANGLE_SUPPORT': 1,
    'EMTERPRETIFY': 1,
    'EMTERPRETIFY_ASYNC': 1,
    'LINKABLE': 1,
    'RESERVED_FUNCTION_POINTERS': 1024,
    'TOTAL_MEMORY': 67108864,
    'WARN_ON_UNDEFINED_SYMBOLS': 1,
    }

# Get the build type and the compiler command
# -------------------------------------------

env_verbose = os.getenv('V', '0')
env_emcc = os.getenv('EMCC', 'emcc')
env_build_type = os.getenv('BUILDTYPE', 'Debug')
env_cflags = os.getenv('CFLAGS', [])

if env_build_type == 'Release':
    optimisation_flags = ['-Os', '-g0']
else:
    optimisation_flags = ['-O2', '-g3']

# Separate out separate elements of command line
emcc = env_emcc.split()
cflags = env_cflags.split()

# Process command line options
# ----------------------------
#
# Each option absorbs all subsequent arguments up to the next option.
# Options are identified by the fact they start with "--".

option = None
options = {}
for arg in sys.argv[1:]:
    if arg.startswith('--'):
        option = arg[2:]
        options[option] = []
    else:
        if option is None:
            print('ERROR: unrecognized option \'{}\''.format(arg))
            sys.exit(1)
        options[option].append(arg)

# Special handling for "--whitelist"
if options.has_key('whitelist'):
    settings['EMTERPRETIFY_WHITELIST'] = '@{}'.format(options['whitelist'][0])

# Construct emcc command line
# ---------------------------

command = emcc + ["--emrun"] + optimisation_flags + cflags

for input in options['input']:
    command.append(input)
    
command += ['-o', options['output'][0]]

for setting in sorted(settings.keys()):
    command += ['-s', '{}={}'.format(setting, settings[setting])]

for option in ['pre-js', 'shell-file', 'js-library']:
    if options.has_key(option):
        for value in options[option]:
            command += ['--' + option, value]

# Run emcc
# --------

if env_verbose.strip() is not '0':
    print(" ".join(command))

emcc_result = subprocess.call(command)
sys.exit(emcc_result)
