#!/usr/bin/env perl

use warnings;
use Text::ParseWords;

# Read all the data available on stdin
my @spec = <STDIN>;

# Modules
my %moduleSymbols = ();
my %moduleDetails = ();
my $currentModule = undef;

# Predeclarations
sub output;
sub generateModule;
sub symbolIsReference;
sub typeListToProto;
sub symbolName;
sub symbolInputs;
sub symbolOutputs;
sub symbolIsOptional;

sub trim
{
	my $trim = $_[0];
	$trim =~ s/^\s+|\s+$//g ;
	return $trim;
}

# Repeat for each line in the input
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
	
	# Lines not beginning with tabs define modules
	if (substr($line, 0, 1) ne "\t")
	{
		# Divide the string into the first word and the rest
		$line =~ /^([^\s]*)(.*$)/ ;
		my $module = trim($1);
		my $details = trim($2);
		$currentModule = $module;
		$moduleDetails{$module} = $details;
		#print STDOUT "Module: $module ($details)\n";
	}
	else
	{
		# Line defines a symbol for the current module
		$line =~ s/^\s+|\s+$// ;
		$moduleSymbols{$currentModule} .= $line;
		#print STDOUT "\tSymbol: $line";
	}
}

output "#include <stdlib.h>";
output "#include <stdio.h>";
output "#include <cstring>";
output "#if defined(_MACOSX) || defined(_MAC_SERVER)";
output "#define SYMBOL_PREFIX \"_\"";
output "#else";
output "#define SYMBOL_PREFIX";
output "#endif";
output ;
output "typedef void *module_t;";
output "typedef void *handler_t;";
output ;
output "extern \"C\" void *MCU_loadmodule(const char *);";
output "extern \"C\" void MCU_unloadmodule(void *);";
output "extern \"C\" void *MCU_resolvemodulesymbol(void *, const char *);";
output ;
output "extern \"C\"";
output "{";
output ;
output "static int module_load(const char *p_source, module_t *r_module)";
output "{";
output "  module_t t_module;";
output " 	t_module = (module_t)MCU_loadmodule(p_source);";
output "  if (t_module == NULL)";
output "    return 0;";
output "  *r_module = t_module;";
output "  return 1;";
output "}";
output ;

# MM-2014-02-14: [[ LibOpenSSL 1.0.1e ]] Implemented module_unload for Android.
output "static int module_unload(module_t p_module)";
output "{";
output "  MCU_unloadmodule(p_module);";
output "  return 1;";
output "}";
output ;
output "static int module_resolve(module_t p_module, const char *p_name, handler_t *r_handler)";
output "{";
output "  handler_t t_handler;";
output "    t_handler = MCU_resolvemodulesymbol(p_module, p_name);";
output "  if (t_handler == NULL)";
output "    return 0;";
output "  *r_handler = t_handler;";
output "  return 1;";
output "}";
output ;

# Process each module
foreach my $module (keys %moduleDetails)
{
	#print STDOUT "generateModule $module\n";
	generateModule $module;
}

output "}";


sub generateModule
{
	my $module = $_[0];
	
	my @libraries = shellwords($moduleDetails{$module});
	my $unixLibrary = $libraries[0];
	my $darwinLibrary = $libraries[1];
	my $windowsLibrary = $libraries[2];
	
	if (@libraries < 2)
	{
		$darwinLibrary = "";
	}
	if (@libraries < 3)
	{
		$windowsLibrary = "";
	}
	
	my @symbols = split("\n", $moduleSymbols{$module});
	
	foreach my $symbol (@symbols)
	{
		if (!symbolIsReference($symbol))
		{
			output "typedef " . typeListToProto(symbolOutputs($symbol), 0) . "(*" . symbolName($symbol) . "_t)(" . typeListToProto(symbolInputs($symbol), 1) . ");";
			output symbolName($symbol) . "_t " . symbolName($symbol) . "_ptr = NULL;";
		}
		else
		{
			output "void *" . symbolName($symbol) . "_ptr = NULL;"
		}
	}
	
	my $moduleUpper = uc $module;
	output ;
    output "#if defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)";
    output "#define MODULE_${moduleUpper}_NAME \"$darwinLibrary\"";
    output "#elif defined(_LINUX)";
    output "#define MODULE_${moduleUpper}_NAME \"$unixLibrary\"";
    # MM-2014-02-10: [[ LibOpenSSL 1.0.1e ]] Prefix android modules with lib.
    output "#elif defined(TARGET_SUBPLATFORM_ANDROID)";
    output "#define MODULE_${moduleUpper}_NAME \"lib$unixLibrary\"";
    output "#elif defined(_WINDOWS)";
    output "#define MODULE_${moduleUpper}_NAME \"$windowsLibrary\"";
    output "#endif";
    output ;
    output "static module_t module_$module = NULL;";
    output ;

    # MM-2014-02-14: [[ LibOpenSSL 1.0.1e ]] Added finalise function and made sure staid variables are initialised to NULL.
    output "void finalise_weak_link_$module(void)";
    output "{";
    output "  module_unload(module_$module);";
    output "  module_$module = NULL;";
    output "}";
    output ;
    output "int initialise_weak_link_${module}_with_path(const char *p_path)";
    output "{";
    output "  module_$module = NULL;";
	
    output "  if (!module_load(p_path, &module_$module))";
    output "  {";
    output "  #ifdef _DEBUG";
    output "    fprintf(stderr, \"Unable to load library: $unixLibrary\\n\");";
    output "  #endif";
    output "    goto err;";
    output "  }";
	
	foreach my $symbol (@symbols)
	{
		if (symbolIsOptional($symbol))
		{
			output "module_resolve(module_$module, SYMBOL_PREFIX \"" . symbolName($symbol) . "\", (handler_t *)&" . symbolName($symbol) . "_ptr);";
		}
		else
		{
	       	output "  if (!module_resolve(module_$module, SYMBOL_PREFIX \"" . symbolName($symbol) . "\", (handler_t *)&" . symbolName($symbol) . "_ptr))";
		   	output "{";
	        output "#ifdef _DEBUG";
	        output "fprintf(stderr, \"Unable to load: " . symbolName($symbol) . "\\n\");";
	        output "#endif";
	        output "goto err; ";
	        output "}";
		}
	}
	
    output ;
    output "  return 1;";
    output ;
    output "err:";
    output "  if (module_$module != NULL)";
    output "    module_unload(module_$module);";
    output ;
    output "  return 0;";
    output "}";
    output ;

    output "int initialise_weak_link_${module}(void)";
    output "{";

	# The list of libraries for Linux may be comma-separated
	output "#if defined(_LINUX)";
	foreach my $item (split(',', $unixLibrary))
	{
		output "  if(!initialise_weak_link_${module}_with_path(\"$item\"))";
	}
	
	output "#else";
        output "  if (!initialise_weak_link_${module}_with_path(MODULE_${moduleUpper}_NAME))";
	output "#endif";
	
    output "{";
    output "#ifdef _DEBUG";
    output "    fprintf(stderr, \"Unable to load library: $unixLibrary\\n\");";
    output "#endif";
    output "return 0;";
    output "}";
    output "return -1;";
    output "}";
	
	foreach my $symbol (@symbols)
	{
		if (symbolIsReference($symbol))
		{
			next;
		}
		
		my $outputs = typeListToProto(symbolOutputs($symbol), 0);
		my $inputs = typeListToProto(symbolInputs($symbol), 1);
		my $args = "";
		
		if ($inputs ne "void")
		{
			my @inputList = split(',', $inputs);
			for (my $index = 0; $index < scalar @inputList; $index++)
			{
				$args .= "pArg${index}, ";
			}
			substr($args, -2) = "";
		}
		
		output "$outputs" . symbolName($symbol) . "($inputs)";
		output "{";
		if (symbolName($symbol) =~ /gdk_(?!pixbuf)/)
		{
			#output "  fprintf(stderr, \"" . symbolName($symbol) . "\\n\");";
		}
		if ($outputs ne "void ")
		{
			output "  return " . symbolName($symbol) . "_ptr($args);";
		}
		else
		{
			output "  " . symbolName($symbol) . "_ptr($args);";
		}
		output "}";
		output ;
	}
}

sub symbolName
{
	my $symbol = $_[0];
	
	my @items = split(':', $symbol);
	my @words = split('\s+', $items[0]);
	
	if ($words[0] eq '?' or $words[0] eq '@')
	{
		return $words[1];
	}
	
	return $items[0];
}

sub symbolIsOptional
{
	my $symbol = $_[0];
	
	return substr($symbol, 0, 1) eq '?';
}

sub symbolIsReference
{
	my $symbol = $_[0];
	
	return substr($symbol, 0, 1) eq '@';
}

sub symbolInputs
{
	my $symbol = $_[0];
	
	my @items = split(':', $symbol);
	my $spec = $items[1];
	
	if (scalar @items < 2)
	{
		$spec = "() -> ()";
	}
	
	my $inputs = substr($spec, 0, index($spec, "->"));
	
	# Remove leading/trailing whitespace and parentheses
	$inputs = trim($inputs);
	$inputs = substr($inputs, 1, -1);
	
	@items = split(',', $inputs);
	my $result = "";
	foreach my $item (@items)
	{
		$result .= trim($item) . "\n";
	}
	
	return $result;
}

sub symbolOutputs
{
	my $symbol = $_[0];
	
	my @items = split(':', $symbol);
	my $spec = $items[1];
	
	if (@items < 2)
	{
		$spec = "() -> ()";
	}
	
	my $outputs = substr($spec, index($spec, "->") + 3);
	
	# Remove leading/trailing whitespace and parentheses
	$outputs = trim($outputs);
	$outputs = substr($outputs, 1, -1);
	
	@items = split(',', $outputs);
	my $result = "";
	foreach my $item (@items)
	{
		$result .= trim($item) . "\n";
	}
	
	return $result;
}

sub typeListToProto
{
	my $list = $_[0];
	my $withArgs = $_[1];
	
	my $proto = "";
	my $index = 0;
	
	my @lines = split("\n", $list);
	foreach my $line (@lines)
	{
		if ($line eq "pointer")
		{
			$proto .= "void *";
		}
		elsif ($line eq "integer")
		{
			$proto .= "int ";
		}
		elsif ($line eq "double")
		{
			$proto .= "double ";
		}
		elsif ($line eq "integer64")
		{
			$proto .= "long long int ";
		}
		else
		{
			die "Unknown type specified: $line";
		}
		
		if ($withArgs)
		{
			$proto .= "pArg$index";
		}
		
		$proto .= ", ";
		$index++;
	}
	
	substr($proto, -2) = "";
	
	if ($proto eq "")
	{
		$proto = "void";
		if (!$withArgs)
		{
			$proto .= " ";
		}
	}
	
	return $proto;
}

sub output
{
	my $line = $_[0];
	if (@_ != 1)
	{
		$line = "";
	}
	
	print STDOUT "$line\n";
}
