#pragma once
#include <string>
#include <vector>
#include <types.h>
#include <source_ref.h>

namespace gjs {
	namespace error { enum class ecode; };
	namespace warning { enum class wcode; };

	struct compile_message {
		bool is_error;
		union {
			error::ecode e;
			warning::wcode w;
		} code;
		std::string text;
		source_ref src;
	};

	class compile_log {
		public:
			compile_log();
			~compile_log();

			void err(error::ecode code, source_ref at, ...);
			void warn(warning::wcode code, source_ref at, ...);

			std::vector<compile_message> errors;
			std::vector<compile_message> warnings;
	};
};

