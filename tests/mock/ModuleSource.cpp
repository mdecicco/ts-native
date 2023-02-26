#include <mock/ModuleSource.h>
#include <mock/ScriptMeta.h>

tsn::ModuleSource* mock_module_source(const char* code) {
    tsn::script_metadata* meta = mock_script_metadata();
    meta->size = strlen(code);

    return new tsn::ModuleSource(code, meta);
}

void delete_mocked_source(tsn::ModuleSource* src) {
    tsn::script_metadata* meta = const_cast<tsn::script_metadata*>(src->getMeta());
    delete src;
    delete meta;
}