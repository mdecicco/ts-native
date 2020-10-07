## Script Context
The `script_context` class is a container for everything related to the compilation and execution of scripts. Multiple script contexts can exist concurrently and will not interfere with each other on their own.

A script context can not be used without having a _backend_ provided.

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
    ctx.call<void>(ctx.function("func"), nullptr);
} else {
	// do something with error log (ctx.compiler()->log())
}
```

The `add_code` function will return a `script_module` object on success which must be initialized with `mod->init()` if there is any code in the global scope of the script. Global code is added to an implicit function named `__init__`, which takes no parameters and adopts the return type of the first encountered `return` statement in the global scope. The call to `init` may be made automatic in the future. Modules will be covered more in a later section.

(work in progress)
