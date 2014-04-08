#!/usr/bin/env perl

use warnings;

sub trim
{
	my $trim = $_[0];
	$trim =~ s/^\s+|\s+$// ;
	return $trim;
}

# Predeclarations
sub output;

# Parameters
my $name = $ARGV[0];

# Read in from stdin
my @spec = <STDIN>;

# Prefix for symbols
my $prefix = "";
if (defined $ARGV[0] and $ARGV[0] ne "")
{
	$prefix = $ARGV[0];
}

# Output variables
my $symbolExterns = "";
my $symbolEntries = "";

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
	
	$symbolExterns .= "extern \"C\" void *$symbol;\n";
	$symbolEntries .= "    { \"$symbol\", $symbol },\n";
}

print STDOUT $symbolExterns;
output "struct LibExport { const char *name; void *address; };";
output "struct LibInfo { const char **name; struct LibExport *exports; };";
output "static const char *__libexternalname = \"$name\";";
output "static struct LibExport __libexports[] =";
output "{";
print STDOUT $symbolEntries;
output "  { 0, 0 }";
output "};";
output "static struct LibInfo __libinfo=";
output "{";
output "    &__libexternalname,";
output "    __libexports";
output "};";
output "LibInfo *__libinfoptr_$name = &__libinfo;";

sub output
{
	my $line = $_[0];
	if (@_ != 1)
	{
		$line = "";
	}
	
	print STDOUT "$line\n";
}
