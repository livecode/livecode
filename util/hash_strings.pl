#!/usr/bin/env perl

use warnings;
use File::Basename;
use File::Temp qw(tempfile);

# Arguments
my $sourceFile = $ARGV[0];
my $targetFile = $ARGV[1];
my $perfectCmd = $ARGV[2];

# Get the input
open SOURCE, "<$sourceFile"
	or die "Could not open source file \"$sourceFile\": $!";
my @sourceLines = <SOURCE>;
close SOURCE;

# List of tokens
my %tokens = ();

# Look for the lines with the following properties:
#	The first token is "{"
#	The second token is a C-string
foreach my $line (@sourceLines)
{
	# Does the line begin with a { token?
	if (!($line =~ s/^\s*\{\s*//g))
	{
		next;
	}

	# Is the next token a C-string?
	if (substr($line, 0, 1) ne '"')
	{
		next;
	}

	# Scan to the end of the string
	# NOTE: embedded quotation marks are not handled correctly!
	my $end = index($line, '"', 1);
	if ($end == -1)
	{
		next;
	}

	# Copied over from hash_strings.rev
	if (substr($line, 1, 1) eq '\\')
	{
		next;
	}

	# Add to the list of tokens
	my $token = substr($line, 1, $end-1);
	$tokens{$token} = 1;
}

# Write the list of tokens out to a temporary file
($tempFH, $tempName) = tempfile();
($tempFH2, $tempName2) = tempfile();
print $tempFH join("\n", sort(keys %tokens));
close $tempFH;
close $tempFH2;		# Need to close because Win32 opens exclusively

# Path to the "perfect" executable
my $perfectExe = $perfectCmd;

# Execute the appropriate "perfect" executable
my $result = system("\"$perfectExe\" <\"$tempName\" >\"$tempName2\"");
die unless ($result == 0);

# Strip of any leading warning message
open $tempFH2, "<$tempName2";
my @lines = <$tempFH2>;
close $tempFH2;
unlink $tempName;
unlink $tempName2;
if (substr($lines[0], 0, 1) ne '#')
{
	splice(@lines, 0, 1);
}

# Write to the output file
open OUTPUT, ">$targetFile"
	or die "Couldn't open output file: $1";
print OUTPUT join('', @lines);
close OUTPUT;
