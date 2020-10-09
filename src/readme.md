## A disorganized collection of useful information

### Script Context
The `script_context` class is a container for everything related to the compilation and execution of scripts. Multiple script contexts can exist concurrently and will not interfere with each other on their own.

A script context can not be used meaningfully without having a _backend_.

### Backends
A _backend_ is a class that inherits from the  `backend` class (`gjs/backend/backend.h`) and defines the interface for the _pipeline_ to generate code for some target architecture.

The backend has 4 pure virtual methods which must be overridden:
- `void generate(compilation_output& in)`:
Generates code for the target architecture with the provided compilation output.
- `u16 gp_count() const`
Returns the number of available general purpose registers of the target architecture
- `u16 fp_count() const`
Returns the number of available floating point registers of the target architecture
- `void call(script_function* func, void* ret, void** args)`
Passes the arguments stored in `args` to the function `func`, executes the function, then stores the result in `ret`.  The details of how to write to `ret` and read from `args` will be covered in another section.

Currently there is only one backend implemented, the `vm_backend` which compiles all code to bytecode for gjs's custom VM.
The VM backend is available in `gjs/backends/vm_backend.h` and aims to help with debugging script code, or executing code on architectures which don't have a corresponding backend.

The implementation details of backends which generate machine code for actual hardware is yet to be determined. Most likely, they will have to bind entry points of functions to the corresponding `script_function` objects. Though, since the backend class has a `call` method, it would also be possible to store entry points in a map on the backend class and look them up from the `call` method.

### Creating a VM context
The code for setting up a VM context is simple:
```
#include <gjs.h>

int main(int argc, const char** args) {
	basic_malloc_allocator alloc;
	u32 stack_size = 4096; // measured in bytes
	u32 mem_size = 4096;   // ^
	vm_backend gen(&alloc, stack_size, mem_size);
	script_context ctx(&gen);

	// add code, do fun stuff

	return 0;
}
```

The `vm_backend` uses an instance of a `vm_allocator` interface for dynamic allocations and deallocations that happen within scripts. The stack size and memory size must also be provided.

The `stack_size` parameter defines how large the stack can grow before throwing a stack overflow exception. The stack is used to store local objects which were not dynamically allocated, register values that must not be changed during function calls, and occasionally primitive variables that could not fit in the registers (can happen if there are more active variables than the VM has registers).

The `mem_size` parameter defines how much memory can be dynamically allocated by all scripts before a runtime exception is thrown.

Currently, the VM context is created by default and managed internally if `nullptr` is passed to the `backend` parameter of `script_context`.

### Compiling scripts
To compile scripts, you just need to provide the filename and source code to the context via `ctx.add_code(const std::string& filename, const std::string& source)`. Here's an example:

```
script_module* mod = ctx.add_code(
    "test.gjs",
    "string world = 'world!';\n"
    "void func() { print('Hello, ' + world); }\n"
);
if (mod) {
    mod->init();
    ctx.call<void>(mod->function("func"), nullptr);
} else {
	print_log(&ctx);
}
```

The `add_code` function will return a `script_module` object on success which must be initialized with `mod->init()` if there is any code in the global scope of the script. Global code is added to an implicit function named `__init__`, which takes no parameters and adopts the return type of the first encountered `return` statement in the global scope. The call to `init` may be made automatic in the future. Modules will be covered more in a later section.

### Executing script functions
The execution of script functions is somewhat similar to calling native functions. The differences are that you have to go through the `script_context`, pass a pointer to the function, and the return value is always written to an output parameter. Also templates are somewhat heavily involved, though as much out of view as possible.

First, the function to call must be obtained from the module which it belongs to. There are a few ways of doing this:
```
// This is the easiest way, but it's only valid if the function is not overloaded
script_function* func = mod->function("function_name");

// If you know the return type and argument types (if any) of a specific overload, this is the way
// Note: You can't get overloaded functions with this method if any of the involved types are pure
//       script types that have no bound or mirrored C++ types
script_function* func = mod->function<ret_type, arg_types...>("function_name");

// If you'd prefer to programmatically determine which overload to use for some reason, there's this
std::vector<script_function*> funcs = mod->function_overloads("function_name");

// You also have access to all of the functions in the module (except for the __init__ function) with
const std::vector<script_function*>& funcs = mod->functions();

// If you want to get the __init__ function for some reason, you can do this
script_function* init = mod->function("__init__");
```

Then, to call the script function:
```
// no return (void return type must be specified)
ctx.call<void>(func, nullptr);

// float return (return type determined automatically)
float result;
ctx.call(func, &result);

// arguments are also automatically determined and can be passed like a regular function call!
ctx.call(multiply_floats, &result, 61.0f, 69.0f);
```

**Note:** the `call` method will throw an exception if one of the following happens:
- Return type doesn't match the function being called
- Provided argument count doesn't match the number of arguments of the function being called
- Any argument type does not match the corresponding argument type of the function being called
- No output variable is specified for a function that returns a value
