#!/usr/bin/env perl

use strict;
use warnings;

# Open the file specified on the command line
my $file = $ARGV[0];
open (my $FILE, '<:encoding(UTF-8)', $file) or die "Could not open file '$file' $!";

# Loop over the lines of the file
while (my $line = <$FILE>)
{
	# Remove the trailing newline
	chomp $line;
	
	# Remove leading punctuation that Gyp adds (leading '\""' or '"')
	$line =~ s/^(\\\")?\"//;
	
	# Remove trailing punctuation that Gyp adds (trailing '"\"' or '"')
	$line =~ s/(\"\\)?\"$//;
	
	# Replace the .exe or .dll suffix of each file with .pdb
	$line =~ s/\.(exe|dll)$/\.pdb/;
	
	# Output the filename
	print "$line\n";
}
