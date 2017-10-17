#!/usr/bin/env perl

use File::Basename;

$test_wrapper = $ARGV[0];
$test_exec = $ARGV[1];
$test_dir = dirname($test_exec);
$test_log = $ARGV[2];

# Use an interpreter, if specified.
if ($test_wrapper eq "" && $ENV{"INTERPRETER"} ne "") {
	$test_wrapper = $ENV{"INTERPRETER"};
}

$test_command_line = "$test_wrapper $test_exec --tap=$test_log";

chdir $test_dir or die "Failed to change directory!";
print "$test_command_line\n";
exec "$test_command_line" or die "Failed to run tests!";
