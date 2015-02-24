# LiveCode Builder Packages
Copyright 2015 LiveCode Ltd.

## Introduction
12345678901234567890123456789012345678901234567890123456789012345678901234567890
LiveCode Builder is built around the idea of 'modules' - independently compiled
units of code which can be selectively used by others.

In a larger system, however, just the compiled module code is not enough. There
are other things which are naturally related to the module which don't make sense
to be included within the modules binary file - principally because they are not
of direct interest to the virtual machine which must execute the code.

To solve this problem, compiled LCB modules are embedded in a more general
container called a 'package'. A package is a ZIP format archive file with a
standard structure which combines modules together with other related resources
and metadata which can be used by an environment loading it.

## Structure

An LCB package is a ZIP archive with a standard layout:

  <root>/
    mimetype
    manifest.xml
    modules/
      <compiled module code>
    code/
      mac-x86_64-10_6/
        <compiled foreign code for 64-bit Mac min version 10.6>
      iossimulator-i386-4_3/
        <compiled foreign code for 32-bit iOS Simulator min version 4.3>
      windows-i386/
        <compiled foreign code for 32-bit windows>
      <arch>
        ...
    resources/
      <resource files for compiled modules>
    source/
      <source files for compiled modules>
    support/
      <structure defined by loading environment>
    docs/
      <structure defined by loading environment>
      
The support, docs and manifest file are not used by the LCB runtime itself and
provide supporting information for the environment that is intended to load and
use the package.

## Contents

### Manifest

### Modules

### Support

### Resources

### Code

### Source

### Docs

## Signing
