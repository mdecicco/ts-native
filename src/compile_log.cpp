#include <compile_log.h>
#include <parse.h>

namespace gjs {
	void compile_log::err(const std::string& text, const std::string& file, const std::string& lineText, u32 line, u32 col) {
		errors.push_back({ file, text, lineText, line, col });
	}

	void compile_log::err(const std::string& text, ast_node* node) {
		err(text, node->start.file, node->start.lineText, node->start.line, node->start.col);
	}

	void compile_log::warn(const std::string& text, const std::string& file, const std::string& lineText, u32 line, u32 col) {
		warnings.push_back({ file, text, lineText, line, col });
	}

	void compile_log::warn(const std::string& text, ast_node* node) {
		warn(text, node->start.file, node->start.lineText, node->start.line, node->start.col);
	}
};