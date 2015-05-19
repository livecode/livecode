#include <map>
#include <vector>
#include <assert.h>
#include <iostream>
#include <execinfo.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <cxxabi.h>
#include <malloc.h>

void initialize_mem_dbg();
void report_mem_dbg();

void set_handlers();
void restore_handlers();

static void *my_malloc_hook(size_t size, const void *caller);
static void *my_realloc_hook(void *ptr, size_t size, const void *caller);
static void *my_memalign_hook(size_t alignment, size_t size, const void *caller);
static void my_free_hook(void *ptr, const void *caller);

static void *(*old_malloc_hook)(size_t size, const void *caller);
static void *(*old_realloc_hook)(void *ptr, size_t size, const void *caller);
static void *(*old_memalign_hook)(size_t alignment, size_t size, const void *caller);
static void (*old_free_hook)(void *ptr, const void *caller);


static void __MarkFreed(void* x);
static void __MarkAllocated(void* x, size_t size);


//_____________________________________________________________________________
//


void initialize_mem_dbg()
{
	old_malloc_hook   = __malloc_hook;
	old_realloc_hook  = __realloc_hook;
	old_memalign_hook = __memalign_hook;
	old_free_hook     = __free_hook;

	set_handlers();
}

void set_handlers()
{
	__malloc_hook   = my_malloc_hook;
	__realloc_hook  = my_realloc_hook;
	__memalign_hook = my_memalign_hook;
	__free_hook     = my_free_hook;
}


void restore_handlers()
{
	__malloc_hook   = old_malloc_hook;
	__realloc_hook  = old_realloc_hook;
	__memalign_hook = old_memalign_hook;
	__free_hook     = old_free_hook;
}

static void* my_malloc_hook (size_t size, const void *caller)
{
	restore_handlers();
	void* result = malloc(size);
	__MarkAllocated(result, size);
	set_handlers();
	return result;
}

static void my_free_hook (void *ptr, const void *caller)
{
	restore_handlers();
	__MarkFreed(ptr);
	free(ptr);
	set_handlers();
}

static void* my_realloc_hook(void* ptr, size_t size, const void* caller)
{
	restore_handlers();
	__MarkFreed(ptr);
	void* result = realloc(ptr, size);
	__MarkAllocated(result, size);
	set_handlers();
	return result;
}


static void* my_memalign_hook(size_t alignment, size_t size, const void* caller)
{
	restore_handlers();
	void* result = memalign(alignment, size);
	__MarkAllocated(result, size);
	set_handlers();
	return result;
}




//_____________________________________________________________________________
//

typedef std::vector<void*> Backtrace;

static std::map<void*, size_t > allocated_size;
static std::map<void*, Backtrace> allocated_backtrace;

static std::map<size_t, long int> size_counts;
static std::map<Backtrace, long int> backtrace_counts;
static std::map<std::pair<size_t, Backtrace>, long int> size_backtrace_counts;

void __MarkAllocated(void* x, size_t size)
{
	// Get the current call stack
	void* buffer[500];
	int bt_size = backtrace(buffer, 500);
	Backtrace bt(buffer, buffer + bt_size - 1);

	allocated_size.insert({x, size});
	allocated_backtrace.insert({x, bt});

	size_counts[size]++;
	backtrace_counts[bt]++;
	size_backtrace_counts[std::make_pair(size, bt)]++;
}


void __MarkFreed(void* x) 
{
	if (x) {
		assert(allocated_size.count(x) == 1);

		// update counters
		size_counts[allocated_size[x]]--;
		backtrace_counts[allocated_backtrace[x]]--;
		size_backtrace_counts[std::make_pair(allocated_size[x], allocated_backtrace[x])]--;
		
		// remove from allocated lists
		allocated_size.erase(x);
		allocated_backtrace.erase(x);
	}
}



std::ostream& operator<<(std::ostream& out, Backtrace& bt)
{
	char** symbols = backtrace_symbols(&bt[0], bt.size());
	
	for (int i = 0; i < bt.size(); i++) {

		char* start;
		char* end;
		for (start = symbols[i]; *start != '('; ++start) { }
		start++;
		for (end = start; *end != '+' && *end != ')'; ++end) { }

		std::string first(symbols[i], start);
		std::string mangled(start, end);
		std::string rest(end);

		std::string demangled;
		
		int status = 0;
		char* demangled_cstr = abi::__cxa_demangle(mangled.c_str(), 0, 0, &status);
		if (status == 0) {
			demangled = std::string(demangled_cstr);
		} else {
			demangled = mangled;
		}
		
		out << "\t\t"
			<< first
			<< demangled
			<< rest
			<< std::endl;

		free(demangled_cstr);
	}
	out << std::endl;
	
	free(symbols);
	
	return out;
}


template<typename X, typename Y>
void print_reverse(std::map<X,Y>& map, int num = -1)
{
	std::multimap<Y, X> inverse;
	
	for (auto it = map.begin(); it != map.end(); ++it) {
		inverse.insert({it->second, it->first});
	}
	
	for (auto it = inverse.rbegin(); (num != 0) && it != inverse.rend(); ++it, --num) {
		std::cerr << it->first << "\t" << it->second << std::endl;
	}
}


void report_mem_dbg()
{
	restore_handlers();
	{

		std::cerr << "size\tcount" << std::endl;
		for (auto it = size_counts.begin(); it != size_counts.end(); ++it) {
			if (it->second != 0) {
				std::cerr << it->first << "\t" << it->second << std::endl;
			}
		}
		std::cerr << std::endl;

		std::cerr << "count\tsize" << std::endl;
		print_reverse(size_counts, 20);
		std::cerr << std::endl;
 
		print_reverse(backtrace_counts, 20);


		std::multimap<long int, std::pair<size_t, Backtrace>> inverse;
	
		for (auto it = size_backtrace_counts.begin(); it != size_backtrace_counts.end(); ++it) {
			inverse.insert({it->second, it->first});
		}

		int i = 20;
		for (auto it = inverse.rbegin(); (i != 0); it++, i--) {
			std::cerr << it->first << "\t" << it->second.first << std::endl;
			std::cerr << it->second.second << std::endl;
		}
		
	}
	set_handlers();
}

