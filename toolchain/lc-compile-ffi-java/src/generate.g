/* Copyright (C) 2016 LiveCode Ltd.
 
 This file is part of LiveCode.
 
 LiveCode is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License v3 as published by the Free
 Software Foundation.
 
 LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.
 
 You should have received a copy of the GNU General Public License
 along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

'module' generate

'use'
    types
    support

'export'
    GeneratePackages

--------------------------------------------------------------------------------

'var' ModuleDependencyList : NAMELIST

'var' GeneratingPackageIndex : INT

'action' GeneratePackages(PACKAGELIST)

    'rule' GeneratePackages(List):
        OutputLCBBegin()
        GetOutputLCBModuleName(-> ModuleName)
        OutputWriteS("module ", ModuleName, " \n\n")
        OutputWrite("use com.livecode.java\n")
        GeneratingPackageIndex <- 1
        GenerateForEachPackage(List)
        OutputWrite("end module \n\n")
        OutputEnd()
    
'action' GenerateForEachPackage(PACKAGELIST)

    'rule' GenerateForEachPackage(packagelist(Head, Rest)):
       	GenerateSinglePackage(Head)
        GeneratingPackageIndex -> CurrentIndex
        GeneratingPackageIndex <- CurrentIndex + 1
        GenerateForEachPackage(Rest)
        
    'rule' GenerateForEachPackage(nil):
        -- do nothing

'action' GenerateSinglePackage(PACKAGE)

    'rule' GenerateSinglePackage(Package:package(_, _, Definitions)):
        GenerateForeignHandlers(Definitions)
        OutputWrite("\n")
        GenerateDefinitions(Definitions)

----------

'action' GenerateForeignHandlers(DEFINITION)

    'rule' GenerateForeignHandlers(sequence(Head, Tail)):
    	GenerateForeignHandlers(Head)
    	GenerateForeignHandlers(Tail)
    	
    'rule' GenerateForeignHandlers(class(_, _, Type, Definitions, _, _)):
        GenerateForeignHandlersOfClass(Type, Definitions)

    'rule' GenerateForeignHandlers(interface(_, Type, Definitions, _)):
        GenerateForeignHandlersOfClass(Type, Definitions)

    'rule' GenerateForeignHandlers(Definition):
        -- foreign handler declaration not needed
/*
    'rule' GenerateForeignHandlers(class(_, _, Type, Definitions, _, _)):
        GenerateForeignHandlersOfClass(Type, Definitions)

    'rule' GenerateForeignHandlers(use(_, _)):
        -- foreign handler declaration not needed
*/

'action' GenerateForeignHandlersOfClass(TYPE, DEFINITION)

    'rule' GenerateForeignHandlersOfClass(Type, sequence(Head, Tail)):
    	GenerateForeignHandlerOfClass(Type, Head)
    	GenerateForeignHandlersOfClass(Type, Tail)

    'rule' GenerateForeignHandlersOfClass(Type, nil):
		-- finished

'action' GenerateForeignHandlerOfClass(TYPE, DEFINITION)

    'rule' GenerateForeignHandlerOfClass(Type, method(_, Modifiers, Id, Signature, Alias, _)):
    	(|
            where(Alias -> id(AliasName))
            ResolveIdName(AliasName -> MethodName)
            ResolveIdName(Id -> RealName)
        ||
            ResolveIdName(Id -> MethodName)
            ResolveIdName(Id -> RealName)
        |)
        
        OutputWrite("__safe foreign handler ")
        OutputForeignHandlerName(Type, MethodName)
        OutputForeignHandlerSignatureWithParameter(Type, Signature, Modifiers)
        OutputMethodBindingString(Type, RealName, Signature, Modifiers)
        OutputWrite("\n")

    'rule' GenerateForeignHandlerOfClass(Type, constructor(_, _, Id, Signature, Alias)):
    	(|
            where(Alias -> id(AliasName))
            ResolveIdName(AliasName -> MethodName)
            ResolveIdName(Id -> RealName)
        ||
            ResolveIdName(Id -> MethodName)
            ResolveIdName(Id -> RealName)
        |)
        
        OutputWrite("__safe foreign handler ")
        OutputForeignHandlerName(Type, MethodName)
        OutputForeignSignatureWithReturnType(Type, Signature)
        OutputConstructorBindingString(Type, RealName, Signature)
        OutputWrite("\n")
        
    'rule' GenerateForeignHandlerOfClass(Type, variable(_, variablemodifiers(_, _, Modify, Instance), Id, VarType)):
		ResolveIdName(Id -> RealName)
		
		GenerateForeignGetterOfClass(Type, RealName, VarType, Instance)
		(|
			where(Modify -> final)
		||
			GenerateForeignSetterOfClass(Type, RealName, VarType, Instance)
		|)
		
    'rule' GenerateForeignHandlerOfClass(Type, constant(_, Id, VarType, nil)):
		ResolveIdName(Id -> RealName)
		GenerateForeignGetterOfClass(Type, RealName, VarType, class)

    'rule' GenerateForeignHandlerOfClass(Type, Definition):
        -- nothing yet

'action' GenerateForeignGetterOfClass(TYPE, NAME, TYPE, MODIFIER)
	
	'rule' GenerateForeignGetterOfClass(ClassType, Name, VarType, InstanceModifier)        
		OutputWrite("__safe foreign handler ")
        OutputForeignGetterName(ClassType, Name)
        OutputForeignGetterSignature(ClassType, Name, VarType, InstanceModifier)
        OutputGetVariableBindingString(ClassType, Name, VarType, InstanceModifier)
        OutputWrite("\n")
        
'action' GenerateForeignSetterOfClass(TYPE, NAME, TYPE, MODIFIER)
	
	'rule' GenerateForeignSetterOfClass(ClassType, Name, VarType, InstanceModifier)        
		OutputWrite("__safe foreign handler ")
        OutputForeignSetterName(ClassType, Name)
        OutputForeignSetterSignature(ClassType, Name, VarType, InstanceModifier)
        OutputSetVariableBindingString(ClassType, Name, VarType, InstanceModifier)
        OutputWrite("\n")

'action' OutputForeignGetterName(TYPE, NAME)

	'rule' OutputForeignGetterName(Type, FieldName):
	    TypeToUnqualifiedName(Type -> ClassName)
		OutputWriteI("_JNI_", ClassName, "")
		OutputWriteI("_Get_", FieldName, "")

'action' OutputForeignSetterName(TYPE, NAME)

	'rule' OutputForeignSetterName(Type, FieldName):
	    TypeToUnqualifiedName(Type -> ClassName)
		OutputWriteI("_JNI_", ClassName, "")
		OutputWriteI("_Set_", FieldName, "")

'action' OutputForeignHandlerName(TYPE, NAME)

	'rule' OutputForeignHandlerName(Type, MethodName):
	    TypeToUnqualifiedName(Type -> ClassName)
		OutputWriteI("_JNI_", ClassName, "")
		OutputWriteI("_", MethodName, "")

    'rule' OutputForeignHandlerName(Type, MethodName):
    	print("fail")
    	print(MethodName)
    	
'action' OutputForeignGetterSignature(TYPE, NAME, TYPE, MODIFIER)

    'rule' OutputForeignGetterSignature(ObjType, Name, VarType, InstanceModifier):
    	OutputWrite("(")
    	(|
    		where(InstanceModifier -> class)
    	||
    	    OutputWrite("in pObj as ")
        	GenerateType(ObjType)
    	|)
        OutputWrite(")")
        GenerateJavaReturns(VarType)
        
'action' OutputForeignSetterSignature(TYPE, NAME, TYPE, MODIFIER)

    'rule' OutputForeignSetterSignature(ObjType, Name, VarType, InstanceModifier):
    	OutputWrite("(")
    	(|
    		where(InstanceModifier -> class)
    	||
    	    OutputWrite("in pObj as ")
        	GenerateType(ObjType)
        	OutputWrite(", ")
    	|)
        OutputWriteI("in pParam_", Name, "")
        OutputWrite(" as ")
        GenerateJavaType(VarType)
        OutputWrite(") returns nothing\n")

'action' OutputForeignHandlerSignatureWithParameter(TYPE, SIGNATURE, MODIFIER)

    'rule' OutputForeignHandlerSignatureWithParameter(ObjType, Signature, methodmodifiers(_, _, _, _, _, InstanceModifier)):
    	OutputForeignSignatureWithParameter(ObjType, Signature, InstanceModifier)
    	
    'rule' OutputForeignHandlerSignatureWithParameter(ObjType, Signature, interfacemethodmodifiers(InstanceModifier)):
    	OutputForeignSignatureWithParameter(ObjType, Signature, InstanceModifier)
		
'action' OutputForeignSignatureWithParameter(TYPE, SIGNATURE, MODIFIER)

    'rule' OutputForeignSignatureWithParameter(ObjType, signature(Params, ReturnType), InstanceModifier):
        OutputWrite("(")
        (|
        	where(InstanceModifier -> class)
        ||
        	OutputWrite("in pObj as ")
        	GenerateType(ObjType)
        	
        	(|
        		where(Params -> nil)
        	||
        		OutputWrite(", ")
        	|)
        |)
        (|
            where(Params -> nil)
        ||
            GenerateJavaParams(Params)
        |)
        OutputWrite(")")
        GenerateJavaReturns(ReturnType)
        
'action' OutputForeignSignatureWithReturnType(TYPE, SIGNATURE)

    'rule' OutputForeignSignatureWithReturnType(ReturnType, signature(Params, nil)):
        OutputWrite("(")
        GenerateJavaParams(Params)
        OutputWrite(")")
        GenerateJavaReturns(ReturnType)
        
'action' OutputMethodBindingString(TYPE, NAME, SIGNATURE, MODIFIER)

	'rule' OutputMethodBindingString(ClassType, MethodName, Signature, methodmodifiers(_, _, _, _, _, InstanceModifier)):
        OutputMethodBindingStringInstance(ClassType, MethodName, Signature, InstanceModifier)

    'rule' OutputMethodBindingString(ClassType, MethodName, Signature, interfacemethodmodifiers(InstanceModifier)):
        OutputMethodBindingStringInstance(ClassType, MethodName, Signature, InstanceModifier)

'action' OutputMethodBindingStringInstance(TYPE, NAME, SIGNATURE, MODIFIER)

'rule' OutputMethodBindingStringInstance(ClassType, MethodName, Signature, InstanceModifier):
        TypeToQualifiedName(ClassType -> QualifiedClass)
		OutputWriteI(" \\\nbinds to \"java:", QualifiedClass, ">")
		OutputWriteI("", MethodName, "")
		OutputJavaSignature(Signature)
        (|
            where(InstanceModifier -> class)
            OutputWrite("!static")
        ||
        |)
		OutputWrite("\"")
		
'action' OutputConstructorBindingString(TYPE, NAME, SIGNATURE)

	'rule' OutputConstructorBindingString(ClassType, MethodName, Signature):
        TypeToQualifiedName(ClassType -> QualifiedClass)
		OutputWriteI(" \\\nbinds to \"java:", QualifiedClass, ">")
		OutputWrite("new")
		OutputJavaSignature(Signature)
		OutputWrite("\"")
		
'action' OutputSetVariableBindingString(TYPE, NAME, TYPE, MODIFIER)

	'rule' OutputSetVariableBindingString(ClassType, FieldName, VarType, InstanceModifier):
        TypeToQualifiedName(ClassType -> QualifiedClass)
		OutputWriteI(" \\\nbinds to \"java:", QualifiedClass, ">")
		OutputWriteI("set.", FieldName, "")
		OutputWrite("(")
        OutputJavaTypeCode(VarType)
        OutputWrite(")")
		(|
            where(InstanceModifier -> class)
            OutputWrite("!static")
        ||
        |)
		OutputWrite("\"")
		
'action' OutputGetVariableBindingString(TYPE, NAME, TYPE, MODIFIER)

	'rule' OutputGetVariableBindingString(ClassType, FieldName, VarType, InstanceModifier):
        TypeToQualifiedName(ClassType -> QualifiedClass)
		OutputWriteI(" \\\nbinds to \"java:", QualifiedClass, ">")
		OutputWriteI("get.", FieldName, "")
		OutputWrite("(")
        OutputWrite(")")
        OutputJavaTypeCode(VarType)
		(|
            where(InstanceModifier -> class)
            OutputWrite("!static")
        ||
        |)
		OutputWrite("\"")
		
'action' OutputJavaSignature(SIGNATURE)
	
	'rule' OutputJavaSignature(signature(Params, ReturnType))
		OutputWrite("(")
        OutputJavaParams(Params)
        OutputWrite(")")
        OutputJavaReturnTypeCode(ReturnType)
		
'action' OutputJavaParams(PARAMETERLIST)

    'rule' OutputJavaParams(nil):

    'rule' OutputJavaParams(parameterlist(Head, nil)):
        OutputJavaParam(Head)

    'rule' OutputJavaParams(parameterlist(Head, Tail)):
        OutputJavaParam(Head)
        OutputJavaParams(Tail)
	
'action' OutputJavaParam(PARAMETER)

	'rule' OutputJavaParam(parameter(_, _, Type)):
		OutputJavaTypeCode(Type)

    'rule' OutputJavaParam(variadic(_)):
        OutputWrite("JArray")

----

'action' GenerateClassDefinitions(TYPE, DEFINITION)

    'rule' GenerateClassDefinitions(ObjType, sequence(Left, Right)):
        GenerateClassDefinitions(ObjType, Left)
        GenerateClassDefinitions(ObjType, Right)

    'rule' GenerateClassDefinitions(ObjType, constant(_, Id, Type, Value)):
        ResolveIdName(Id -> SymbolName)
        TypeToUnqualifiedName(ObjType -> ObjName)
        (|
            where(Value -> nil)
            OutputWriteI("handler ", ObjName, "_")
            OutputWriteI("Get_", SymbolName, "()")
            GenerateReturns(Type)
            OutputWrite("\n")
            OutputCallForeignGetter(ObjType, SymbolName, Type, class)
            OutputWrite("\nend handler")
        ||
            OutputWriteI("public constant ", SymbolName, "")
            OutputWrite(" is ")
            GenerateValue(Value)
        |)
        OutputWrite("\n\n")

    'rule' GenerateClassDefinitions(ObjType, variable(_, variablemodifiers(_, _, Modify, Instance), Id, Type)):
        ResolveIdName(Id -> SymbolName)
        TypeToUnqualifiedName(ObjType -> ObjName)
        OutputWriteI("public handler ", ObjName, "_")
        OutputWriteI("Get_", SymbolName, "(")
        (|
			where(Instance -> class)
		||
			OutputWrite("in pObj as ")
			GenerateType(ObjType)
		|)
		OutputWrite(")")
		GenerateReturns(Type)
		OutputWrite("\n")
        OutputCallForeignGetter(ObjType, SymbolName, Type, Instance)
        OutputWrite("\n")
        OutputWrite("end handler")
        OutputWrite("\n\n")
        (|
        	where(Modify -> final)
        ||
        	OutputWriteI("public handler ", ObjName, "_")
			OutputWriteI("Set_", SymbolName, "(")
			(|
				where(Instance -> class)
			||
				OutputWrite("in pObj as ")
				GenerateType(ObjType)
				OutputWrite(", ")
			|)
			OutputWriteI("in pParam_", SymbolName, " as ")
			GenerateType(Type)
			OutputWrite(") returns nothing\n")
			OutputCallForeignSetter(ObjType, SymbolName, Type, Instance)
			OutputWrite("end handler")
			OutputWrite("\n\n")
        |)

    'rule' GenerateClassDefinitions(ObjType, method(_, Modifiers, Id, Signature, Alias, Throws)):
        (|
            where(Alias -> id(AliasName))
            ResolveIdName(AliasName -> Name)
        ||
            ResolveIdName(Id -> Name)
        |)
        TypeToUnqualifiedName(ObjType -> ObjName)
        OutputWriteI("public handler ", ObjName, "_")
        OutputWriteI("", Name, "")
        GenerateMethodSignatureWithParameter(ObjType, Signature, Modifiers)
        OutputWrite("\n")
        OutputCallForeignHandler(ObjType, Name, Signature, Modifiers)
        OutputWrite("end handler")
        OutputWrite("\n\n")

    'rule' GenerateClassDefinitions(ObjType, constructor(_, Modifiers, Id, Signature, Alias)):
        (|
            where(Alias -> id(AliasName))
            ResolveIdName(AliasName -> Name)
        ||
            ResolveIdName(Id -> Name)
        |)
        OutputWriteI("public handler ", Name, "_Constructor")
        GenerateSignatureWithReturnType(ObjType, Signature)
        OutputWrite("\n")

        OutputCallForeignConstructor(ObjType, Name, Signature)
        OutputWrite("end handler")
        OutputWrite("\n\n")

    'rule' GenerateClassDefinitions(ObjType, nil):
        -- do nothing

'action' OutputCallForeignConstructor(TYPE, NAME, SIGNATURE)

    'rule' OutputCallForeignConstructor(ObjType, Name, signature(Params, nil))
     	OutputConvertToForeignParams(Params)
		OutputWrite("\treturn ")
		OutputForeignHandlerName(ObjType, Name)	
		OutputWrite("(")
		OutputForeignCallParams(Params)
		OutputWrite(")\n")

'action' OutputCallForeignHandlerParams(TYPE, NAME, PARAMETERLIST, MODIFIER)

	'rule' OutputCallForeignHandlerParams(ObjType, Name, Params, InstanceModifier)
            OutputForeignHandlerName(ObjType, Name)	
            OutputWrite("(")
            (|
            	where(InstanceModifier -> class)
            	OutputForeignCallParams(Params)
            ||
				OutputWrite("pObj")
				(|
					where(Params -> nil)
				||
					OutputWrite(", ")
					OutputForeignCallParams(Params)
				|)
			|)
			OutputWrite(")")		

'action' OutputCallForeignHandler(TYPE, NAME, SIGNATURE, MODIFIER)

	'rule' OutputCallForeignHandler(ObjType, Name, Signature, methodmodifiers(_, _, _, _, _, InstanceModifier))
        OutputCallForeignHandlerModifier(ObjType, Name, Signature, InstanceModifier)

    'rule' OutputCallForeignHandler(ObjType, Name, Signature, interfacemethodmodifiers(InstanceModifier))
        OutputCallForeignHandlerModifier(ObjType, Name, Signature, InstanceModifier)

'action' OutputCallForeignHandlerModifier(TYPE, NAME, SIGNATURE, MODIFIER)

    'rule' OutputCallForeignHandlerModifier(ObjType, Name, signature(Params, ReturnType), InstanceModifier)
		OutputConvertToForeignParams(Params)
        (|
            where(ReturnType -> nil)
            OutputWrite("\t")
            OutputCallForeignHandlerParams(ObjType, Name, Params, InstanceModifier)
			OutputWrite("\n")
        ||
            (|
                RequiresConversion(ReturnType)
                OutputWrite("\tvariable tJNIResult as ")
                GenerateJavaType(ReturnType)
                OutputWrite("\n\tput ")
                OutputCallForeignHandlerParams(ObjType, Name, Params, InstanceModifier)
                OutputWrite(" into tJNIResult\n")
                OutputWrapperReturn(ReturnType)
            ||
                OutputWrite("\treturn ")
                OutputCallForeignHandlerParams(ObjType, Name, Params, InstanceModifier)
            |)
			OutputWrite("\n")
        |)

'action' OutputForeignGetter(TYPE, NAME, MODIFIER)

    'rule' OutputForeignGetter(ObjType, Name, class):
        OutputForeignGetterName(ObjType, Name)
        OutputWrite("(pObj)")

    'rule' OutputForeignGetter(ObjType, Name, Instance):
        OutputForeignGetterName(ObjType, Name)
        OutputWrite("()")

'action' OutputCallForeignGetter(TYPE, NAME, TYPE, MODIFIER)

	'rule' OutputCallForeignGetter(ObjType, Name, Type, InstanceModifier)
            (|
                RequiresConversion(Type)
                OutputWrite("\tvariable tJNIResult as ")
                GenerateJavaType(Type)
                OutputWrite("\n\tput ")
                OutputForeignGetter(ObjType, Name, InstanceModifier)
                OutputWrite(" into tJNIResult\n")
                OutputWrapperReturn(Type)
            ||
                OutputWrite("\treturn ")
                OutputForeignGetter(ObjType, Name, InstanceModifier)
            |)

'action' OutputCallForeignSetter(TYPE, NAME, TYPE, MODIFIER)

	'rule' OutputCallForeignSetter(ObjType, Name, Type, InstanceModifier)
			OutputConvertToForeignParameter(Name, Type)
            OutputWrite("\t")
            OutputForeignSetterName(ObjType, Name)
            OutputWrite("(")
            (|
            	where(InstanceModifier -> class)
            ||
            	OutputWrite("pObj, ")
            |)
            (|
				RequiresConversion(Type)
	            OutputWriteI("tParam_", Name, "")
			||
        	    OutputWriteI("pParam_", Name, "")
	        |)
            OutputWrite(")")            
            OutputWrite("\n")

'action' OutputConvertToForeignParams(PARAMETERLIST)

	'rule' OutputConvertToForeignParams(nil):

    'rule' OutputConvertToForeignParams(parameterlist(Head, Tail)):
        OutputConvertToForeignParam(Head)
        OutputConvertToForeignParams(Tail)	

'action' OutputConvertToForeignParameter(NAME, TYPE)

    'rule' OutputConvertToForeignParameter(SymbolName, Type):
			RequiresConversion(Type)
			OutputWriteI("\tvariable tParam_", SymbolName, "")
			OutputWrite(" as ")
			GenerateJavaType(Type) 
			OutputWrite("\n")
			OutputWrite("\tput ")
			GenerateType(Type)
			OutputWrite("To")
			GenerateJavaType(Type)
			OutputWriteI("(pParam_", SymbolName, ") into ")
			OutputWriteI("tParam_", SymbolName, "\n\n")

    'rule' OutputConvertToForeignParameter(SymbolName, Type):
        -- Do nothing if Type does not require conversion

'action' OutputConvertToForeignParam(PARAMETER)

    'rule' OutputConvertToForeignParam(parameter(_, Id, Type)):
        ResolveIdName(Id -> SymbolName)
        OutputConvertToForeignParameter(SymbolName, Type)

    'rule' OutputConvertToForeignParam(variadic(_)):
        -- TODO: Deal with variadic args

'condition' RequiresConversion(TYPE)

     -- strings require conversion
    'rule' RequiresConversion(string):
    
    'rule' RequiresConversion(jarray(_, byte, _)):

'action' OutputForeignCallParams(PARAMETERLIST)

	'rule' OutputForeignCallParams(nil):
	
	'rule' OutputForeignCallParams(parameterlist(Head, nil)):
        OutputForeignCallParam(Head)
        	
    'rule' OutputForeignCallParams(parameterlist(Head, Tail)):
        OutputForeignCallParam(Head)
        OutputWrite(", ")
        OutputForeignCallParams(Tail)	

'action' OutputForeignCallParam(PARAMETER)

    'rule' OutputForeignCallParam(parameter(_, Id, Type)):
        ResolveIdName(Id -> SymbolName)
        (|
			RequiresConversion(Type)
            OutputWriteI("tParam_", SymbolName, "")
		||
            OutputWriteI("pParam_", SymbolName, "")
        |)

    'rule' OutputForeignCallParam(variadic(_)):
        -- TODO: Deal with variadic args
        OutputWrite("va_args")

'action' OutputWrapperReturn(TYPE)

	'rule' OutputWrapperReturn(Type)
        RequiresConversion(Type)
        OutputWrite("\treturn ")
        OutputConvertJava(Type)
        OutputWrite("(tJNIResult)")

    'rule' OutputWrapperReturn(Type)
		OutputWrite("\treturn tJNIResult")

'action' OutputConvertJava(TYPE)

	'rule' OutputConvertJava(Type):
		GenerateType(Type)
		OutputWrite("From")
		GenerateJavaType(Type)

'action' GenerateMethodSignatureWithParameter(TYPE, SIGNATURE, MODIFIER)

	'rule' GenerateMethodSignatureWithParameter(ObjType, Signature, methodmodifiers(_, _, _, _, _, InstanceMod))
		GenerateSignatureWithParameter(ObjType, Signature, InstanceMod)
		
	'rule' GenerateMethodSignatureWithParameter(ObjType, Signature, interfacemethodmodifiers(InstanceMod))
		GenerateSignatureWithParameter(ObjType, Signature, InstanceMod)

'action' GenerateSignatureWithParameter(TYPE, SIGNATURE, MODIFIER)

    'rule' GenerateSignatureWithParameter(ObjType, signature(Params, ReturnType), InstanceModifier):
        OutputWrite("(")
        (|
        	where(InstanceModifier -> class)
        ||
        	OutputWrite("in pObj as ")
        	GenerateType(ObjType)
        	
        	(|
        		where(Params -> nil)
        	||
        		OutputWrite(", ")
        	|)
        |)
        (|
            where(Params -> nil)
        ||
            GenerateParams(Params)
        |)
        OutputWrite(")")
        GenerateReturns(ReturnType)

'action' GenerateSignatureWithReturnType(TYPE, SIGNATURE)

    'rule' GenerateSignatureWithReturnType(ReturnType, signature(Params, nil)):
        OutputWrite("(")
        GenerateParams(Params)
        OutputWrite(")")
        GenerateReturns(ReturnType)

'action' GenerateParams(PARAMETERLIST)

    'rule' GenerateParams(nil):

    'rule' GenerateParams(parameterlist(Head, nil)):
        GenerateParam(Head)

    'rule' GenerateParams(parameterlist(Head, Tail)):
        GenerateParam(Head)
        OutputWrite(", ")
        GenerateParams(Tail)
        
'action' GenerateJavaParams(PARAMETERLIST)

    'rule' GenerateJavaParams(nil):

    'rule' GenerateJavaParams(parameterlist(Head, nil)):
        GenerateJavaParam(Head)

    'rule' GenerateJavaParams(parameterlist(Head, Tail)):
        GenerateJavaParam(Head)
        OutputWrite(", ")
        GenerateJavaParams(Tail)

'action' GenerateParam(PARAMETER)

    'rule' GenerateParam(parameter(_, Id, Type)):
        QuerySymbolId(Id -> SymbolInfo)
        ResolveIdName(Id -> SymbolName)
        OutputWriteI("in pParam_", SymbolName, "")
        OutputWrite(" as ")
        GenerateType(Type)

    'rule' GenerateParam(variadic(_)):
        -- TODO: Deal with variadic args
        OutputWrite("in va_args as List")

'action' GenerateJavaParam(PARAMETER)

    'rule' GenerateJavaParam(parameter(_, Id, Type)):
        QuerySymbolId(Id -> SymbolInfo)
        ResolveIdName(Id -> SymbolName)
        OutputWriteI("in pParam_", SymbolName, "")
        OutputWrite(" as ")
        GenerateJavaType(Type)

    'rule' GenerateJavaParam(variadic(_)):
        OutputWrite("in pArgs as List")

'action' GenerateDefinitions(DEFINITION)

    'rule' GenerateDefinitions(sequence(Left, Right)):
        GenerateDefinitions(Left)
        GenerateDefinitions(Right)

    'rule' GenerateDefinitions(class(_, Modifiers, Type, Definitions, Inherits, Implements)):
        GenerateClassDefinitions(Type, Definitions)

    'rule' GenerateDefinitions(interface(_, Type, Definitions, Inherits)):
        GenerateClassDefinitions(Type, Definitions)

    'rule' GenerateDefinitions(use(_,_)):
        -- do nothing

    'rule' GenerateDefinitions(nil):
        -- do nothing

'action' GenerateInherits(TYPELIST)

    'rule' GenerateInherits(nil):

    'rule' GenerateInherits(Typelist)
        OutputWrite(" inherits ")
        GenerateTypelist(Typelist)

'action' GenerateImplements(TYPELIST)

    'rule' GenerateImplements(nil):

    'rule' GenerateImplements(Typelist)
        OutputWrite(" implements ")
        GenerateTypelist(Typelist)

'action' GenerateTypelist(TYPELIST)

    'rule' GenerateTypelist(typelist(Head, Tail)):
        GenerateType(Head)
        (|
            where(Tail -> nil)
        ||
            OutputWrite(", ")
            GenerateTypelist(Tail)
        |)

'action' GenerateOptionalAlias(OPTIONALID)

    'rule' GenerateOptionalAlias(nil):
        -- do nothing

    'rule' GenerateOptionalAlias(id(Id)):
        ResolveIdName(Id -> AliasName)
        OutputWriteI(" named ", AliasName, "")

'action' GenerateOptionalThrows(OPTIONALID)

    'rule' GenerateOptionalThrows(nil):
        -- do nothing

    'rule' GenerateOptionalThrows(id(Id)):
        ResolveIdName(Id -> ThrowsName)
        OutputWriteI(" throws ", ThrowsName, "")

'action' GenerateReturns(TYPE)

    'rule' GenerateReturns(Type):
        OutputWrite(" returns ")
        GenerateType(Type)
        
'action' GenerateJavaReturns(TYPE)

    'rule' GenerateJavaReturns(Type):
        OutputWrite(" returns ")
        GenerateJavaType(Type)

'action' GenerateJavaType(TYPE)

    'rule' GenerateJavaType(byte):
        OutputWrite("JByte")

    'rule' GenerateJavaType(short):
        OutputWrite("JShort")

    'rule' GenerateJavaType(int):
        OutputWrite("JInt")

    'rule' GenerateJavaType(long):
        OutputWrite("JLong")

    'rule' GenerateJavaType(float):
        OutputWrite("JFloat")

    'rule' GenerateJavaType(double):
        OutputWrite("JDouble")

    'rule' GenerateJavaType(boolean):
        OutputWrite("JBoolean")

    'rule' GenerateJavaType(char):
        OutputWrite("JChar")

    'rule' GenerateJavaType(string):
        OutputWrite("JString")
        
    'rule' GenerateJavaType(named(_, Id, Parameters)):
    	OutputWrite("JObject")

    'rule' GenerateJavaType(template(_, Id, Parameters)):
    	OutputWrite("JObject")

    'rule' GenerateJavaType(wildcard(_, _)):
    	OutputWrite("JObject")

    'rule' GenerateJavaType(placeholder(_, Id)):
        ResolveIdName(Id -> SymbolName)
        OutputWriteI("", SymbolName, "")

    -- byte array special case
    'rule' GenerateJavaType(jarray(_, byte, Dimension)):
        eq(Dimension, 1)
    	OutputWrite("JByteArray")

    -- output java array
    'rule' GenerateJavaType(jarray(_, Type, Dimension)):
    	OutputWrite("JArray")

    'rule' GenerateJavaType(nil):
    	OutputWrite("nothing")
    	
'action' GenerateType(TYPE)

    'rule' GenerateType(byte):
        OutputWrite("Number")

    'rule' GenerateType(short):
        OutputWrite("Number")

    'rule' GenerateType(int):
        OutputWrite("Number")

    'rule' GenerateType(long):
        OutputWrite("Number")

    'rule' GenerateType(float):
        OutputWrite("Number")

    'rule' GenerateType(double):
        OutputWrite("Number")

    'rule' GenerateType(boolean):
        OutputWrite("Boolean")

    'rule' GenerateType(char):
        OutputWrite("Number")

    'rule' GenerateType(string):
        OutputWrite("String")

    'rule' GenerateType(named(_, Id, Parameters)):
    	OutputWrite("JObject")

    'rule' GenerateType(template(_, Id, Parameters)):
    	OutputWrite("JObject")

    'rule' GenerateType(wildcard(_, _)):
    	OutputWrite("JObject")

    'rule' GenerateType(placeholder(_, Id)):
        ResolveIdName(Id -> SymbolName)
        OutputWriteI("", SymbolName, "")

    -- byte array special case as data
    'rule' GenerateType(jarray(_, byte, Dimension)):
        eq(Dimension, 1)
    	OutputWrite("Data")

    -- output java array
    'rule' GenerateType(jarray(_, Type, Dimension)):
    	OutputWrite("List")

    'rule' GenerateType(nil):
    	OutputWrite("nothing")

'action' OutputJavaTypeCode(TYPE)

    'rule' OutputJavaTypeCode(byte):
        OutputWrite("B")

    'rule' OutputJavaTypeCode(short):
        OutputWrite("S")

    'rule' OutputJavaTypeCode(int):
        OutputWrite("I")

    'rule' OutputJavaTypeCode(long):
        OutputWrite("J")

    'rule' OutputJavaTypeCode(float):
        OutputWrite("F")

    'rule' OutputJavaTypeCode(double):
        OutputWrite("D")

    'rule' OutputJavaTypeCode(boolean):
        OutputWrite("Z")

    'rule' OutputJavaTypeCode(char):
        OutputWrite("C")

    'rule' OutputJavaTypeCode(string):
        OutputWrite("Ljava/lang/String;")

    'rule' OutputJavaTypeCode(named(_, Id, _)):
		ResolveIdQualifiedName(Id -> Name)
		JavaQualifiedNameToClassPath(Name -> ClassPath)
		OutputWriteI("L", ClassPath, ";")

    'rule' OutputJavaTypeCode(template(_, Id, _)):
		ResolveIdQualifiedName(Id -> Name)
		JavaQualifiedNameToClassPath(Name -> ClassPath)
		OutputWriteI("L", ClassPath, ";")

    'rule' OutputJavaTypeCode(placeholder(_, Id)):
		print("skip generic placeholder")

    'rule' OutputJavaTypeCode(jarray(_, Type, Dimension)):
        OutputJavaArrayTypeCode(Type, Dimension)

    'rule' OutputJavaTypeCode(nil):

-- Ensure return type code is always specified as V if returning nothing
'action' OutputJavaReturnTypeCode(TYPE)

    'rule' OutputJavaReturnTypeCode(nil):
        OutputWrite("V")

    'rule' OutputJavaReturnTypeCode(Type):
        OutputJavaTypeCode(Type)

'action' OutputJavaArrayTypeCode(TYPE, INT)

    -- output java array
    'rule' OutputJavaArrayTypeCode(Type, Dimension):
        OutputWrite("[")
        (|
            ne(Dimension, 1)
            OutputJavaArrayTypeCode(Type, Dimension - 1)
        ||
            OutputJavaTypeCode(Type)
        |)

'action' GenerateTypeList(TYPELIST)

    'rule' GenerateTypeList(typelist(Head, Tail)):
        GenerateType(Head)
        (|
            where(Tail -> nil)
        ||
            OutputWrite(", ")
            GenerateTypeList(Tail)
        |)

'action' GenerateJArray(TYPE, INT)

    'rule' GenerateJArray(Type, Dim):
        OutputWrite("List of ")
        (|
            ne(Dim, 1)
            GenerateJArray(Type, Dim - 1)
        ||
            GenerateType(Type)
        |)


'action' GenerateValue(EXPRESSION)

    'rule' GenerateValue(true(_)):
        OutputWrite("true")

    'rule' GenerateValue(false(_)):
        OutputWrite("false")

    'rule' GenerateValue(real(_, Value)):
        OutputWriteD("", Value, "")

    'rule' GenerateValue(integer(_, Value)):
        OutputWriteN("", Value, "")

    'rule' GenerateValue(string(_, Value)):
        OutputWriteS("\"", Value, "\"")


-- TODO: use appropriate modifiers
/*
'action' GenerateModifiers(MODIFIER)

    'rule' GenerateModifiers(classmodifiers(Access, StrictFP, Inherit, Modify, Instance)):
        GenerateModifiers(Access)
        GenerateModifiers(StrictFP)
        GenerateModifiers(Inherit)
        GenerateModifiers(Modify)
        GenerateModifiers(Instance)

    'rule' GenerateModifiers(interfacemethodmodifiers(Modifier)):
        GenerateModifiers(Modifier)

    'rule' GenerateModifiers(variablemodifiers(Access, Transient, Modify, Instance)):
        GenerateModifiers(Access)
        GenerateModifiers(Transient)
        GenerateModifiers(Modify)
        GenerateModifiers(Instance)

    'rule' GenerateModifiers(methodmodifiers(Access, Sync, Native, StrictFP, Inherit, Instance)):
        GenerateModifiers(Access)
        GenerateModifiers(Sync)
        GenerateModifiers(Native)
        GenerateModifiers(StrictFP)
        GenerateModifiers(Inherit)
        GenerateModifiers(Instance)

    'rule' GenerateModifiers(constructormodifiers(Access)):
        GenerateModifiers(Access)

    'rule' GenerateModifiers(protected):
        OutputWrite("protected ")

    'rule' GenerateModifiers(synchronized):
        OutputWrite("synchronized ")

    'rule' GenerateModifiers(native):
        OutputWrite("native ")

    'rule' GenerateModifiers(strictfp):
        OutputWrite("strictfp ")

    'rule' GenerateModifiers(abstract):
        OutputWrite("abstract ")

    'rule' GenerateModifiers(final):
        OutputWrite("final ")

    'rule' GenerateModifiers(class):
        OutputWrite("class ")

    'rule' GenerateModifiers(volatile):
        OutputWrite("volatile ")

    'rule' GenerateModifiers(transient):
        OutputWrite("transient ")

    'rule' GenerateModifiers(default):
        OutputWrite("default ")

    'rule' GenerateModifiers(public):

    'rule' GenerateModifiers(inferred):
*/
--------------------------------------------------------------------------

'action' TypeToQualifiedName(TYPE -> NAME)

    'rule' TypeToQualifiedName(named(_, Id, _) -> Name)
		ResolveIdQualifiedName(Id -> Name)
		
    'rule' TypeToQualifiedName(template(_, Id, _) -> Name)
		ResolveIdQualifiedName(Id -> Name)

'action' TypeToUnqualifiedName(TYPE -> NAME)

    'rule' TypeToUnqualifiedName(named(_, Id, _) -> Name)
		ResolveIdName(Id -> Name)

    'rule' TypeToUnqualifiedName(template(_, Id, _) -> Name)
		ResolveIdName(Id -> Name)

--------------------------------------------------------------------------------
-- Defined in check.g
'action' QueryId(ID -> MEANING)
'action' QueryPackageOfId(ID -> ID)

'condition' QuerySymbolId(ID -> SYMBOLINFO)
'condition' QueryPackageId(ID -> PACKAGEINFO)

-- Defined in bind.g
'action' ResolveIdName(ID -> NAME)
'action' ResolveIdQualifiedName(ID -> NAME)
--------------------------------------------------------------------------------
