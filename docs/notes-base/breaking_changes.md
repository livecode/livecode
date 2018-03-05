# Breaking changes

## LiveCode Builder

### Exponentiation operator precedence
Prior to this release, exponentiation had lower precedence that unary 
minus. In order to write code that operates as expected in both this 
release and previous releases, please use parentheses where appropriate.

Using lc-compile tool in LiveCode 9:
	
	-1^2 = -1
	
Using lc-compile tool in LiveCode 8:
	
	-1^2 = 1