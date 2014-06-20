# File handle leak on Mac
When getting a file or binfile URL where the target file exists and is of zero size, a file handle leak occurs.
