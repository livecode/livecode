# Starting Activities and Handling Results

A new handler has been added to facilitate starting activities by Intent and
handling results. The `StartActivityForResult` handler registers a listener to
handle the `onActivityResult` method of the engine activity and when handled
calls a callback handler with the request code, result code and Intent object.