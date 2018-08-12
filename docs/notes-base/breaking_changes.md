# Breaking changes

## Standalone Building

The standalone builder has always needed to close the stacks it builds 
for reasons pretty deeply ingrained in the code. However this causes a 
few problems, for example:

* values in script locals become empty
* behaviors are broken when the parent script is on / in a stack which closes
	
As an attempt to improve this situation, the code that locks messages
when closing and opening stacks for standalone builds has been removed.
This means that where previously mainstacks would not receive openStack
and closeStack messages during standalone build, they now do.

If this causes problems for your stack, you can exit from the handler if 
standalone building is in progress:

	on closeStack
		if the mode of stack "revStandaloneProgress" > 0 then
			exit closesStack
		end if
	end closeStack

## LiveCode Builder

### Exponentiation operator precedence
Prior to this release, exponentiation had lower precedence that unary 
minus. In order to write code that operates as expected in both this 
release and previous releases, please use parentheses where appropriate.

Using lc-compile tool in LiveCode 9:
	
	-1^2 = -1
	
Using lc-compile tool in LiveCode 8:
	
	-1^2 = 1