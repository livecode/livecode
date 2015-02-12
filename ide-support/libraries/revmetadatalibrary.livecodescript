script "revmetadatalibrary"

on revLoadLibrary
   insert the script of me into back
end revLoadLibrary

on revUnloadLibrary
   remove the script of me from back
end revUnloadLibrary

################################################################################
#
# Metadata library, this allows the setting and getting of metadata items on 
# Revolution objects. The metadata is divided into types, which map to the 
# custom property sets cREVGeneral, cREVDatabase etc.
#
################################################################################

constant kTypePrefix = "cREV"

# Returns
#   A list of available metadata types
private function metadataTypes
  return "general,database,geometry,table,debugger"
end metadataTypes

# Parameters
#   pType : a metadata type
# Returns
#   The name of the custom property set the stores metadata items of this type
private function metadataPropertySetFromTypeName pType
  return kTypePrefix & toUpper(char 1 of pType) & char 2 to -1 of pType
end metadataPropertySetFromTypeName

# Parameters
#   pObject : reference to an object
#   pType : a metadata type
#   pTag : the name of the metadata item to retrieve
# Returns
#   The value of the specified metadata item for the specified object.
function revMetadataGet pObject, pType, pTag
   if pType is not among the items of metadataTypes() then
      throw "invalid_type: " & pType
   end if
   
   local tPropertyName
   put metadataPropertySetFromTypeName(pType) & "[" & pTag & "]" into tPropertyName
   lock messages
   get the tPropertyName of pObject
   unlock messages
   return it
end revMetadataGet

private command log pMessage
   put pMessage & return after msg
end log

# Parameters
#   pObject : reference to an object
#   pType : a metadata type
#   pTag : the name of the metadata item to set
#   pValue : the new value to set the item to.
# Description
#   Sets the specified metadata item for pObject to pValue
command revMetadataSet pObject, pType, pTag, pValue
   if pType is not among the items of metadataTypes() then
      throw "invalid_type: " & pType
   end if
   
   local tPropertyName
   put metadataPropertySetFromTypeName(pType) & "[" & pTag & "]" into tPropertyName
   
   # Lock messages around this for now to ensure that revCommon does not interfere with this..
   lock messages
   set the tPropertyName of pObject to pValue
   unlock messages
end revMetadataSet



