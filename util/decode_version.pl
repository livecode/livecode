#!/usr/bin/env perl

open(FILE, $ARGV[1]) || die "Can't open input file";

my $expr = "^${ARGV[0]}[[:blank:]]*=[[:blank:]]*(.*)\$";

while (<FILE>) {
	if (/$expr/) { print $1 }
}
