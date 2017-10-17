#!/usr/bin/env python

# This script removes lines from a file that match any lines from a second file.
# Note that this will not preserve the order of lines, and will remove duplicates.

#Usage: remove_matching <file1> <file2> [<file3>]

# Output is written to stdout, or to a third file if specified

import sys

if len(sys.argv) < 3 or len(sys.argv) > 4:
	print "ERROR: incorrect number of arguments"
	sys.exit(1)
	
file1 = set(open(sys.argv[1]).readlines())
file2 = set(open(sys.argv[2]).readlines())

if len(sys.argv) == 4:
	outfile = open(sys.argv[3], mode="w")
else:
	outfile = sys.stdout

diff = list(file1.difference(file2))
diff.sort()
outfile.writelines(diff)
