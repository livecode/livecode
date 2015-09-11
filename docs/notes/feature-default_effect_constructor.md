# LiveCode Builder Host Library
## Simplified Canvas Effect Constructor

You can now create a new canvas effect object without setting up an array of properties.
Default values will be assumed for unspecified properties:
* size: 5
* spread: 0
* distance: 5
* angle: 60

Example:
	variable tEffect as Effect
	put outer shadow effect into tEffect
