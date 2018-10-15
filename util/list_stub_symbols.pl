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
my $exportDefOpt = $ARGV[3];

my ($exportDefArg, $exportDefModule) = split(/=/, $exportDefOpt);
my $isWindows = $exportDefArg eq "--exportdef";
my $isLinux = $exportDefArg eq "--version-script";

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

# Output header for windows
if ($isWindows)
{
	print OUTPUT "LIBRARY $exportDefModule\n";
	print OUTPUT "EXPORTS\n";
}
elsif ($isLinux)
{
	print OUTPUT "$exportDefModule {\n";
	print OUTPUT "  global:\n";
}

# Symbol index counter
my $symbolIndex = 1;

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
	if ($isWindows)
	{
		print OUTPUT "    $prefix$symbol    \@$symbolIndex\n";
	}
	elsif ($isLinux)
	{
		print OUTPUT "    $prefix$symbol;\n";
	}
	else
	{
		print OUTPUT "$prefix$symbol\n";
	}

	$symbolIndex += 1;
}

if ($isLinux)
{
	print OUTPUT "  local: *;\n";
	print OUTPUT "};\n";
}
