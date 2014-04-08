#!/usr/bin/env perl

use warnings;

sub generateErrorsList
{
	my $sourceFile = $_[0];
	my $name = $_[1];
	
	my $array = "const char * ${name} = \n";
	
	open SOURCE, "<$sourceFile"
		or die "Could not open \"$sourceFile\": $!";
	my @lines = <SOURCE>;
	close SOURCE;
	
	my $found = 0;
	foreach $line (@lines)
	{
		# If the first word of the line is "enum" we have found the error list
		if ($line =~ /^\s*enum\s/)
		{
			$found = 1;
		}
		
		# Continue reading lines until we get to the enum
		if (!$found)
		{
			next;
		}
		
		# End of the enum
		if ($line =~ /};/)
		{
			last;
		}
		
		# The comment contains the error message for this error
		if ($line =~ m|^\s*//\s*\{|)
		{
			# Remove the newline character
			substr($line, -1) = "";
			
			# Remove the prefix from the error message
			$line =~ s|^\s*//\s*\{[^\}]*\}\s*|| ;
			
			# Protect any quotation marks
			$line =~ s/\"/\\\"/g ;
			
			# Output the message
			# tab & quote & line & "\n" & quote & return
			$array .= "\t\"${line}\\n\"\n";
		}
	}
	
	$array .= ";\n";
	return $array;
}

# Need to generate the error lists for both the parse and execution errors
my $path = $ARGV[0];
my $outputFile = $ARGV[1];

my $output = "";
$output .= generateErrorsList("${path}/executionerrors.h", "MCexecutionerrors");
$output .= "\n";
$output .= generateErrorsList("${path}/parseerrors.h", "MCparsingerrors");

# Write out the error lists
open OUTPUT, ">$outputFile"
	or die "Could not open output file \"$outputFile\": $!";
print OUTPUT $output;
close OUTPUT;
