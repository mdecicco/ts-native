#include <gjs/common/script_tracer.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_module.h>
#include <gjs/common/script_function.h>
#include <gjs/common/script_type.h>
#include <gjs/common/function_signature.h>
#include <gjs/util/util.h>

namespace gjs {
    script_tracer::script_tracer() : node_count(0), node_capacity(128), has_error(false) {
        nodes = new trace_node[node_capacity];
    }

    script_tracer::~script_tracer() {
        delete [] nodes;
        nodes = nullptr;
        node_capacity = node_count = 0;
    }

    void script_tracer::expand() {
        trace_node* newMem = new trace_node[node_capacity + 128];
        memcpy(newMem, nodes, node_count * sizeof(trace_node));
        delete [] nodes;
        nodes = newMem;
        node_capacity += 128;
    }

    void script_tracer::produce_error(const script_string& msg) {
        has_error = true;

        error += "Error: ";
        error += msg;

        script_context* ctx = script_context::current();

        for (u32 i = 0;i < node_count;i++) {
            error += "\n";

            script_module* mod = ctx->module(nodes[i].mod);
            script_function* fn = ctx->function(nodes[i].func);
            source_ref* ref = (source_ref*)mod->data()->data(nodes[i].ref_offset);
            error += format("In function '%s' of module '%s' at %d:%d:\n", fn->type->signature->to_string(fn->name, fn->is_method_of, nullptr, true).c_str(), ref->module.c_str(), ref->line, ref->col);
            error += "\t";
            const char* ln = ref->line_text.c_str();
            u16 wscount = 0;
            while (isspace(*ln)) {
                ln++;
                wscount++;
            }
            error += ln;
            error += "\n\t";
            for (u16 c = 0;c < ref->col - wscount;c++) error += " ";
            error += "^";
        }
    }
};