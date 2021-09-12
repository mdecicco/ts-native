#include <gjs/gjs.h>
#include <gjs/backends/b_x86.h>
#include <gjs/optimize/optimize.h>
#include <gjs/builtin/script_vec2.h>
#include <gjs/builtin/script_vec3.h>
#include <stdio.h>
#include <time.h>
#include <chrono>
#include <thread>

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
void pf(f32 f) {
    printf("%.2f\n", f);
}
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
f32 maxSpeed = 230.0f;
f32 minSpeed = 20.0f;
f32 cohesionFac = 180.0f;
f32 separationFac = 120.0f;
f32 alignFac = 180.0f;
f32 wanderFac = 120.0f;
f32 minDist = 20.0f;
f32 detectionRadius = 50.0f;

f32 max_speed() { return maxSpeed; }
f32 min_speed() { return minSpeed; }
f32 cohesion_fac() { return cohesionFac * 0.01f; }
f32 separation_fac() { return separationFac * 0.01f; }
f32 align_fac() { return alignFac * 0.01f; }
f32 wander_fac() { return wanderFac * 0.01f; }
f32 min_dist() { return minDist; }
f32 detection_radius() { return detectionRadius; }

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
    u64 us = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - frameStartTime).count();
    dt = f32(us) / 1000000.0f;
    ImGui::Text("Frame Time: %.2fms", dt * 1000.0);
    ImGui::Text("FPS: %.2f", 1.0 / dt);
    f32 diff = ((1.0f / 60.0f) * 1000.0f) - (dt * 1000.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds((i32)diff));
    dt = 1.0f / 60.0f;
    ImGui::PushItemWidth(300.0f);
    ImGui::DragFloat("Max Speed"   , &maxSpeed       , 1.0f, 0.1f , 1000.0f, "%.2f", 1.0f);
    ImGui::DragFloat("Min Speed"   , &minSpeed       , 1.0f, 0.1f , 1000.0f, "%.2f", 1.0f);
    ImGui::DragFloat("Cohesion"    , &cohesionFac    , 1.0f, 10.0f, 1000.0f, "%.2f", 1.0f);
    ImGui::DragFloat("Separation"  , &separationFac  , 1.0f, 10.0f, 1000.0f, "%.2f", 1.0f);
    ImGui::DragFloat("Align"       , &alignFac       , 1.0f, 10.0f, 1000.0f, "%.2f", 1.0f);
    ImGui::DragFloat("Wander"      , &wanderFac      , 1.0f, 10.0f, 1000.0f, "%.2f", 1.0f);
    ImGui::DragFloat("Min Distance", &minDist        , 1.0f, 0.1f , 100.0f , "%.2f", 1.0f);
    ImGui::DragFloat("Radius"      , &detectionRadius, 1.0f, 0.1f , 100.0f , "%.2f", 1.0f);
    ImGui::PopItemWidth();
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

void log_update(script_context* ctx, compilation_output& in, u16 fidx) {
    if (fidx != 2) return;
    debug_ir_step(ctx, in, fidx);
}

int main(int arg_count, const char** args) {
    srand(time(nullptr));

    // basic_malloc_allocator alloc;
    // vm_backend be(&alloc, 8 * 1024 * 1024, 8 * 1024 * 1024);
    x86_backend be;
    script_context ctx(&be);
    ctx.bind(pf, "pf");

    ctx.bind(deltaT, "deltaT");
    ctx.bind(running, "running");
    ctx.bind(begin, "begin_frame");
    ctx.bind(end, "end_frame");
    ctx.bind(FUNC_PTR(window_size, vec2<f32>), "window_size");
    ctx.bind(max_speed, "max_speed");
    ctx.bind(min_speed, "min_speed");
    ctx.bind(cohesion_fac, "cohesion_fac");
    ctx.bind(separation_fac, "separation_fac");
    ctx.bind(align_fac, "align_fac");
    ctx.bind(wander_fac, "wander_fac");
    ctx.bind(min_dist, "min_dist");
    ctx.bind(detection_radius, "detection_radius");
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
    // ctx.compiler()->add_ir_step(optimize::ir_phase_1, false);
    ctx.compiler()->add_ir_step(optimize::dead_code, false);
    ctx.compiler()->add_ir_step(debug_ir_step, false);
    // ctx.compiler()->add_ir_step(log_update, false);
    // ctx.compiler()->add_ir_step(log_update, true);

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