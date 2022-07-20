script "com.livecode.script-library.package"
/*
Copyright (C) 2016 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.

*/

------------------------------------------------------
-- LIBRARY INIT

/**

- path to jarsigner?
- path to semver library although perhaps it can bootstrap itself
- path to lc-compile
- paths to interface files

*/
command packageSetEnvironmentProperty pProperty, pValue
   
end packageSetEnvironmentProperty


------------------------------------------------------
-- PACKAGE LOADING AND CREATION

/**

use jarsigner??

*/
command packageFileVerifySignature pFile

end packageFileVerifySignature

/**

use jarsigner??

*/
command packageFileSign pFile
   -- use jarsigner??
end packageFileSign

/**

extract archive

*/
command packageFileToFolder pFile, pFolder
   
end packageFileToFolder

/**

create archive

*/
command packageFolderToFile pFolder, pFile
   
end packageFolderToFile

/**

Load package manifest and inclusion manifests

*/
command packageLoadManifests pFolder
   
end packageLoadManifests

/**

Get package properties on a currently loaded
package

*/
function packageProperty pName, pProperty
   
end packageProperty

/**

Set package properties on a currently loaded
package

*/
command packageSetProperty pName, pProperty, pValue
   
end packageSetProperty

/**

Search the folder for manifest files and update inclusions 
then write out package manifest with current properties.

*/
command packageSaveManifest pName
   
end packageSaveManifest

/**

List package requirements. Should search inclusion
manifests and package manifest for requirements and versions.

Should check loaded packages to determine if requirements
are currently satisfied then output ordered array with
keys:

- name
- version range
- range satisfied

*/
function packageRequirements pName
   
end packageRequirements

/**

A way to set the requirement version range

*/
command packageSetRequirement pName, pRequirementName, pVersionRange
   
end packageSetRequirement

/**

Should fail if the requirement is in an inclusion manifest

*/
command packageRemoveRequirement pName, pRequirementName
   
end packageRemoveRequirement

/**

Should list packages in the order they should be loaded in

*/
function packageRequirementsOrder
   
end packageRequirementsOrder

/**

Should check all requirements of all packages against
the package to determine if the update will break
required version ranges. Then ensure all the package
requirements version ranges are also satisfied.

*/
function packageSatisfiesLoadedPackages pPackageInfo
   
end packageSatisfiesLoadedPackages

/**

Get the right compile order for package inclusions
and compile if there is source.

*/
command packageCompile pName
   
end packageCompile


/**

Load a package that has previously had it's manifests
loaded into the runtime environment:

- extensions
- externals
- stackfiles
- tutorials
- custom controls
- templates
- frameworks
- docs (possible docs only inclusion?)

*/
command packageLoad pName
   
end packageLoad

/**

Create a new package

*/

command packageNew pName, pFolder
   
end packageNew

------------------------------------------------------
-- INCLUSIONS

/**

Create a new package inclusion

*/
command packageNewInclusion pName, pInclusionName
   
end packageNewInclusion

/**

Get package inclusions in ordered list with element keys:

- name
- version
- type (enum)

*/

function packageInclusions pName
   
end packageInclusions

/**

Get package inclusion properties on a currently loaded
package

*/

function packageInclusionProperty pName, pInclusionName, pProperty
   
end packageInclusionProperty

/**

Set package inclusion properties on a currently loaded
package. Should not be used where metadata can be gleaned
from the source file (LCB)

*/
command packageSetInclusionProperty pName, pInclusionName, pProperty, pValue
   
end packageSetInclusionProperty

/**

Write out package manifest with current properties.
Should not be used where metadata can be gleaned
from the source file (LCB)

*/
command packageSaveInclusionManifest pName, pInclusionName
   
end packageSaveInclusionManifest
