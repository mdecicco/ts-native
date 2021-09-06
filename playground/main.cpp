#include <gjs/gjs.h>
#include <gjs/builtin/script_vec2.h>
#include <gjs/builtin/script_vec3.h>
#include <stdio.h>
#include <time.h>

#include "ui.h"

using namespace gjs;
using namespace math;

static const vec2<f32> arrow[5]  = {
    vec2<f32>( 0.0f , -0.4f ),
    vec2<f32>( 0.25f,  0.4f ),
    vec2<f32>( 0.0f ,  0.25f),
    vec2<f32>(-0.25f,  0.4f ),
    vec2<f32>( 0.0f , -0.4f )
};

typedef struct boid {
    vec2<f32> facing;
    vec2<f32> position;
};
static f32 bscale = 25.0f;
void draw_boid(const boid& b) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    mat3<f32> orientation = mat3<f32>::compose2D(r2d<f32>(atan2f(b.facing.y, b.facing.x)), vec2<f32>(bscale, bscale), b.position);
    vec2<f32> points[5] = {
        orientation * arrow[0],
        orientation * arrow[1],
        orientation * arrow[2],
        orientation * arrow[3],
        orientation * arrow[4]
    };

    dl->AddPolyline((ImVec2*)points, 5, ImColor(255, 255, 255), ImDrawFlags_None, 1.0f);
}

clock_t frameTimer;
void begin() {
    frameTimer = clock();
    begin_frame();
    i32 w, h;
    window_size(w, h);
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
    ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
}

void end() {
    clock_t ct = clock();
    f64 dt = f64(ct - frameTimer) / CLOCKS_PER_SEC;
    ImGui::Text("Frame Time: %.2fms", dt * 1000.0);
    ImGui::Text("FPS: %.2f", 1.0 / dt);
    ImGui::End();
    end_frame(0.25f, 0.25f, 0.25f, 1.0f);
}

// forgot to implement the ! operator...
bool dont_exit() { return !do_exit(); }

int main(int arg_count, const char** args) {
    if (!init_ui()) {
        return 1;
    }

    basic_malloc_allocator alloc;
    vm_backend be(&alloc, 8 * 1024 * 1024, 8 * 1024 * 1024);
    script_context ctx(&be);

    ctx.bind(dont_exit, "dont_exit");
    ctx.bind(begin, "begin_frame");
    ctx.bind(end, "end_frame");
    ctx.bind<boid>("boid").prop("facing", &boid::facing).prop("pos", &boid::position).finalize(ctx.global());
    ctx.bind(draw_boid, "draw_boid");

    be.commit_bindings();
    ctx.io()->set_cwd_from_args(arg_count, args);
    // ctx.compiler()->add_ir_step(debug_ir_step, false);
    // ctx.compiler()->add_ir_step(debug_ir_step, true);

    script_module* mod = ctx.resolve("test");
    if (!mod) {
        print_log(&ctx);
        return -1;
    }
    // print_code(&be);
    // be.log_instructions(true);

    mod->init();

    mod->function("main")->call(nullptr);

    shutdown_ui();

    return 0;
}