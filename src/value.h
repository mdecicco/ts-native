#pragma once
#include <types.h>
#include <string.h>
#include <string>

namespace gjs {
	const u8 value_type_bits = 3;
	enum class value_type {
		undefined,
		integer,
		decimal,
		string,
		boolean
	};

	const u8 value_flag_bits = 4;
	enum class value_flag {
		is_null			= 0b0001,
		is_external		= 0b0010,
		is_read_only	= 0b0100,
		is_allocated	= 0b1000
	};

	typedef u8 value_flags;
	const u8 value_type_shift = 0;
	const value_flags value_type_mask = 0b00000111;

	const u8 value_flags_shift = value_type_bits;
	const value_flags value_flags_mask = 0b00000111;

	#define set_val(val, typ, to) \
		{ \
			value_flags mask = (value_flags(value_type::typ) << value_type_shift);	\
			val.f = (val.f & mask) | mask;	\
			val.v.i = (to); \
		}


	struct value {
		FORCE_INLINE value() {
			memset(&f, 0, sizeof(value_flags));
			memset(&v, 0, sizeof(v));
		}
		FORCE_INLINE value(const value& val) {
			f = val.f;
			v = val.v;
		}
		FORCE_INLINE value(integer val) {
			memset(&f, 0, sizeof(value_flags));
			memset(&v, 0, sizeof(v));
			set(val);
		}
		FORCE_INLINE value(decimal val) {
			memset(&f, 0, sizeof(value_flags));
			memset(&v, 0, sizeof(v));
			set(val);
		}
		FORCE_INLINE value(char* val) {
			memset(&f, 0, sizeof(value_flags));
			memset(&v, 0, sizeof(v));
			set(val);
		}
		FORCE_INLINE value(bool val) {
			memset(&f, 0, sizeof(value_flags));
			memset(&v, 0, sizeof(v));
			set(val);
		}
		FORCE_INLINE ~value() { }

		FORCE_INLINE void set(const value& val) {
			f = val.f;
			v = val.v;
		}
		FORCE_INLINE void set(integer val) {
			value_flags mask = (value_flags(value_type::integer) << value_type_shift);
			f = (f & mask) | mask;
			v.i = val;
		}
		FORCE_INLINE void set(decimal val) {
			value_flags mask = (value_flags(value_type::decimal) << value_type_shift);
			f = (f & mask) | mask;
			v.d = val;
		}
		FORCE_INLINE void set(char* val) {
			value_flags mask = (value_flags(value_type::string) << value_type_shift);
			f = (f & mask) | mask;
			v.s = val;
		}
		FORCE_INLINE void set(bool val) {
			value_flags mask = (value_flags(value_type::boolean) << value_type_shift);
			f = (f & mask) | mask;
			v.b = val;
		}

		FORCE_INLINE void operator=(const value& val) {
			f = val.f;
			v = val.v;
		}
		FORCE_INLINE void operator=(integer val) {
			value_flags mask = (value_flags(value_type::integer) << value_type_shift);
			f = (f & mask) | mask;
			v.i = val;
		}
		FORCE_INLINE void operator=(decimal val) {
			value_flags mask = (value_flags(value_type::decimal) << value_type_shift);
			f = (f & mask) | mask;
			v.d = val;
		}
		FORCE_INLINE void operator=(char* val) {
			value_flags mask = (value_flags(value_type::string) << value_type_shift);
			f = (f & mask) | mask;
			v.s = val;
		}
		FORCE_INLINE void operator=(bool val) {
			value_flags mask = (value_flags(value_type::boolean) << value_type_shift);
			f = (f & mask) | mask;
			v.b = val;
		}

		FORCE_INLINE value_type	type	() const { return (value_type)((f >> value_type_shift) & value_type_mask); }
		FORCE_INLINE bool is_read_only	() const { return (f >> value_flags_shift) & (value_flags)value_flag::is_read_only; }
		FORCE_INLINE bool is_null		() const { return (f >> value_flags_shift) & (value_flags)value_flag::is_null; }
		FORCE_INLINE bool is_allocated	() const { return (f >> value_flags_shift) & (value_flags)value_flag::is_allocated; }
		FORCE_INLINE bool is_external	() const { return (f >> value_flags_shift) & (value_flags)value_flag::is_external; }
		FORCE_INLINE bool is_undefined	() const { return ((f >> value_type_shift) & value_type_mask) == 0; }
		FORCE_INLINE void set_flag(value_flag flag, bool value) { f ^= (-value ^ f) & (value_flags(flag) << value_flags_shift); }
		
		FORCE_INLINE operator integer&	() { return v.i; }
		FORCE_INLINE operator decimal&	() { return v.d; }
		FORCE_INLINE operator integer	() const { return v.i; }
		FORCE_INLINE operator decimal	() const { return v.d; }
		FORCE_INLINE operator char*		() const { return (is_null() || is_undefined()) ? nullptr : v.s; }
		FORCE_INLINE operator bool		() const { return v.b && !is_null() && !is_undefined(); }

		std::string to_string() const;

		value_flags f;
		union {
			integer i;
			decimal d;
			bool	b;
			char*	s;
		} v;
	};
};