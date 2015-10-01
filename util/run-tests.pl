#!/usr/bin/env perl

use File::Basename;

$test_wrapper = $ARGV[0];
$test_exec = $ARGV[1];
$test_dir = dirname($test_exec);

chdir $test_dir or die "Failed to change directory!";
print "$test_wrapper $test_exec\n";
exec "$test_wrapper $test_exec";
