# Showing a modal dialog confuses mouse state.
When a modal dialog is shown, the engine will now immediate reset the mouse state to ensure the context of events is the new dialog. In particular, mouseRelease will be sent if the mouse is down and mouseLeave will be sent if the mouse is within the previous window.

