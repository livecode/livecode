# LiveCode Builder Host Library
## Canvas library

A new statement `begin effect only layer with <effect>` has been implemented to
allow drawing shadow & glow effects without rendering the source content

For example to draw a dropshadow of a rectangle without drawing the rectangle itself:
	variable tEffect as Effect
	put outer shadow effect into tEffect

	begin effect only layer with tEffect on this canvas
	fill rectangle path of rectangle [50,50,100,100] on this canvas
	end layer on this canvas
