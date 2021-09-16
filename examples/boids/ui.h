#pragma once
#include <imgui.h>
#include <imgui_internal.h>

bool init_ui();
void window_size(int& w, int& h);
bool do_exit();
void begin_frame();
void end_frame(float br, float bg, float bb, float ba);
void shutdown_ui();