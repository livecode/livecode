# LiveCode Package Format

## Document Glossary

Some confusion exists between the term `package` and `extension` due
in part because LCB modules are referred to as extensions in the load
and unload syntax. To reduce confusion this document will *not* use the
term `extension` and in place use the following:

- component - a set of files intended to extend the LiveCode platform in
some way. A component can range from a collection of icons to standalone
engines.
- package - a collection of components and shared resources described by
a manifest

## States of a package

A LiveCode package may be either in compressed or extracted state where:

- Compressed state is a zip archive of the extracted state. The
  compressed state **may** be signed and verified with a zip archive
  signing tool such as jarsigner.
- Extracted state is the content of the zip archive extracted into a
  directory.

## Package directory structure:

- Optional paths are surrounded by square brackets `[]`
- Directories are identified by a trailing slash `/`
- Where no directory content is specified the content should be defined
in the manifest

```
manifest.xml
[License.txt]
[components/]
[resources/]
[docs/]
```

**Note:** All directory names should be lower case.

### manifest.xml
 
The manifest file describes the contents of the package along with any 
other metadata. Most of the information is inferable from the rest of
the archive, however it is repeated in the manifest to make it easier
for simple introspection.

The `manifest.xml` file schema can be found in [Appendix A](#manifest-schema)

### License.txt

A license file is required if it is referred to in the license element
of the package manifest.

### Components

Component paths listed in the manifest are relative to the components
directory not the package root so all components *must* be within the
components directory.

### Resources

Resource paths listed in the manifest are relative to the resources
directory not the package root.

Some resources are treated as special cases:
- `*.entitlement-snippet.xml` are merged into the entitlements file when
building a standalone for iOS or Mac
- `*.plist-snippet.xml` are merged into the plist when building a standalone
for iOS or Mac
- `*.manifest-snippet.xml` are merged into the manifest file when building a
standalone for Android or Windows

#### Compiled objects

Compiled objects will only be included on the appropriate corresponding
platform when building a standalone.

Compiled objects that are required for LCB handlers to bind to at
runtime may be placed in the resources directory. In order to bind to
the code the bind string must include the path to the code including
a common prefix followed platform and architecture specific suffix. For
example for a library function `libfoo>bar` the following file names
should be used.

The files paths should follow the following scheme within the resources
directory:

`<platform>/<arch>/<variant>/<name>.<extension` where `<arch>` and
`<variant>` could be dropped if they don't apply. For example:

- mac/libfoo.dylib -> mac (universal binary)
- mac/libfoo.bundle -> mac (universal binary)
- mac/x86_64/libfoo.framework -> mac 64 bit only
- linux/x86_64/libfoo.so -> linux 64 bit
- linux/x86/libfoo.so -> linux 32 bit
- windows/x86/libfoo.dll -> windows 32 bit
- ios/10_0/libfoo.a -> iOS static library (universal binary built
   against SDK 10.0)
- ios/libfoo.framework -> iOS static library framework (universal binary)
- android/armeabi/libfoo.so -> android aremabi
- android/libfoo.jar -> if libfoo was a java library
- android/libfoo.aar -> if libfoo was an android aar library package

##### Mobile plaforms

For mobile platforms the files are located here for convenience rather
than runtime binding. The files will be included in standalone builds
in their correct locations.

###### Android

- `*.aar` files will need to be extracted with libs and jars being
placed correctly and R classes generated.
- `*.jar` files will need to be placed correctly

###### iOS

iOS libraries must be statically linked. As a result third party
libraries rarely come with build configurations for creating a `*.dylib`
form. The most common form is via a static library framework. This
means that we will need to support static linking for simulator
deployment.

In order to link for iOS an additional `dependencies.txt` file should be
placed in the ios directory listing those frameworks required when
linking. Each line of the file will list a library or framework that
should be available on the system that is linking for iOS in the form:

```
{[weak-]framework|library}[min-sdk] <name>
```

If no SDK variant is specified for iOS then an attempt should be made
to link with the most recent available SDK as it may be unknown what SDK
was used to build a third party library or framework. If a dylib is
available that should be used for simulator deployment in lieu of static
linking.

### docs

The docs folder contains a tree of documentation resources for the
package components. The directory may be organised however the author
or their tooling sees fit. The following file formats should be
supported:

- `*.lcdoc` - load into the documentation dictionary
- `*.md` - load into the documentation guides

## Package development considerations

### .lcignore

A `.lcignore` file may be used to filter files to be included in a
package. It should be left to the package author if they want to place
files outside the normal directory structure or include files that are
not listed in the manifest. The caveat is *all* files in resources that
are *not* in the manifest *will* be included in *all* standalones using
the package while the reverse is true of the components directory where
*only* those files listed in the manifest *will* be included in a
standalone.

### Automatic updating of package manifest

Tooling should be developed to automatically update the package manifest
as files are added or removed from the resources directory. The package
manifest should also be updated with package requirements when a module
component is built. Such updates should not modify existing requirements
that may have been set by other tools or manually.

## Appendix A

### Manifest Schema

```
<rng:grammar xmlns:rng="http://relaxng.org/ns/structure/1.0" xmlns:a="http://relaxng.org/ns/compatibility/annotations/1.0" ns="" datatypeLibrary="http://www.w3.org/2001/XMLSchema-datatypes">
   <rng:start combine="choice">
      <rng:ref name="package"/>
   </rng:start>
   <rng:define name="package">
      <rng:element name="package">
         <rng:ref name="title"/>
         <rng:ref name="author"/>
         <rng:ref name="description"/>
         <rng:ref name="license"/>
         <rng:ref name="name"/>
         <rng:ref name="version"/>
         <rng:oneOrMore>
            <rng:ref name="metadata"/>
         </rng:oneOrMore>
         <rng:oneOrMore>
            <rng:ref name="requires"/>
         </rng:oneOrMore>
         <rng:oneOrMore>
            <rng:ref name="component"/>
         </rng:oneOrMore>
         <rng:oneOrMore>
            <rng:ref name="resource"/>
         </rng:oneOrMore>
         <rng:attribute name="version">
            <rng:data type="string"/>
         </rng:attribute>
      </rng:element>
   </rng:define>
   <rng:start combine="choice">
      <rng:ref name="author"/>
   </rng:start>
   <rng:define name="author">
      <rng:element name="author">
         <rng:data type="token"/>
      </rng:element>
   </rng:define>
   <rng:start combine="choice">
      <rng:ref name="description"/>
   </rng:start>
   <rng:define name="description">
      <rng:element name="description">
         <rng:data type="string"/>
      </rng:element>
   </rng:define>
   <rng:start combine="choice">
      <rng:ref name="license"/>
   </rng:start>
   <rng:define name="license">
      <rng:element name="license">
         <rng:data type="token"/>
      </rng:element>
   </rng:define>
   <rng:start combine="choice">
      <rng:ref name="version"/>
   </rng:start>
   <rng:define name="version">
      <rng:element name="version">
         <rng:data type="token"/>
      </rng:element>
   </rng:define>
   <rng:start combine="choice">
      <rng:ref name="requires"/>
   </rng:start>
   <rng:define name="requires">
      <rng:element name="requires">
         <rng:attribute name="name">
            <rng:data type="token"/>
         </rng:attribute>
         <rng:optional>
            <rng:attribute name="version">
               <rng:data type="token"/>
            </rng:attribute>
         </rng:optional>
      </rng:element>
   </rng:define>
   <rng:start combine="choice">
      <rng:ref name="component"/>
   </rng:start>
   <rng:define name="component">
      <rng:element name="component">
         <rng:zeroOrMore>
            <rng:ref name="target"/>
         </rng:zeroOrMore>
         <rng:ref name="type"/>
         <rng:ref name="name"/>
         <rng:optional>
            <rng:ref name="title"/>
         </rng:optional>
         <rng:choice>
            <rng:oneOrMore>
               <rng:ref name="property"/>
            </rng:oneOrMore>
            <rng:oneOrMore>
               <rng:ref name="event"/>
            </rng:oneOrMore>
            <rng:zeroOrMore>
               <rng:ref name="handler"/>
            </rng:zeroOrMore>
         </rng:choice>
         <rng:zeroOrMore>
            <rng:ref name="metadata"/>
         </rng:zeroOrMore>
         <rng:attribute name="path"/>
      </rng:element>
   </rng:define>
   <rng:start combine="choice">
      <rng:ref name="type"/>
   </rng:start>
   <rng:define name="type">
      <rng:element name="type">
         <rng:choice>
            <rng:value>library</rng:value>
            <rng:value>widget</rng:value>
            <rng:value>module</rng:value>
            <rng:value>script-library</rng:value>
            <rng:value>script-back</rng:value>
            <rng:value>script-front</rng:value>
            <rng:value>runtime</rng:value>
            <rng:value>external</rng:value>
            <rng:value>stack</rng:value>
            <rng:value>custom-control</rng:value>
            <rng:value>template</rng:value>
            <rng:value>framework</rng:value>
            <rng:value>tutorial</rng:value>
            <rng:value>theme</rng:value>
         </rng:choice>
      </rng:element>
   </rng:define>
   <rng:start combine="choice">
      <rng:ref name="property"/>
   </rng:start>
   <rng:define name="property">
      <rng:element name="property">
         <rng:attribute name="get"/>
         <rng:attribute name="name">
            <rng:data type="string"/>
         </rng:attribute>
         <rng:optional>
            <rng:attribute name="set"/>
         </rng:optional>
      </rng:element>
   </rng:define>
   <rng:start combine="choice">
      <rng:ref name="event"/>
   </rng:start>
   <rng:define name="event">
      <rng:element name="event">
         <rng:attribute name="name">
            <rng:data type="string"/>
         </rng:attribute>
         <rng:attribute name="parameters"/>
         <rng:attribute name="return"/>
      </rng:element>
   </rng:define>
   <rng:start combine="choice">
      <rng:ref name="handler"/>
   </rng:start>
   <rng:define name="handler">
      <rng:element name="handler">
         <rng:attribute name="name">
            <rng:data type="string"/>
         </rng:attribute>
         <rng:attribute name="parameters"/>
         <rng:attribute name="return">
            <rng:data type="string"/>
         </rng:attribute>
      </rng:element>
   </rng:define>
   <rng:start combine="choice">
      <rng:ref name="resource"/>
   </rng:start>
   <rng:define name="resource">
      <rng:element name="resource">      
         <rng:zeroOrMore>
            <rng:ref name="target"/>
         </rng:zeroOrMore>
         <rng:attribute name="path"/>      
      </rng:element>
   </rng:define>
   <rng:start combine="choice">
      <rng:ref name="title"/>
   </rng:start>
   <rng:define name="title">
      <rng:element name="title">
         <rng:data type="token"/>
      </rng:element>
   </rng:define>
   <rng:start combine="choice">
      <rng:ref name="name"/>
   </rng:start>
   <rng:define name="name">
      <rng:element name="name">
         <rng:data type="token"/>
      </rng:element>
   </rng:define>
   <rng:start combine="choice">
      <rng:ref name="metadata"/>
   </rng:start>
   <rng:define name="metadata">
      <rng:element name="metadata">
         <rng:attribute name="key">
            <rng:data type="token"/>
         </rng:attribute>
      </rng:element>
   </rng:define>
   <rng:start combine="choice">
      <rng:ref name="target"/>
   </rng:start>
   <rng:define name="target">
      <rng:element name="target">
         <rng:optional>
            <rng:attribute name="runtime">
               <rng:data type="boolean"/>
            </rng:attribute>
         </rng:optional>
         <rng:optional>
            <rng:attribute name="architecture">
               <rng:choice>
                  <rng:value>x86-32</rng:value>
                  <rng:value>x86-64</rng:value>
                  <rng:value>armv7</rng:value>
                  <rng:value>armv6</rng:value>
                  <rng:value>arm64</rng:value>
                  <rng:value>emscripten</rng:value>
               </rng:choice>
            </rng:attribute>
         </rng:optional>
         <rng:optional>
            <rng:attribute name="os">
               <rng:choice>
                  <rng:value>ios</rng:value>
                  <rng:value>android</rng:value>
                  <rng:value>mac</rng:value>
                  <rng:value>windows</rng:value>
                  <rng:value>linux</rng:value>
                  <rng:value>html5</rng:value>
               </rng:choice>
            </rng:attribute>
         </rng:optional>
         <rng:optional>
            <rng:attribute name="platform">
               <rng:choice>
                  <rng:value>desktop</rng:value>
                  <rng:value>mobile</rng:value>
                  <rng:value>server</rng:value>
               </rng:choice>
            </rng:attribute>
         </rng:optional>
      </rng:element>
   </rng:define>
</rng:grammar>
```

### Example Package Manifest

```
<?xml version="1.0" encoding="UTF-8"?>
<package version="1.0">
	<title>Human Readable Foo</title>
	<author>Mr Magoo</author>
	<description>Foo is a super amazing package that will do everything for you.</description>
	<license>SPDX expression</license>
	<name>a.reverse.domain.foo</name>
	<version>semver</version>
	<metadata key="dot.heirarchical.foo">string</metadata>
	<metadata key="dot.heirarchical.bar">string</metadata>
	<requires name="com.livecode.foobar" version="semver range"/>
	<requires name="com.livecode.baz" version="semver range"/>
	<component path="path/to/module.lcm">
		<type>library</type>
		<name>a.reverse.domain.foo.bar</name>
		<title>Human Readable Foo Bar</title>
		<handler name="foobar" parameters="in(integer),inout(string)" return="undefined"/>
	    <handler name="fubar" parameters="in(integer),inout(string)" return="undefined"/>
	    <metadata key="dot.heirarchical.foo">string</metadata>
	    <metadata key="dot.heirarchical.bar">string</metadata>
	</component>
	<component path="path/to/module.lcm">
		<type>widget</type>
		<name>a.reverse.domain.foo.baz</name>
		<title>Human Readable Foo Baz</title>
		<property name="foo" get="optional(integer)" set="optional(integer)"/>
		<property name="bar" get="string"/>
		<event name="click" parameters="in(integer),out(real)" return="optional(any)"/>
	   <event name="drag" parameters="in(integer),out(real)" return="optional(any)"/>
	   <metadata key="dot.heirarchical.foo">string</metadata>
	   <metadata key="dot.heirarchical.bar">string</metadata>
	</component>
	<component path="path/to/file.ext">
		<target platform="desktop" os="mac" architecture="x86-64" />
		<target platform="desktop" os="ios" architecture="armv7" />
		<target platform="desktop" os="ios" architecture="arm64" />
		<target platform="desktop" os="ios" architecture="x86-64" />
		<target platform="desktop" os="ios" architecture="x86-32" />
		<type>script-library</type>
		<name>a.reverse.domain.foo.example</name>
		<title>Human Readable Foo Example</title>
	</component>
	<resource path="path/to/file.ext">
		<target platform="desktop" os="mac" architecture="x86-64" />
		<target platform="desktop" os="ios" architecture="armv7" />
		<target platform="desktop" os="ios" architecture="arm64" />
		<target platform="desktop" os="ios" architecture="x86-64" />
		<target platform="desktop" os="ios" architecture="x86-32" />
	</resource>
	<resource path="path/to/file2.ext" />
	<resource path="path/to/file3.ext" >
		<target runtime="false" />
	</resource>
</package>
```
