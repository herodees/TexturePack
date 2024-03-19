
#include "include.hpp"
#include "app.hpp"

void ShowDock(box::app&);
void InitTheme(bool dark_theme);
bool EnableDarkTheme(size_t wnd);

int  main(int argc, char* argv[])
{
    // Initialization
    //--------------------------------------------------------------------------------------
    int screenWidth  = 1280;
    int screenHeight = 800;

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "BOX TexturePacker");
    SetTargetFPS(144);
    rlImGuiSetup(true);
    InitTheme(true);

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    box::app _app;

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        BeginDrawing();
        ClearBackground(DARKGRAY);

        // start ImGui Conent
        rlImGuiBegin();

        // show ImGui Content
        // bool open = true;
        // ImGui::ShowDemoWindow(&open);

        ShowDock(_app);

        // end ImGui Content
        rlImGuiEnd();

        EndDrawing();
        //----------------------------------------------------------------------------------
    }
    rlImGuiShutdown();

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

void ShowDock(box::app& application)
{
    static bool init_docking = true;

    ImGuiWindowFlags flags       = ImGuiWindowFlags_MenuBar;
    flags |= ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", 0, flags);
    ImGui::PopStyleVar();

    if (ImGui::BeginMenuBar())
    {
        application.show_menu();
        ImGui::EndMenuBar();
    }

    ImGuiIO& io           = ImGui::GetIO();
    ImGuiID  dockspace_id = ImGui::GetID("MyDockspace");

    if (init_docking)
    {
        init_docking = false;

        ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
        ImGui::DockBuilderAddNode(dockspace_id); // Add empty node

        ImGuiID dock_main_id = dockspace_id; // This variable will track the document node, however we are not using it here as we aren't docking anything into it.
        ImGuiID dock_id_list = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, NULL, &dock_main_id);
        ImGuiID dock_id_prop   = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, NULL, &dock_main_id);
        ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.20f, NULL, &dock_main_id);

        ImGui::DockBuilderDockWindow("Texture", dock_id_bottom);
        ImGui::DockBuilderDockWindow("Properties", dock_id_prop);
        ImGui::DockBuilderDockWindow("Sprites", dock_id_list);
        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::DockSpace(dockspace_id);
    if (!init_docking)
    {
        application.show();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void InitTheme(bool dark_theme)
{
    rlImGuiSetup(dark_theme);

    EnableDarkTheme((size_t)GetWindowHandle());

    Image ico = GenImageColor(16, 16, {255,255,255,50});
    SetWindowIcon(ico);
    UnloadImage(ico);

    ImGui::GetIO().IniFilename = nullptr;

    // Cherry style from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha                     = 1.0f;
    style.DisabledAlpha             = 0.6000000238418579f;
    style.WindowPadding             = ImVec2(3.0f, 3.0f);
    style.WindowRounding            = 7.0f;
    style.WindowBorderSize          = 1.0f;
    style.WindowMinSize             = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign          = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition  = ImGuiDir_Left;
    style.ChildRounding             = 7.0f;
    style.ChildBorderSize           = 1.0f;
    style.PopupRounding             = 0.0f;
    style.PopupBorderSize           = 1.0f;
    style.FramePadding              = ImVec2(3.0f, 3.0f);
    style.FrameRounding             = 3.0f;
    style.FrameBorderSize           = 0.0f;
    style.ItemSpacing               = ImVec2(3.0f, 3.0f);
    style.ItemInnerSpacing          = ImVec2(3.0f, 3.0f);
    style.CellPadding               = ImVec2(3.0f, 3.0f);
    style.IndentSpacing             = 21.0f;
    style.ColumnsMinSpacing         = 3.0f;
    style.ScrollbarSize             = 13.0f;
    style.ScrollbarRounding         = 3.0f;
    style.GrabMinSize               = 20.0f;
    style.GrabRounding              = 3.0f;
    style.TabRounding               = 3.0f;
    style.TabBorderSize             = 1.0f;
    style.TabMinWidthForCloseButton = 0.0f;
    style.ColorButtonPosition       = ImGuiDir_Right;
    style.ButtonTextAlign           = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign       = ImVec2(0.0f, 0.0f);

    ImVec4* colors                         = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text]                  = ImVec4(0.86f, 0.93f, 0.89f, 0.68f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.86f, 0.93f, 0.89f, 0.28f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.20f, 0.22f, 0.27f, 0.90f);
    colors[ImGuiCol_Border]                = ImVec4(0.99f, 0.99f, 0.99f, 0.05f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.27f, 0.32f, 0.45f, 0.17f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.45f, 0.20f, 0.30f, 0.78f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.20f, 0.07f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.44f, 0.14f, 0.26f, 0.97f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.45f, 0.20f, 0.30f, 0.78f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.38f, 0.65f, 0.91f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.38f, 0.65f, 0.91f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.11f, 0.52f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.45f, 0.20f, 0.30f, 0.86f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.45f, 0.20f, 0.30f, 0.76f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.45f, 0.20f, 0.30f, 0.86f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.50f, 0.07f, 0.25f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.41f, 0.35f, 0.40f, 0.47f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.11f, 0.52f, 1.00f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.45f, 0.20f, 0.30f, 0.78f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.45f, 0.20f, 0.30f, 0.76f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.45f, 0.20f, 0.30f, 0.86f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.50f, 0.07f, 0.25f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_DockingPreview]        = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.07f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.45f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.45f, 0.20f, 0.30f, 0.43f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}