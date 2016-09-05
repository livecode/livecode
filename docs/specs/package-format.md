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

### manifest.xml
 
The manifest file describes the contents of the package along with any 
other metadata. Most of the information is inferable from the rest of
the archive, however it is repeated in the manifest to make it easier
for simple introspection.

The `manifest.xml` file has the following schema:

```
<?xml version="1.0" encoding="utf-8"?>
<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema" elementFormDefault="qualified">
    <xs:element name="package">
        <xs:complexType>
            <xs:sequence>
                <xs:element ref="title"/>
                <xs:element ref="author"/>
                <xs:element ref="description"/>
                <xs:element ref="license"/>
                <xs:element ref="name"/>
                <xs:element ref="version"/>
                <xs:element maxOccurs="unbounded" ref="metadata"/>
                <xs:element maxOccurs="unbounded" ref="requires"/>
                <xs:element maxOccurs="unbounded" ref="component"/>
                <xs:element maxOccurs="unbounded" ref="resource"/>
            </xs:sequence>
            <xs:attribute name="version" use="required" type="xs:string"/>
        </xs:complexType>
    </xs:element>
    <xs:element name="author" type="xs:token"/>
    <xs:element name="description" type="xs:string"/>
    <xs:element name="license" type="xs:token"/>
    <xs:element name="version" type="xs:token"/>
    <xs:element name="requires">
        <xs:complexType>
            <xs:attribute name="name" use="required" type="xs:token"/>
            <xs:attribute name="version" use="optional" type="xs:token"/>
        </xs:complexType>
    </xs:element>
    <xs:element name="component">
        <xs:complexType>
            <xs:sequence>
                <xs:element minOccurs="0" maxOccurs="unbounded" ref="target"/>
                <xs:element ref="type"/>
                <xs:element ref="name"/>
                <xs:element minOccurs="0" ref="title"/>
                <xs:choice>
                    <xs:sequence>
                        <xs:element maxOccurs="unbounded" ref="property"/>
                        <xs:element maxOccurs="unbounded" ref="event"/>
                    </xs:sequence>
                    <xs:element minOccurs="0" maxOccurs="unbounded" ref="handler"/>
                </xs:choice>
                <xs:element minOccurs="0" maxOccurs="unbounded" ref="metadata"/>
            </xs:sequence>
            <xs:attribute name="path" use="required"/>
        </xs:complexType>
    </xs:element>
    <xs:element name="type">
        <xs:simpleType>
            <xs:restriction base="xs:token">
                <xs:enumeration value="library" />
                <xs:enumeration value="widget" />
                <xs:enumeration value="module" />
                <xs:enumeration value="script-library" />
                <xs:enumeration value="script-back" />
                <xs:enumeration value="script-front" />
                <xs:enumeration value="runtime" />
                <xs:enumeration value="external" />
                <xs:enumeration value="stack" />
                <xs:enumeration value="custom-control" />
                <xs:enumeration value="template" />
                <xs:enumeration value="framework" />
                <xs:enumeration value="tutorial" />
                <xs:enumeration value="theme" />
            </xs:restriction>
        </xs:simpleType>
    </xs:element>
    <xs:element name="property">
        <xs:complexType>
            <xs:attribute name="get" use="required"/>
            <xs:attribute name="name" use="required" type="xs:string"/>
            <xs:attribute name="set"/>
        </xs:complexType>
    </xs:element>
    <xs:element name="event">
        <xs:complexType>
            <xs:attribute name="name" use="required" type="xs:string"/>
            <xs:attribute name="parameters" use="required"/>
            <xs:attribute name="return" use="required"/>
        </xs:complexType>
    </xs:element>
    <xs:element name="handler">
        <xs:complexType>
            <xs:attribute name="name" use="required" type="xs:string"/>
            <xs:attribute name="parameters" use="required"/>
            <xs:attribute name="return" use="required" type="xs:string"/>
        </xs:complexType>
    </xs:element>
    <xs:element name="resource">
        <xs:complexType>
            <xs:element minOccurs="0" maxOccurs="unbounded" ref="target"/>
            <xs:attribute name="path" use="required"/>
        </xs:complexType>
    </xs:element>
    <xs:element name="title" type="xs:token"/>
    <xs:element name="name" type="xs:token"/>
    <xs:element name="metadata">
        <xs:complexType>
            <xs:simpleContent>
                <xs:extension base="xs:token">
                    <xs:attribute name="key" use="required" type="xs:token"/>
                </xs:extension>
            </xs:simpleContent>
        </xs:complexType>
    </xs:element>
    <xs:element name="target">
        <xs:complexType>
            <xs:attribute name="runtime" use="optional" type="xs:boolean" />
            <xs:attribute name="architecture" use="optional">
                <xs:simpleType>
                    <xs:restriction base="xs:token">
                        <xs:enumeration value="x86-32" />
                        <xs:enumeration value="x86-64" />
                        <xs:enumeration value="armv7" />
                        <xs:enumeration value="armv6" />
                        <xs:enumeration value="arm64" />
                        <xs:enumeration value="emscripten" />
                    </xs:restriction>
                </xs:simpleType>
            </xs:attribute>
            <xs:attribute name="os" use="optional">
                <xs:simpleType>
                    <xs:restriction base="xs:token">
                        <xs:enumeration value="ios" />
                        <xs:enumeration value="android" />
                        <xs:enumeration value="mac" />
                        <xs:enumeration value="windows" />
                        <xs:enumeration value="linux" />
                        <xs:enumeration value="html5" />
                    </xs:restriction>
                </xs:simpleType>
            </xs:attribute>
            <xs:attribute name="platform" use="optional">
                <xs:simpleType>
                    <xs:restriction base="xs:token">
                        <xs:enumeration value="desktop" />
                        <xs:enumeration value="mobile" />
                        <xs:enumeration value="server" />
                    </xs:restriction>
                </xs:simpleType>
            </xs:attribute>
        </xs:complexType>
    </xs:element>
</xs:schema>

```

Here the 'version' field in the package tag is the version of the
package manifest XML. The version of the package is used by dependants
to ensure they are able to use the version currently in use. The version
of the requirements is a semver range expression of acceptable versions.

### License.txt

A license file is required if it is referred to in the license element
of the package manifest.

### components

Component paths are relative to the components directory not the package
root.

### resources

Resource paths are relative to the resources directory not the package
root.

Some resources are treated as special cases:
- `*.entitlement.snippet` are merged into the entitlements file when
building a standalone for iOS or Mac
- `*.plist.snippet` are merged into the plist when building a standalone
for iOS or Mac
- `*.manifest.snippet` are merged into the manifest file when building a
standalone for Android or Windows

#### Compiled objects

Compiled objects will only be included on the appropriate corresponding
platform when building a standalone.

Compiled objects that are required for LCB handlers to bind to at
runtime may be placed in the resources directory. In order to bind to
the code the bind string must unclude the path to the code including
a common prefix followed platform and architecture specific suffix. For
example for a library function `libfoo>bar` the following file names
should be used.

- mac/libfoo.dylib -> mac (universal binary)
- mac/libfoo.bundle -> mac (universal binary)
- mac/libfoo.framework -> mac (universal binary)
- linux/libfoo-x86-64.so -> linux 64 bit
- linux/libfoo-x86-32.so -> linux 32 bit
- windows/libfoo-x86-32.dll -> windows 32 bit
- ios/libfoo.a -> iOS static library
- ios/libfoo.framework -> iOS static library framework
- android/libfoo-armv6.so -> android armv6
- android/libfoo.jar -> if libfoo was a java library
- android/libfoo.aar -> if libfoo was an android aar library package

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

## Appendix

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
