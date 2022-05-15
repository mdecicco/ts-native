#include <utils/robin_hood.h>
#include <malloc.h>
#include <memory.h>

namespace robin_hood {
	void* rh_malloc(size_t size) {
		return malloc(size);
	}

	void* rh_calloc(size_t count, size_t size) {
		void* mem = calloc(count, size);
		if (mem) memset(mem, 0, count * size);
		return mem;
	}

	void rh_free(void* ptr) {
		free(ptr);
	}
};