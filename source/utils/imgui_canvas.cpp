#include "imgui_canvas.hpp"
#include <imgui_internal.h>

namespace ImGui
{

    bool BeginCanvas(const char* id, ImVec2 size, CanvasParams& params)
    {
        // Start child window
        if (!ImGui::BeginChild(id, size, true, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
            return false;

        ImVec2 canvas_pos  = ImGui::GetCursorScreenPos();    // top-left
        ImVec2 canvas_size = ImGui::GetContentRegionAvail(); // current visible size

        ImGui::InvisibleButton("canvas_bg", canvas_size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
        bool hovered = ImGui::IsItemHovered();
        bool active  = ImGui::IsItemActive();

        // Handle scrolling (pan with right click drag)
        if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            params.origin.x += ImGui::GetIO().MouseDelta.x;
            params.origin.y += ImGui::GetIO().MouseDelta.y;
        }

        // Handle zoom with mouse wheel (optional: zoom to cursor)
        if (hovered)
        {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f)
            {
                ImVec2 mouse  = ImGui::GetIO().MousePos;
                ImVec2 before = (mouse - canvas_pos - params.origin) / params.zoom;
                params.zoom *= (1.0f + wheel * 0.1f);
                params.zoom   = ImClamp(params.zoom, params.min_zoom, params.max_zoom);
                ImVec2 after  = (mouse - canvas_pos - params.origin) / params.zoom;
                ImVec2 offset = (after - before) * params.zoom;
                params.origin += offset; // Clamp zoom
            }
        }

        // Setup canvas state
        params.canvas_pos        = canvas_pos;
        params.canvas_size       = canvas_size;
        params.mouse_pos_canvas  = ImGui::GetIO().MousePos - canvas_pos;
        params.mouse_pos_world   = (params.mouse_pos_canvas - params.origin) / params.zoom;
        params.mouse_pos_world.x = roundf(params.mouse_pos_world.x / params.snap_grid.x) * params.snap_grid.x;
        params.mouse_pos_world.y = roundf(params.mouse_pos_world.y / params.snap_grid.y) * params.snap_grid.y;

        // Clip drawing to canvas area
        ImGui::PushClipRect(canvas_pos, canvas_pos + canvas_size, true);

        return true;
    }

    void EndCanvas()
    {
        ImGui::PopClipRect();
        ImGui::EndChild();
    }

    inline float ComputeDynamicStep(float base_step, float zoom, float min_pixel_spacing = 30.0f)
    {
        float scaled = base_step * zoom;
        int   exp    = 0;

        while (scaled < min_pixel_spacing)
        {
            scaled *= 2.0f;
            exp++;
        }

        return base_step * powf(2.0f, exp);
    }

    void DrawRuler(const CanvasParams& params, ImVec2 grid_step, ImU32 color)
    {
        auto* draw = ImGui::GetWindowDrawList();

        ImVec2 dynamic_step{ComputeDynamicStep(grid_step.x, params.zoom), ComputeDynamicStep(grid_step.y, params.zoom)};

        const ImVec2 canvas_min = params.canvas_pos;
        const ImVec2 canvas_max = canvas_min + params.canvas_size;

        ImFont* font        = ImGui::GetFont();
        float   font_height = font->FontSize;

        // Horizontal ruler
        float start_x = floorf(params.ScreenToWorld(canvas_min).x / dynamic_step.x) * dynamic_step.x;
        float end_x   = params.ScreenToWorld(canvas_max).x;

        for (float x = start_x; x < end_x; x += dynamic_step.x)
        {
            ImVec2 p = params.WorldToScreen(ImVec2(x, 0));
            draw->AddLine(ImVec2(p.x, canvas_min.y), ImVec2(p.x, canvas_min.y + 6), color);
            char buf[16];
            snprintf(buf, sizeof(buf), "%.0f", x);
            draw->AddText(ImVec2(p.x + 2, canvas_min.y + 6), color, buf);
        }

        // Vertical ruler
        float start_y = floorf(params.ScreenToWorld(canvas_min).y / dynamic_step.y) * dynamic_step.y;
        float end_y   = params.ScreenToWorld(canvas_max).y;

        for (float y = start_y; y < end_y; y += dynamic_step.y)
        {
            ImVec2 p = params.WorldToScreen(ImVec2(0, y));
            draw->AddLine(ImVec2(canvas_min.x, p.y), ImVec2(canvas_min.x + 6, p.y), color);
            char buf[16];
            snprintf(buf, sizeof(buf), "%.0f", y);
            draw->AddText(ImVec2(canvas_min.x + 8, p.y - font_height * 0.5f), color, buf);
        }
    }

    void DrawGrid(const CanvasParams& params, ImVec2 grid_step, ImU32 color)
    {
        auto* draw = ImGui::GetWindowDrawList();

        ImVec2 dynamic_step{ComputeDynamicStep(grid_step.x, params.zoom), ComputeDynamicStep(grid_step.y, params.zoom)};

        const ImVec2 canvas_min = params.canvas_pos;
        const ImVec2 canvas_max = canvas_min + params.canvas_size;

        // Convert screen to world bounds
        const ImVec2 world_min = params.ScreenToWorld(canvas_min);
        const ImVec2 world_max = params.ScreenToWorld(canvas_max);

        // Snap start to nearest lower grid cell
        const float start_x = floorf(world_min.x / dynamic_step.x) * dynamic_step.x;
        const float start_y = floorf(world_min.y / dynamic_step.y) * dynamic_step.y;

        for (float x = start_x; x < world_max.x; x += dynamic_step.x)
        {
            const float sx = params.WorldToScreen(ImVec2(x, 0)).x;
            draw->AddLine(ImVec2(sx, canvas_min.y), ImVec2(sx, canvas_max.y), color);
        }

        for (float y = start_y; y < world_max.y; y += dynamic_step.y)
        {
            const float sy = params.WorldToScreen(ImVec2(0, y)).y;
            draw->AddLine(ImVec2(canvas_min.x, sy), ImVec2(canvas_max.x, sy), color);
        }
    }

    void DrawOrigin(const CanvasParams& params, float axis_length)
    {
        auto*  draw          = ImGui::GetWindowDrawList();
        ImVec2 origin_screen = params.WorldToScreen(ImVec2(0, 0));

        // Calculate line endpoints in screen space
        ImVec2 x_start, x_end, y_start, y_end;

        if (axis_length < 0)
        {
            // Extend lines beyond canvas in both directions
            // We'll just pick a large enough length in screen space (like 10k pixels)
            const float big_len = 10000.0f;

            // X axis: horizontal line
            x_start = origin_screen - ImVec2(big_len, 0);
            x_end   = origin_screen + ImVec2(big_len, 0);

            // Y axis: vertical line
            y_start = origin_screen - ImVec2(0, big_len);
            y_end   = origin_screen + ImVec2(0, big_len);
        }
        else
        {
            // Finite length in world space, convert to screen
            x_start = origin_screen;
            x_end   = params.WorldToScreen(ImVec2(axis_length, 0));

            y_start = origin_screen;
            y_end   = params.WorldToScreen(ImVec2(0, axis_length));
        }

        // Draw X axis in red
        draw->AddLine(x_start, x_end, IM_COL32(255, 0, 0, 255), 1.0f);
        // Draw Y axis in green
        draw->AddLine(y_start, y_end, IM_COL32(0, 255, 0, 255), 1.0f);

        // Optional labels at positive ends (only if finite)
        if (axis_length >= 0)
        {
            ImFont* font = ImGui::GetFont();
            if (font)
            {
                draw->AddText(x_end + ImVec2(5, -10), IM_COL32(255, 0, 0, 255), "X");
                draw->AddText(y_end + ImVec2(5, -10), IM_COL32(0, 255, 0, 255), "Y");
            }
        }
    }

    void DrawTexture(const CanvasParams& params, ImTextureID tid, ImVec2 p1, ImVec2 p2, ImVec2 uv1, ImVec2 uv2, ImU32 color)
    {
        auto* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddImage(tid, params.WorldToScreen(p1), params.WorldToScreen(p2), uv1, uv2, color);
    }



    
    ImVec2 CanvasParams::WorldToScreen(ImVec2 world) const
    {
        return canvas_pos + origin + world * zoom;
    }

    ImVec2 CanvasParams::ScreenToWorld(ImVec2 screen) const
    {
        return (screen - canvas_pos - origin) / zoom;
    }

    ImVec2 CanvasParams::Snap(ImVec2 world) const
    {
        return ImVec2{snap_grid.x > 0 ? roundf(world.x / snap_grid.x) * snap_grid.x : world.x,
                      snap_grid.y > 0 ? roundf(world.y / snap_grid.y) * snap_grid.y : world.y};
    }

    ImVec2 CanvasParams::SnapScreenToWorld(ImVec2 screen) const
    {
        return Snap(ScreenToWorld(screen));
    }

    void CanvasParams::CenterOnScreen(ImVec2 size)
    {
        zoom   = 1.0f;
        origin = canvas_size * 0.5f - size * 0.5f;
    }

    void CanvasParams::FitToScreen(ImVec2 size)
    {
        if (size.x == 0)
            size.x = 1.f;
        if (size.y == 0)
            size.y = 1.f;
        auto sc     = canvas_size / size;
        zoom = sc.x < sc.y ? sc.x : sc.y;
        origin      = (canvas_size - size * zoom) * 0.5f;
    }

    bool CanvasParams::DragPoint(ImVec2& pt, void* user_data, float radius_screen)
    {
        if (!dragging)
        {
            ImVec2      mouse_pos = ImGui::GetIO().MousePos;
            ImVec2      pt_screen = WorldToScreen(pt);
            const float dx        = mouse_pos.x - pt_screen.x;
            const float dy        = mouse_pos.y - pt_screen.y;
            const float dist_sq   = dx * dx + dy * dy;
            const float radius_sq = radius_screen * radius_screen;

            if (dist_sq < radius_sq && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                dragging           = true;
                dragging_user_data = user_data;

                // Capture offset in world space between mouse and point
                ImVec2 mouse_world = ScreenToWorld(mouse_pos);
                drag_offset_world  = pt - mouse_world;

                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
        }

        return EndDrag(pt, user_data);
    }

    bool CanvasParams::BeginDrag(ImVec2& pt, void* user_data)
    {
        if (!dragging)
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                dragging           = true;
                dragging_user_data = user_data;

                ImVec2 mouse_world = ScreenToWorld(ImGui::GetIO().MousePos);
                drag_offset_world  = pt - mouse_world;

                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                return true;
            }
        }

        return false;
    }

    bool CanvasParams::EndDrag(ImVec2& pt, void* user_data)
    {
        if (dragging && dragging_user_data == user_data)
        {
            ImVec2 mouse_pos = ImGui::GetIO().MousePos;

            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImVec2 mouse_world = ScreenToWorld(mouse_pos);
                pt                 = Snap(mouse_world + drag_offset_world); // Apply offset
                return true;
            }
            else
            {
                dragging           = false;
                dragging_user_data = nullptr;
            }
        }
        return false;
    }
}