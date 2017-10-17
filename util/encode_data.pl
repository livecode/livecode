#!/usr/bin/env perl

use warnings;

# Incoming arguments
my $sourceFile	= $ARGV[0];
my $destFile	= $ARGV[1];
my $varName		= $ARGV[2];

# Read in the source file
open INPUT, "<$sourceFile"
	or die "Could not open source file \"$sourceFile\": $!";
binmode INPUT;
my $input = do { local( $/ ) ; <INPUT> } ;
close INPUT;
	
# Split into bytes
my @bytes = unpack('C*', $input);

# Convert to a list of uint8_t values appropriate for an array in C
my $cArray = "";
my $index;
my $length = scalar @bytes;
for ($index = 0; $index < $length; $index++)
{
	$cArray .= sprintf("0x%02x, ", $bytes[$index]);
	if ($index % 16 == 15)
	{
		$cArray .= "\n\t";
	}
}

# Tidy up the output
if ($index % 16 == 0)
{
	substr($cArray, -4) = "";
}
else
{
	substr($cArray, -2) = "";
}

# Write to the output file
open OUTPUT, ">$destFile"
	or die "Could not open output file \"$destFile\": $!";
print OUTPUT "alignas(16) unsigned char ${varName}[] = \n{\n\t$cArray\n};\n";
print OUTPUT "unsigned int ${varName}_length = $length;\n";
close OUTPUT;
