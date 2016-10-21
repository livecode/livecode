# Undocumented multi-file libUrlMultipartFormAddPart removed

Previously, the **libUrlMultipartFormAddPart** command had the undocumented capability to accept multiple file names separated by commas. The handler failed to work for files that had commas in the name, however. The undocumented behaviour has been removed.  To add multiple files to a form, call **libURLMultipartFormAddPart** once for each file.
