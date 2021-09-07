#include <gjs/gjs.h>
#include <gjs/optimize/optimize.h>
#include <gjs/builtin/script_vec2.h>
#include <gjs/builtin/script_vec3.h>
#include <stdio.h>
#include <time.h>
#include <chrono>

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
    vec2<f32> position;
    vec2<f32> velocity;
    vec2<f32> acceleration;
    f32 wander;
};
static f32 bscale = 25.0f;
void draw_boid(const boid& b) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    vec2<f32> facing = b.velocity.normalized();
    mat3<f32> orientation = mat3<f32>::compose2D(r2d<f32>(atan2f(facing.y, facing.x) + 1.5708f), vec2<f32>(bscale, bscale), b.position);
    vec2<f32> points[5] = {
        orientation * arrow[0],
        orientation * arrow[1],
        orientation * arrow[2],
        orientation * arrow[3],
        orientation * arrow[4]
    };

    dl->AddPolyline((ImVec2*)points, 5, ImColor(255, 255, 255), ImDrawFlags_None, 1.0f);
}

void draw_line(const vec2<f32>& a, const vec2<f32>& b) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    vec2<f32> points[2] = { a, b };
    dl->AddPolyline((ImVec2*)points, 2, ImColor(255, 255, 255), ImDrawFlags_None, 1.0f);
}

auto frameStartTime = std::chrono::high_resolution_clock::now();
f32 dt = 1.0f / 60.0f;
void begin() {
    frameStartTime = std::chrono::high_resolution_clock::now();
    begin_frame();
    i32 w, h;
    window_size(w, h);
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
}

void end() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    u64 ms = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - frameStartTime).count();
    dt = f32(ms) / 1000000.0f;
    ImGui::Text("Frame Time: %.2fms", dt * 1000.0);
    ImGui::Text("FPS: %.2f", 1.0 / dt);
    ImGui::End();
    ImGui::PopStyleVar();
    end_frame(0.25f, 0.25f, 0.25f, 1.0f);
}

f32 deltaT() {
    return dt;
}

bool running() { return !do_exit(); }

vec2<f32> window_size() {
    i32 w, h;
    window_size(w, h);
    return vec2<f32>(w, h);
}

int main(int arg_count, const char** args) {
    srand(time(nullptr));

    basic_malloc_allocator alloc;
    vm_backend be(&alloc, 8 * 1024 * 1024, 8 * 1024 * 1024);
    script_context ctx(&be);

    ctx.bind(deltaT, "deltaT");
    ctx.bind(running, "running");
    ctx.bind(begin, "begin_frame");
    ctx.bind(end, "end_frame");
    ctx.bind(FUNC_PTR(window_size, vec2<f32>), "window_size");
    ctx.bind<boid>("boid")
        .prop("position", &boid::position)
        .prop("velocity", &boid::velocity)
        .prop("acceleration", &boid::acceleration)
        .prop("wander", &boid::wander)
    .finalize(ctx.global())->is_trivially_copyable = true;
    ctx.bind(draw_boid, "draw_boid");
    ctx.bind(draw_line, "draw_line");

    be.commit_bindings();
    ctx.io()->set_cwd_from_args(arg_count, args);
    //ctx.compiler()->add_ir_step(debug_ir_step, false);
    ctx.compiler()->add_ir_step(optimize::test_step, false);
    //ctx.compiler()->add_ir_step(debug_ir_step, false);
    // ctx.compiler()->add_ir_step(debug_ir_step, true);

    script_module* mod = ctx.resolve("test");
    if (!mod) {
        print_log(&ctx);
        return -1;
    }
    // print_code(&be);
    // be.log_instructions(true);
    // be.log_lines(true);

    if (!init_ui()) {
        return 1;
    }


    mod->init();

    mod->function("main")->call(nullptr);

    shutdown_ui();

    return 0;
}