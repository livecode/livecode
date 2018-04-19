# Print To PDF

The browser widget now includes a handler to print the current content to
a PDF file via `call widget` or `send widget`. For example:

    put the long id of widget "browser" into tBrowser
    put specialFolderPath("documents") & "/report.pdf" into tFile
    call widget "PrintToPDF" of tBrowser with tFile, 595, 842
