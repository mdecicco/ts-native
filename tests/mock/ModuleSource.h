#pragma once
#include <tsn/utils/ModuleSource.h>

tsn::ModuleSource* mock_module_source(const char* code);
void delete_mocked_source(tsn::ModuleSource* src);