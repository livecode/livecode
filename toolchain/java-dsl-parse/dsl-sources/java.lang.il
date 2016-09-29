foreign package java.lang named com.livecode.wrapped.java.lang

use java.lang.reflect.GenericDeclaration
use java.lang.reflect.Type
use java.lang.reflect.TypeVariable
use java.lang.reflect.Constructor
use java.lang.reflect.Method
use java.lang.reflect.Field

class Object
	constructor Object()
	
	final method getClass() returns Class
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

class Class<T> inherits Object implements GenericDeclaration, Type
	method asSubclass(clazz as Class) returns Class
	method cast(obj as Object) returns T
	method desiredAssertionStatus() returns boolean
	method getCanonicalName() returns String
	method getClasses() returns Class[]
	method getComponentType() returns Class
	method getConstructors() returns Constructor[]
	method getDeclaredClasses() returns Class[]
	method getDeclaredConstructors() returns Constructor[]
	method getDeclaredField(name as String) returns Field
	method getDeclaredFields() returns Field[]
	method getDeclaredMethods() returns Method[]
	method getDeclaringClass() returns Class
	method getEnclosingClass() returns Class
	method getEnclosingConstructor() returns Constructor
	method getEnclosingMethod() returns Method
	method getEnumConstants() returns T[]
	method getField(name as String) returns Field
	method getFields() returns Field[]
	method getGenericInterfaces() returns Type[]
	method getGenericSuperclass() returns Type
	method getInterfaces() returns Class[]
	method getMethods() returns Method[]
	method getModifiers() returns int
	method getName() returns String
	method getSimpleName() returns String
	method getSuperclass() returns Class<T>
	method getTypeParameters() returns TypeVariable<Class<T>>[]
	method isAnonymousClass() returns boolean
	method isArray() returns boolean
	method isAssignableFrom(cls as Class) returns boolean
	method isEnum() returns boolean
	method isInstance(obj as Object) returns boolean
	method isInterface() returns boolean
	method isLocalClass() returns boolean
	method isMemberClass() returns boolean
	method isPrimitive() returns boolean
	method isSynthetic() returns boolean
	method newInstance() returns T
	method toString() returns String
end class

end package