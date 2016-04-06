#!/usr/bin/env perl

use warnings;

sub trim
{
	my $trim = $_[0];
	$trim =~ s/^\s+|\s+$// ;
	return $trim;
}

my $inputFile = $ARGV[1];
my $outputFile = $ARGV[2];

# Read all of the input data
open INPUT, "<$inputFile"
	or die "Could not open input file \"$inputFile\": $!";
my @spec = <INPUT>;
close INPUT;

# Open the output file
open OUTPUT, ">$outputFile"
	or die "Could not open output file \"$outputFile\": $!";

# Prefix for symbols
my $prefix = "";
if (defined $ARGV[0] and $ARGV[0] ne "")
{
	$prefix = $ARGV[0];
}

foreach my $line (@spec)
{
	# Ignore empty lines
	if ($line =~ /^\s*$/)
	{
		next;
	}
	
	# Ignore comment lines
	if ($line =~ /^\s*#/)
	{
		next;
	}
	
	# Ignore lines defining modules (we only care about symbols)
	if (substr($line, 0, 1) ne "\t")
	{
		next;
	}
	
	# Skip the tab character
	substr($line, 0, 1) = "";
	trim($line);
	
	# Handle optional symbols
	my $symbol = undef;
	my @words = split('\s+', $line);
	if (substr($line, 0, 1) eq "?")
	{
		$symbol = $words[1];
	}
	else
	{
		$symbol = $words[0];
	}
	
	# Remove any trailing colons
	$symbol =~ s/:$// ;
	
	# Write the symbol to the output
	print OUTPUT "$prefix$symbol\n";
}
