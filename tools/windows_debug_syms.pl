#!/usr/bin/env perl

# Replace the .exe suffix for each executable with .pdb
for (@ARGV)
{
	s/\.(exe|dll)/\.pdb/g;
}

print join("\n", @ARGV);