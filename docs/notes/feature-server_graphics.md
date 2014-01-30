# Server graphics support

Support for graphic rendering has been to the server engines. This allows users to use the export snapshot command as they would on the desktop and mobile platforms. For example, the following command can be called from within a server script in order to create a PNG image of the first card of the given stack (note that by default, server stacks have a black background).

	`export snapshot from card 1 of stack "graphics.livecode" to file "graphics.png" as PNG`

As part of these changes, the Linux server engine now requires PangoFT2 and Glib for text rendering.
