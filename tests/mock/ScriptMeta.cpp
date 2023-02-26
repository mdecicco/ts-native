#include <mock/ScriptMeta.h>

tsn::script_metadata* mock_script_metadata() {
    tsn::script_metadata* meta = new tsn::script_metadata;

    meta->path = "./test.tsn";
    meta->module_id = 1;
    meta->size = 1;
    meta->modified_on = 1;
    meta->cached_on = 2;
    meta->is_trusted = false;
    meta->is_external = false;

    return meta;
};