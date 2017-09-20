# Fix treatment of NUL containing arguments in ask dialogs

Prior to 7, any arguments passed to LiveCode provided ask dialogs
(e.g. ask question) containing NUL would be truncated at the NUL. After
7, any such arguments would cause incorrect calling of the ask dialog.
The pre-7 behavior has been resurrected, meaning that trailing NUL
bytes in arguments passed to ask dialogs will be ignored.
