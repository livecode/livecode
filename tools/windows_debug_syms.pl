#!/usr/bin/env perl

# Replace the .exe suffix for each executable with .pdb
for (@ARGV)
{
	s/\.exe$/\.pdb/;
}

print join("\n", @ARGV);
