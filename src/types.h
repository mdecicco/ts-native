#pragma once
#include <stdint.h>

#define FORCE_INLINE __forceinline

namespace gjs {
	typedef uint64_t	u64;
	typedef int64_t		i64;
	typedef uint32_t	u32;
	typedef int32_t		i32;
	typedef uint16_t	u16;
	typedef uint8_t		u8;

	typedef uint64_t	instruction;
	typedef uint32_t	address;
	typedef float		decimal;
	typedef int32_t		integer;
	typedef int16_t		memory_offset;
};
