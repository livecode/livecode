#!/usr/bin/env perl

use warnings;
use Text::ParseWords;
use Getopt::Long;

my $foundation = 0;
GetOptions ('foundation|f' => \$foundation);
my $inputFile = $ARGV[0];
my $outputFile = $ARGV[1];

my $load = '';
my $unload = '';
my $resolve = '';

# MCU_loadmodule and friends have engine dependency - for example when
# weak linking to the crypto library we rely on revsecurity living next
# to the executable, which is resolved using MCcmd. We also rely on 
# mappings from dynamic library names to paths which can be custom-
# defined as deploy parameters. Hence we need to use special simplified 
# implementations in libfoundation if we require no engine dependency.
if ($foundation == 1) {
    $load = "MCSDylibLoadModuleCString";
    $unload = "MCSDylibUnloadModule";
    $resolve = "MCSDylibResolveModuleSymbolCString";
} else {
    $load = "MCU_loadmodule";
    $unload = "MCU_unloadmodule";
    $resolve = "MCU_resolvemodulesymbol";
}

# Read all of the input data
open INPUT, "<$inputFile"
	or die "Could not open input file \"$inputFile\": $!";
my @spec = <INPUT>;
close INPUT;

# Open the output file
open OUTPUT, ">$outputFile"
	or die "Could not open output file \"$outputFile\": $!";

# Modules
my %moduleSymbols = ();
my %moduleDetails = ();
my $currentModule = undef;

# Predeclarations
sub output;
sub outputpart;
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
output ;
output "#if defined(_MACOSX) || defined(_MAC_SERVER)";
output "#include <mach-o/dyld.h>";
output "#define SYMBOL_PREFIX \"_\"";
output "#else";
output "#define SYMBOL_PREFIX";
output "#endif";

output ;
output "typedef void *module_t;";
output "typedef void *handler_t;";
output ;
outputpart "extern \"C\" void *";
outputpart $load;
output "(const char *);";
outputpart "extern \"C\" void ";
outputpart $unload;
output "(void *);";
outputpart "extern \"C\" void *";
outputpart $resolve;
output "(void *, const char *);";
output ;

output "extern \"C\"";
output "{";
output ;
output "static int module_load(const char *p_source, module_t *r_module)";
output "{";
output "  module_t t_module;";
outputpart " 	t_module = (module_t)";
outputpart $load;
output "(p_source);";
output "  if (t_module == NULL)";
output "    return 0;";
output "  *r_module = t_module;";
output "  return 1;";
output "}";
output ;

# MM-2014-02-14: [[ LibOpenSSL 1.0.1e ]] Implemented module_unload for Android.
output "static int module_unload(module_t p_module)";
output "{";
outputpart "  ";
outputpart $unload;
output "(p_module);";
output "  return 1;";
output "}";
output ;
output "static int module_resolve(module_t p_module, const char *p_name, handler_t *r_handler)";
output "{";
output "  handler_t t_handler;";
outputpart "    t_handler = ";
outputpart $resolve;
output "(p_module, p_name);";
output "  if (t_handler == NULL)";
output "    return 0;";
output "  *r_handler = t_handler;";
output "  return 1;";
output "}";
output ;

# Support code for 'could not find library' message boxes
# The order of tools for displaying messages is:
#    zenity
#    wish (part of Tk)
#    xmessage
#
# The engine is about to die anyway so this doesn't do any error checking. It
# also doesn't wait for command completion to prevent hanging waiting for the
# user to click.
#
output ;
output "#if defined(_LINUX)";
output "static void failed_required_link(const char* libname, const char* liblist)";
output "{";
output "  const char* dialog =";
output "    \"zenity --error --title \\\"\$TITLE\\\" --text \\\"\$TEXT\\\" 2>/dev/null 1>/dev/null && \"";
output "    \"echo \\\"wm state . withdrawn ; tk_messageBox -icon error -message \\\\\\\"\$TEXT\\\\\\\" -title \\\\\\\"\$TITLE\\\\\\\" -type ok ; exit \\\" | wish && \"";
output "    \"xmessage -center -buttons OK -default OK \\\"\$TITLE:\\\" \\\"\$TEXT\\\"\" ";
output "	;";
output ;
output "  char* error = new char[65536];";
output "  char* command = new char[65536];";
output ;
output "  snprintf(error, 65536, \"Failed to load library \\\'%s\\\' (tried %s)\", libname, liblist);";
output "  snprintf(command, 65536, \"TITLE=\\\"LiveCode startup error\\\" TEXT=\\\"%s\\\" /bin/sh -c \\\'%s\\\' &\", error, dialog);";
output "  fprintf(stderr, \"Fatal: failed to load library \\\'%s\\\' (tried %s)\\n\", libname, liblist);";
output "  int ignored = system(command); (void)ignored;";
output "  exit(-1);";
output "}";
output "#endif";
output ;

# Process each module
foreach my $module (keys %moduleDetails)
{
	#print STDOUT "generateModule $module\n";
	generateModule $module;
}

output "}";

close OUTPUT;

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
    output "#elif defined(_LINUX) || defined(__EMSCRIPTEN__)";
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
    output "    fprintf(stderr, \"Warning: could not load library: $unixLibrary\\n\");";
    output "#endif";
    output "return 0;";
    output "}";
    output "return -1;";
    output "}";
	
	output ;
	output "#if defined(_LINUX)";
	output "void initialise_required_weak_link_${module}(void)";
	output "{";
	output "  if (!initialise_weak_link_${module}())";
	output "  {";
	output "    failed_required_link(\"${module}\", \"${unixLibrary}\");";
	output "  }";
	output "}";
	output "#endif";
	output ;
	
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
	outputpart "$line\n"
}

sub outputpart
{
	my $line = $_[0];
	print OUTPUT "$line";
}
