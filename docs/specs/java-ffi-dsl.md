# Java FFI DSL Syntax
The Java FFI DSL is used to describe the classes and interfaces of a 
java package, and their constructors, methods, variables and constants.

All java class definitions should have a unique representation in the 
DSL (indeed in many cases it will be very similar).

The extension `.lcb-java` is recommended for java class definitions 
using this syntax.

## DSL syntax
### Package definition
A package consists of a declaration of the Name and a collection of 
class and interface definitions, and use clauses.

	Package
	  : "foreign" "package" <Name: QualifiedName>
		{ Definition SEPARATOR }
		"end" "package"

A definition is either a class or an interface definition, or a use clause.

	Definition
	  : Use
	  | Interface
	  | Class

### Use clauses
A use clause specifies which other java classes and interfaces are used 
within this package. 

	Use
	  : "use" <PackageName: QualifiedName> "." <ClassName: STRING>

### Class and interface definitions
Class and interface definitions contain lists of the constants, 
variables, methods and constructors that are members of the class or 
interface. They also specify how the class or interface fits in to the 
java Object hierarchy.

	Interface
	  : "interface" <Interface: ClassType> ["inherits" <Parents: InterfaceList>] SEPARATOR
	    { InterfaceDef SEPARATOR }
	    "end" "interface"		

	Class
	  : [ <Modifiers: ClassModifiers> ] "class" <Class: ClassType> ["inherits" <Parent: ClassType>] ["implements" <List: InterfaceList>] SEPARATOR
	    { ClassDef SEPARATOR }
	    "end" "class"		

Interfaces can only define methods and constants.

	InterfaceDef
	  : [ <Modifiers: InterfaceMethodModifier> ] "method" <Name: STRING> <ParamList: Parameters> ["named" <Alias: STRING>] ["throws" <List: ClassList>] "returns" <ReturnType: Type>
	  | "constant" <Name: STRING> "as" <VarType: Type> ["is" <Exp: Expression>]	  

Classes can also define constructors and variables.

	ClassDef
	  : [ <Modifiers: MethodModifiers> ] "method" <Name: STRING> <ParamList: Parameters> ["named" <Alias: STRING>] ["throws" <List: ClassList>] "returns" <ReturnType: Type>
	  | [ <Modifiers: ConstructiorModifiers> ] "constructor" <Name: STRING> ["named" <Alias: STRING>]
	  | [ <Modifiers: VariableModifiers> ] "variable" <Name: STRING> "as" <VarType: Type>
	  | "constant" <Name: STRING> "as" <VarType: Type> ["is" <Exp: Expression>]	  

### Implements / inherits
Classes and interfaces can only implement/inherit other classes or 
interfaces of the appropriate type.

The only difference between ClassList and InterfaceList is that it is a 
parse error if the items in InterfaceList are not defined as interfaces.

	InterfaceList
	  : <Head: ClassTypeInstance> [ "," <Tail: InterfaceList> ]

	ClassList
	  : <Head: ClassTypeInstance> [ "," <Tail: ClassList> ]
	  
### Modifiers
Modifiers give extra information about how or in what context classes or 
their members can be used. For example, if the "class" instance modifier 
is applied to a method of a class, an instance of the class is not 
required in order to call that method (i.e. `static`, in java parlance). 

The majority of the modifier information is not currently used, but 
should be provided anyway so that better glue code can be generated in 
the future.

	ClassModifiers
	  : [ <Access: AccessModifier> ] [ <StrictFP: StrictFPModifier> ] [ <Inheritance: InheritanceModifier> ] [ <Modify: ModifyModifier> ] [ <Instance: InstanceModifier> ]

	MethodModifiers
	  : [ <Access: AccessModifier> ] [ <Sync: SyncModifier> ] [ <Native: NativeModifier> ] [ <StrictFP: StrictFPModifier> ] [ <Inheritance: InheritanceModifier> ] [ <Instance: InstanceModifier> ]
	  
	ConstructorModifiers
	  : [ <Access: AccessModifier> ]  
	  
	VariableModifiers
	  : [ <Access: AccessModifier> ] [ <Transient: TransientModifier> ] [ <Modify: ModifyModifier> ] [ <Instance: InstanceModifier> ]

	InterfaceMethodModifier
	  : "default"
	  | "class"
	  
	AccessModifier
	  : "public"
	  | "protected"
	  
	SyncModifier
	  : "synchronized"

	NativeModifier
	  : "native"
	  
	StrictFPModifier
	  : "strictfp"
	  
	InheritanceModifier
	  : "abstract"
	  : "final"
	  
	InstanceModifier
	  : "class"
	  
	ModifyModifier
	  : "final"
	  | "volatile"
	  
	TransientModifier
	  : "transient"
	  

### Parameters
Note: all java method parameters are `in` parameters from the LCB point 
of view.

	Parameters
	  : "(" [ <Head: Parameter> [ "," <Tail: Parameters> ] ")"
	  | "..."
	  
	Parameter
	  : <Name: STRING> "as" <ParamType: Type>
	  
### Types
Types in Java are either primitive, derived from the Object class, or 
arrays of either of those.

	Type
	  : "byte"
	  | "short"
	  | "int"
	  | "long"
	  | "float"
	  | "double"
	  | "boolean"
	  | "char"
	  | "String"
	  | Type "[" "]"
	  | ClassType
	  
	ClassType
	  : <Name: STRING> ["<" <Params: ClassTypeParamList> ">" ]
	
	ClassTypeParamList
	  : <Head: ClassTypeParam> [ "," <Tail: ClassTypeParamList> ]
	
	ClassTypeParam
	  : ClassType
	  | <Placeholder: STRING>
	  | "?" [ <WildcardBounds: Bounds> ]
	  
	Bounds
	  : "extends" <Parent: ClassTypeInstance>
	  | "super" <Child: ClassTypeInstance>
	  
## Examples
### The java.lang.Object class
The [java.lang.Object](https://docs.oracle.com/javase/7/docs/api/java/lang/Object.html)
class can be described in the DSL as follows:

	class Object
		constructor Object()
	
		final method getClass() returns Class<?>
		method hashCode() returns int
		method equals(obj as Object) returns boolean
		protected method clone() returns Object
		method toString() returns String
		final method notify() returns nothing
		final method notifyAll() returns nothing
		final method wait(timeout as long) named waitTimeout returns nothing
		final method wait(timeout as long, nanos as int) named waitNanos returns nothing
		final method wait() returns nothing
		protected method finalize() returns nothing
	end class
	
### The java.lang.reflect.TypeVariable class 
The [java.lang.reflect.TypeVariable](https://docs.oracle.com/javase/7/docs/api/java/lang/reflect/TypeVariable.html) 
class can be translated into the DSL as follows:

	interface TypeVariable<D> inherits Type
		method getBounds() returns Type[]
		method getGenericDeclaration() returns D
		method getName() returns String
	end interface
	
### The android.os.BatteryManager class
The charging/discharging functionality of the [android.os.BatteryManager](https://developer.android.com/reference/android/os/BatteryManager.html)
class can be translated as follows:

	class BatteryManager
		constant ACTION_CHARGING as String is "android.os.action.CHARGING"
		constant ACTION_DISCHARGING as String is "android.os.action.DISCHARGING"
		method isCharging() returns boolean
	end class