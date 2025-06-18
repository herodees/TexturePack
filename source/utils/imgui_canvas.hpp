#pragma once

#include <imgui.h>

namespace ImGui
{
    struct CanvasParams
    {
        ImVec2 origin   = ImVec2(0, 0); // pan offset
        float  zoom     = 1.0f;         // zoom factor
        float  min_zoom = 0.05f;
        float  max_zoom = 8.0f;
        ImVec2 mouse_pos_canvas; // mouse position in canvas local
        ImVec2 mouse_pos_world;  // transformed to world space
        ImVec2 canvas_pos;       // top-left of canvas in screen space
        ImVec2 canvas_size;      // canvas size
        ImVec2 snap_grid          = ImVec2(0, 0);
        ImVec2 drag_offset_world  = ImVec2(0, 0);
        bool   dragging           = false;
        void*  dragging_user_data = nullptr;

        ImVec2 WorldToScreen(ImVec2 world) const;
        ImVec2 ScreenToWorld(ImVec2 screen) const;
        ImVec2 Snap(ImVec2 world) const;
        ImVec2 SnapScreenToWorld(ImVec2 screen) const;
        void   CenterOnScreen(ImVec2 size = {0, 0});
        void   FitToScreen(ImVec2 size);
        bool   DragPoint(ImVec2& pt, void* user_data, float radius_screen);
        bool   BeginDrag(ImVec2& pt, void* user_data);
        bool   EndDrag(ImVec2& pt, void* user_data);
    };

    bool BeginCanvas(const char* id, ImVec2 size, CanvasParams& params);
    void EndCanvas();
    void DrawRuler(const CanvasParams& params, ImVec2 grid_step = ImVec2(32, 32), ImU32 color = IM_COL32(255, 255, 255, 180));
    void DrawGrid(const CanvasParams& params, ImVec2 grid_step = ImVec2(32, 32), ImU32 color = IM_COL32(255, 255, 255, 30));
    void DrawOrigin(const CanvasParams& params, float axis_length = 100.0f);
    void DrawTexture(const CanvasParams& params, ImTextureID tid, ImVec2 p1, ImVec2 p2, ImVec2 uv1, ImVec2 uv2, ImU32 color);
}