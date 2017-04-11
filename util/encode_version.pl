#!/usr/bin/env perl

use warnings;

sub trim
{
	my $trimmed = $_[0];
	$trimmed =~ s/^\s+|\s+$//g ;
	return $trimmed;
}

my $gitRevision = $ARGV[0];
my $inputDir = $ARGV[1];
my $outputDir = $ARGV[2];

# Open the file containing the variables
open VARIABLES, "<" . $inputDir . "/../version"
	or die "Could not open version information: $!";
my @variables = <VARIABLES>;
close VARIABLES;

# Open the template header
open TEMPLATE, "<" . $inputDir . "/include/revbuild.h.in"
	or die "Could not open template header: $!";
my @template = <TEMPLATE>;
close TEMPLATE;

# Flatten the input
my $output = join('', @template);

# Replace each instance of the variable in the template file
foreach $variable (@variables)
{
	my @parts = split('=', $variable);
	my $varName = trim($parts[0]);
	my $varValue = trim($parts[1]);
	$output =~ s/\$${varName}/${varValue}/g;
}

$output =~ s/\$GIT_REVISION/${gitRevision}/g;

# Create the output file
open OUTPUT, ">" . $outputDir . "/include/revbuild.h"
	or die "Could not open output file: $!";
print OUTPUT $output;
close OUTPUT;
