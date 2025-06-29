#include "app.hpp"

namespace box
{
    app::app(properties_t& props) : _props(props)
    {
        auto img = GenImageColor(2, 2, WHITE);
        ImageDrawPixel(&img, 0, 0, BLACK);
        ImageDrawPixel(&img, 1, 1, BLACK);
        _alpha_txt = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    app::~app()
    {
        UnloadTexture(_alpha_txt);
    }

    void app::show()
    {
        show_properties();
        show_texture();
        show_list();
        show_composition();

        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();

            for (uint32_t i = 0; i < droppedFiles.count; ++i)
            {
                add_file(droppedFiles.paths[i]);
            }

            UnloadDroppedFiles(droppedFiles); // Unload filepaths from memory
            _dirty = true;
        }

        if (_dirty)
        {
            _dirty = false;
            repack();
        }
    }

    void app::show_menu()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                char const* filter_patterns[2] = {"*.json", "*.atlas"};
                char const* filter_description = "Atlas file";
                bool        open_save_as       = false;

                if (ImGui::MenuItem("Open"))
                {
                    if (auto file = tinyfd_openFileDialog("Open Atlas",
                                                          "",
                                                          (int)std::size(filter_patterns),
                                                          filter_patterns,
                                                          filter_description,
                                                          0))
                    {
                        open_atlas(file);
                    }
                }
                if (ImGui::BeginMenu("Recent"))
                {
                    for (auto& pth : _props.paths)
                    {
                        if (pth.empty())
                            continue;
                        if (ImGui::MenuItem(pth.c_str()))
                            open_atlas(pth.c_str());

                    }
                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Save"))
                {
                    if (_path.empty())
                        open_save_as = true;
                    else
                        save_atlas(_path.c_str());
                }
                if (ImGui::MenuItem("Save as") || open_save_as)
                {
                    if (auto file = tinyfd_saveFileDialog("Save Atlas",
                                                          "atlas.json",
                                                          (int)std::size(filter_patterns),
                                                          filter_patterns,
                                                          filter_description))
                    {
                        save_atlas(file);
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    void app::show_properties()
    {
        ImGui::Begin("Properties");

        if (ImGui::BeginChildFrame(1, {-1, -1}))
        {
            if (ImGui::CollapsingHeader("Atlas", ImGuiTreeNodeFlags_DefaultOpen))
            {
                show_atlas_properties();
            }

            if (_active && ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
            {
                show_sprite_properties();
            }

            if (_active_comp && ImGui::CollapsingHeader("Composite", ImGuiTreeNodeFlags_DefaultOpen))
            {
                show_composite_properties();
            }

            bool visible = true;

            if (_active_comp &&
                _active_comp->get_active() &&
                ImGui::CollapsingHeader("Node", &visible, ImGuiTreeNodeFlags_DefaultOpen))
            {
                show_node_properties();
            }

            if (!visible && _active_comp)
            {
                _active_comp->remove_node(_active_comp->_active_node);
            }

        }
        ImGui::EndChildFrame();

        ImGui::End();
    }

    void app::show_atlas_properties()
    {
        auto ItemLabel = [](const char* label)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text(label);
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
        };

        if (ImGui::BeginTable("##atlprop",
                              2,
                              ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY,
                              {-1, 220}))
        {
            ItemLabel("Texture width");
            if (ImGui::DragInt("##tsw", &_width))
            {
                _width = _width > 0 ? _width : 1;
                _dirty  = true;
            }

            ItemLabel("Texture height");
            if (ImGui::DragInt("##tsh", &_height))
            {
                _height = _height > 0 ? _height : 1;
                _dirty = true;
            }

            ItemLabel("Padding");
            if (ImGui::DragInt("##pd", &_padding))
            {
                _padding = _padding > 0 ? _padding : 0;
                _dirty   = true;
            }

            ItemLabel("Spacing");
            if (ImGui::DragInt("##sp", &_spacing))
            {
                _spacing = _spacing > 0 ? _spacing : 0;
                _dirty   = true;
            }


            const char* options =
                "BestShortSideFit\0"
                "BestLongSideFit\0"
                "BestAreaFit\0"
                "BottomLeftRule\0"
                "ContactPointRule\0";

            ItemLabel("Packing algorithm");
            if (ImGui::Combo("##hr", &_heuristic, options))
            {
                _dirty = true;
            }

            ItemLabel("Trim size");
            if (ImGui::Checkbox("##trt", &_trim))
            {
                _dirty = true;
            }

            ItemLabel("Embed texture");
            if (ImGui::Checkbox("##emb", &_embed))
            {
            }

            ImGui::EndTable();
        }
    }

    static void ItemLabel(const char* label)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::Text(label);
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-FLT_MIN);
    }

    void app::show_composite_properties()
    {
        if (ImGui::BeginTable("##cmpprop",
                              2,
                              ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, {0,100}))
        {
            ItemLabel("Name");
            ImGui::InputText("##sn", &_active_comp_name);
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                for (auto& el : _compositions)
                {
                    if (&el.second == _active_comp)
                    {
                        if (_compositions.end() == _compositions.find(_active_comp_name))
                        {
                            auto nodeHandler  = _compositions.extract(el.first);
                            nodeHandler.key() = _active_comp_name;
                            _compositions.insert(std::move(nodeHandler));
                        }
                        else
                        {
                            _active_comp_name = el.first;
                        }
                        break;
                    }
                }
            }

            ImGui::EndTable();
        }
    }

    void app::show_node_properties()
    {
        auto& node = *_active_comp->get_active();

        if (ImGui::BeginTable("##ndeprop",
                              2,
                              ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
        {
            ItemLabel("X");
            ImGui::DragFloat("##nposx", &node._position.x);

            ItemLabel("Y");
            ImGui::DragFloat("##nposy", &node._position.y);

            ItemLabel("Scale X");
            ImGui::DragFloat("##nsclx", &node._scale.x, 0.1f);

            ItemLabel("Scale Y");
            ImGui::DragFloat("##nscly", &node._scale.y, 0.1f);

            ItemLabel("Angle");
            ImGui::DragFloat("##nrot", &node._rotation);


            ImGui::TableNextRow();
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            if (ImGui::Button("Up", {-1, 0}))
            {
                _active_comp->reorder_node(_active_comp->_active_node, 1);
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::Button("Down", {-1, 0}))
            {
                _active_comp->reorder_node(_active_comp->_active_node, -1);
            }
            ImGui::EndTable();
        }
    }

    void app::show_sprite_properties()
    {
        if (ImGui::BeginTable("##sprprop",
                              2,
                              ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
        {
            ItemLabel("Name");
            ImGui::InputText("##sn", &_active_name);
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                for (auto& el : _items)
                {
                    if (&el.second == _active)
                    {
                        if (_items.end() == _items.find(_active_name))
                        {
                            auto nodeHandler  = _items.extract(el.first);
                            nodeHandler.key() = _active_name;
                            _items.insert(std::move(nodeHandler));
                        }
                        else
                        {
                            _active_name = el.first;
                        }
                        break;
                    }
                }
            }

            ItemLabel("Data");
            ImGui::SetNextItemWidth(-1);
            const char* options =
                "Origin\0"
                "Two origins\0"
                "9 Patch\0";
            ImGui::Combo("##ad", &_active->_data, options);

            // Basic origin
            if (_active->_data == sprite_data::One)
            {
                ItemLabel("Origin X");
                ImGui::DragInt("##sox", &_active->_oxa);
                ItemLabel("Origin Y");
                ImGui::DragInt("##soy", &_active->_oya);
                ItemLabel("Align");
                show_align(_active->_oxa, _active->_oya, _active->_region.width, _active->_region.height);
            }
            // Two origins / line
            if (_active->_data == sprite_data::Two)
            {
                ItemLabel("Origin X1");
                ImGui::DragInt("##sox1", &_active->_oxa);
                ItemLabel("Origin Y1");
                ImGui::DragInt("##soy1", &_active->_oya);
                ItemLabel("Align");
                show_align(_active->_oxa, _active->_oya, _active->_region.width, _active->_region.height);

                ItemLabel("Origin X2");
                ImGui::DragInt("##sox2", &_active->_oxb);
                ItemLabel("Origin Y2");
                ImGui::DragInt("##soy2", &_active->_oyb);
                ItemLabel("Align");
                ImGui::PushID(42);
                show_align(_active->_oxb, _active->_oyb, _active->_region.width, _active->_region.height);
                ImGui::PopID();
            }
            // Nine patch region
            if (_active->_data == sprite_data::NinePatch)
            {
                ItemLabel("Top");
                ImGui::DragInt("##soya", &_active->_oya);
                ItemLabel("Bottom");
                ImGui::DragInt("##soyb", &_active->_oyb);
                ItemLabel("Left");
                ImGui::DragInt("##soxa", &_active->_oxa);
                ItemLabel("Right");
                ImGui::DragInt("##soxb", &_active->_oxb);
            }
            ImGui::EndTable();
        }
    }

    void app::show_list()
    {
        ImGui::Begin("Sprites");

        if (ImGui::Button(ICON_FA_FOLDER_PLUS))
        {
            add_files();
            _dirty = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FOLDER_MINUS))
        {
            remove_file(_active);
            _dirty = true;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 15});
        if (ImGui::BeginChildFrame(1, {-1, -1}))
        {
            ImVec2 ico(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
            auto clr = ImGui::GetColorU32(ImGuiCol_Text);
            int    index = 1;
            for (auto& spr : _items)
            {
                ImGui::PushID(&spr.second);
                ImGui::PushStyleColor(ImGuiCol_Text, spr.second._packed ? clr : 0xff0000ff);

                const int   maxsze = spr.second._txt.width > spr.second._txt.height ? spr.second._txt.width
                                                                                    : spr.second._txt.height;
                const float scle   = ico.x / maxsze;

                if (ImGui::Selectable("##_", _active == &spr.second))
                {
                    _active      = &spr.second;
                    _active_name = spr.first;
                }

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip))
                {
                    _drag_node._sprite = &spr.second;
                    ImGui::SetDragDropPayload("SPRITE", &_drag_node._sprite, sizeof(sprite*));
                    ImGui::EndDragDropSource();
                }

                ImGui::SameLine();
                ImGui::Image((ImTextureID)&spr.second._txt,
                             ImVec2(scle * spr.second._txt.width, scle * spr.second._txt.height));
                ImGui::SameLine(ico.x + 10);
                ImGui::Text(TextFormat("%02d %s", index, spr.first.c_str()));

                ImGui::PopStyleColor();
                ImGui::PopID();


                ++index;
            }
        }
        ImGui::EndChildFrame();
        ImGui::PopStyleVar();

        ImGui::End();
    }

    void app::show_composition()
    {
        ImGui::Begin("Compositions");

        if (ImGui::Button(ICON_FA_FOLDER_PLUS))
        {
            add_composition("composite");
            _dirty = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FOLDER_MINUS))
        {
            remove_composition(_active_comp);
            _dirty = true;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 15});
        if (ImGui::BeginChildFrame(1, {-1, -1}))
        {
            ImVec2 ico(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
            auto   clr   = ImGui::GetColorU32(ImGuiCol_Text);

            for (auto& spr : _compositions)
            {
                ImGui::PushID(&spr.second);


                if (ImGui::Selectable("##_", _active_comp == &spr.second))
                {
                    _composite_mode   = true;
                    _active_comp = &spr.second;
                    _active      = nullptr;
                    _active_comp_name = spr.first;
                }
                ImGui::SameLine();

                ImGui::Text(TextFormat(" %s [%d]", spr.first.c_str(), spr.second._nodes.size() ));

                ImGui::PopID();
            }
        }
        ImGui::EndChildFrame();
        ImGui::PopStyleVar();

        ImGui::End();
    }

    void app::show_texture()
    {
        if (ImGui::Begin("Texture", 0, ImGuiWindowFlags_NoScrollWithMouse))
        {
            auto old_c = ImGui::GetStyle().Colors[ImGuiCol_Button];
            auto new_c = ImGui::GetStyle().Colors[ImGuiCol_ButtonActive];

            ImGui::GetStyle().Colors[ImGuiCol_Button] = _composite_mode ? old_c : new_c;
            if (ImGui::Button(" Atlas ", {90,0}))
                _composite_mode = false;
            ImGui::SameLine();
            ImGui::GetStyle().Colors[ImGuiCol_Button] = !_composite_mode ? old_c : new_c;
            if (ImGui::Button(" Composite ", {90, 0}))
                _composite_mode = true;
            ImGui::SameLine();
            ImGui::GetStyle().Colors[ImGuiCol_Button] = old_c;

            auto& canvas = _composite_mode ? _comp_canvas : _atlas_canvas;

            if (ImGui::Button(TextFormat("%.0f%%", canvas.zoom * 100), {100, 0}))
            {
                canvas.zoom = 1.f;
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_EXPAND))
            {
                canvas.FitToScreen(get_texture_size());
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_COMPRESS))
            {
                canvas.CenterOnScreen(get_texture_size());
            }

            ImGui::SameLine();
            ImGui::Checkbox("Show origin", &_visible_origin);
            ImGui::SameLine();
            ImGui::Checkbox("Show region", &_visible_region);
            ImGui::SameLine();
            ImGui::Checkbox("Show index", &_visible_index);

            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

            show_canvas(canvas);

            if (_drop_node)
            {
                _drag_node._sprite = nullptr;
                _drop_node         = false;
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (ImGui::AcceptDragDropPayload("SPRITE"))
                {
                    _drop_node         = true;
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PopStyleVar(2);
        }

        ImGui::End();
    }

    void app::show_canvas(ImGui::CanvasParams& canvas)
    {
        if (!ImGui::BeginCanvas("canvas", {-1, -1}, canvas))
            return;

        ImVec2 txtsize(get_texture_size());
        auto   flclr = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
        auto   bgclr = ImGui::GetColorU32(ImGuiCol_NavHighlight);
        auto   dc    = ImGui::GetWindowDrawList();
        auto   mouse = canvas.ScreenToWorld(ImGui::GetIO().MousePos);
        bool   hover = false;
        _mouse       = {mouse.x, mouse.y};

        if (_composite_mode)
        {
            if (_reset_comp_canvas && canvas.canvas_size.x > 0)
            {
                canvas.CenterOnScreen();
                _reset_comp_canvas = false;
            }
            ImGui::DrawGrid(canvas);
            ImGui::DrawOrigin(canvas, -1);

            if (_active_comp)
            {
                matrix2d local = matrix2d({canvas.canvas_pos.x, canvas.canvas_pos.y}, {1.f, 1.f}, {}, 0) *
                                 matrix2d({canvas.origin.x, canvas.origin.y}, {canvas.zoom, canvas.zoom}, {}, 0);
                _active_comp->_mouse = _mouse;
                _active_comp->update_drag(dc, local, canvas.zoom);

                if (_drag_node._sprite)
                {
                    _drag_node._position = local.inverseTransformPoint(GetMousePosition());
                    _drag_node.draw(dc, local, _mouse, &_drag_node);

                    if (_drop_node)
                    {
                        _active_comp->_active_node = (int32_t)_active_comp->_nodes.size();
                        _active_comp->_nodes.emplace_back(_drag_node);

                        _drop_node = false;
                        _drag_node._sprite = nullptr;
                    }
                }
            }
        }
        else
        {
            if (_reset_atlas_canvas && canvas.canvas_size.x > 0)
            {
                canvas.CenterOnScreen(get_texture_size());
                _reset_atlas_canvas = false;
            }

            dc->AddImage((ImTextureID)&_alpha_txt,
                         canvas.WorldToScreen(ImVec2()),
                         canvas.WorldToScreen(txtsize),
                         {},
                         {(txtsize.x / _alpha_txt.width / (16 / canvas.zoom)) / canvas.zoom,
                          (txtsize.y / _alpha_txt.height / (16 / canvas.zoom)) / canvas.zoom}, 0x5fffffff);

            dc->AddRect(canvas.WorldToScreen(ImVec2()), canvas.WorldToScreen(txtsize), 0x5fffffff);

            auto draw_origin = [dc](ImVec2 origin, uint32_t clr)
            {
                dc->AddLine(origin - ImVec2(0, 10), origin + ImVec2(0, 10), clr);
                dc->AddLine(origin - ImVec2(10, 0), origin + ImVec2(10, 0), clr);
                dc->AddRect(origin - ImVec2(3, 3), origin + ImVec2(4, 4), clr);
            };

            int index = 0;
            for (auto& spr : _items)
            {
                ++index;
                if (!spr.second._packed)
                    continue;
                bool isactive = _active == nullptr || _active == &spr.second;

                const uint32_t al = isactive ? 0xffffffff : 0x5fffffff;
                ImVec2         p1(spr.second._region.x, spr.second._region.y);
                ImVec2 p2(spr.second._region.x + spr.second._region.width, spr.second._region.y + spr.second._region.height);
                dc->AddImage((ImTextureID)&spr.second._txt, canvas.WorldToScreen(p1), canvas.WorldToScreen(p2), {0, 0}, {1, 1}, al);

                auto clr = bgclr;

                if (_mouse.x >= p1.x && _mouse.x < p2.x && _mouse.y >= p1.y && _mouse.y < p2.y)
                {
                    hover = true;
                    clr   = flclr;
                    if (IsMouseButtonPressed(0) && !_drag._hovered_active[0] && !_drag._hovered_active[1])
                    {
                        _active      = &spr.second;
                        _active_name = spr.first;
                    }
                }

                if (_visible_region || _active == &spr.second)
                {
                    dc->AddRect(canvas.WorldToScreen(p1), canvas.WorldToScreen(p2), clr);
                }

                if (_visible_index || _active == &spr.second)
                {
                    dc->AddText(canvas.WorldToScreen(p1) + ImVec2{2, 1}, clr, TextFormat("%d", index));
                }

                if (_visible_origin || _active == &spr.second)
                {
                    auto origin = canvas.WorldToScreen(p1 + ImVec2{float(spr.second._oxa), float(spr.second._oya)});
                    origin -= ImVec2(1, 1);
                    auto origin2 = canvas.WorldToScreen(p1 + ImVec2{float(spr.second._oxb), float(spr.second._oyb)});
                    origin2 -= ImVec2(1, 1);

                    clr = flclr;
                    if (_active == &spr.second)
                    {
                        _drag._hovered_active[0] = _mouse.distance_sqr({spr.second._region.x + spr.second._oxa,
                                                                        spr.second._region.y + spr.second._oya}) <
                                                   pow2(8 / canvas.zoom);

                        _drag._hovered_active[1] = _mouse.distance_sqr({spr.second._region.x + spr.second._oxb,
                                                                        spr.second._region.y + spr.second._oyb}) <
                                                   pow2(8 / canvas.zoom);

                        if (_drag._hovered_active[0] || _drag._hovered_active[1])
                        {
                            clr = bgclr;
                        }
                    }

                    if (spr.second._data == sprite_data::One)
                    {
                        draw_origin(origin, clr);
                    }
                    if (spr.second._data == sprite_data::Two)
                    {
                        draw_origin(origin, clr);
                        draw_origin(origin2, clr);
                        dc->AddLine(origin, origin2, clr);
                    }
                    if (spr.second._data == sprite_data::NinePatch)
                    {
                        ImVec2 pp1 = canvas.WorldToScreen(p1 + ImVec2((float)spr.second._oxa, (float)spr.second._oya));
                        ImVec2 pp2 = canvas.WorldToScreen(p1 + ImVec2((float)spr.second._oxb, (float)spr.second._oyb));

                        dc->AddRect(pp1, pp2, clr);
                        draw_origin(pp1, clr);
                        draw_origin(pp2, clr);
                    }
                }

                if (_active == &spr.second && IsMouseButtonPressed(0))
                {
                    if (_drag._hovered_active[0])
                    {
                        _drag._drag_active[0] = true;
                        _drag._drag_origin    = &spr.second;
                        _drag._drag_begin     = {_drag._drag_origin->_oxa, _drag._drag_origin->_oya};
                    }
                    else if (_drag._hovered_active[1])
                    {
                        _drag._drag_active[1] = true;
                        _drag._drag_origin    = &spr.second;
                        _drag._drag_begin     = {_drag._drag_origin->_oxb, _drag._drag_origin->_oyb};
                    }
                }
            }

            if (IsMouseButtonDown(0) && (_drag._drag_active[0] || _drag._drag_active[1]))
            {
                auto off = ImGui::GetMouseDragDelta(0);
                ImGui::BeginTooltip();
                if (_drag._drag_active[0])
                {
                    _drag._drag_origin->_oxa = int32_t(_drag._drag_begin.x + off.x / canvas.zoom);
                    _drag._drag_origin->_oya = int32_t(_drag._drag_begin.y + off.y / canvas.zoom);
                    ImGui::Text("%d x %d", _drag._drag_origin->_oxa, _drag._drag_origin->_oya);
                }
                if (_drag._drag_active[1])
                {
                    _drag._drag_origin->_oxb = int32_t(_drag._drag_begin.x + off.x / canvas.zoom);
                    _drag._drag_origin->_oyb = int32_t(_drag._drag_begin.y + off.y / canvas.zoom);
                    ImGui::Text("%d x %d", _drag._drag_origin->_oxb, _drag._drag_origin->_oyb);
                }
                ImGui::EndTooltip();
            }
            else
            {
                _drag._drag_active[0] = false;
                _drag._drag_active[1] = false;
                _drag._drag_origin    = nullptr;
            }
        }

        ImGui::DrawRuler(canvas);
        ImGui::EndCanvas();
    }

    bool app::show_align(int32_t& x, int32_t& y, float w, float h) const
    {
        float itemw  = 30;
        float offset = ImGui::GetContentRegionAvail().x / 2 - (itemw * 3) / 2;

        auto set_origin = [&x,&y, w, h](ImVec2 vec)
            {
            x = int32_t(w * vec.x);
            y = int32_t(h * vec.y);
            };

        if (ImGui::Button(ICON_FA_CIRCLE "##tl", {itemw, 0}))
            set_origin({0.f, 0.f});
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_CIRCLE "##t", {itemw, 0}))
            set_origin({0.5f, 0.f});
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_CIRCLE "##tr", {itemw, 0}))
            set_origin({1.0f, 0.f});


        if (ImGui::Button(ICON_FA_CIRCLE "##ml", {itemw, 0}))
            set_origin({0.0f, 0.5f});
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_CIRCLE "##m", {itemw, 0}))
            set_origin({0.5f, 0.5f});
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_CIRCLE "##mr", {itemw, 0}))
            set_origin({1.0f, 0.5f});

        if (ImGui::Button(ICON_FA_CIRCLE "##bl", {itemw, 0}))
            set_origin({0.0f, 1.f});
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_CIRCLE "##b", {itemw, 0}))
            set_origin({0.5f, 1.f});
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_CIRCLE "##br", {itemw, 0}))
            set_origin({1.0f, 1.f});
        return false;
    }

    bool app::open_atlas(const char* path)
    {
        reset();
        _path = path;

        auto v = LoadFileText(path);
        if (!v)
            return false;

        msg::Var doc;
        if (msg::VarError::ok != doc.from_string(v))
        {
            UnloadFileText(v);
            return false;
        }
        UnloadFileText(v);

        msg::Var items    = doc.get_item("items");
        msg::Var composites = doc.get_item("composites");
        msg::Var texture  = doc.get_item("texture");
        msg::Var metadata = doc.get_item("metadata");

        _trimed_width = 0;
        _trimed_height = 0;
        _width     = metadata.get_item("width").get(_width);
        _height    = metadata.get_item("height").get(_height);
        _padding   = metadata.get_item("padding").get(_padding);
        _spacing   = metadata.get_item("spacing").get(_spacing);
        _trim      = metadata.get_item("trim_alpha").get(_trim);
        _heuristic = metadata.get_item("heuristics").get(_heuristic);

        Image img{};
        _embed = texture.get_item("data").is_string();
        if (_embed)
        {
            img = load_cb64(texture);
        }
        else
        {
            std::string txtpath = GetDirectoryPath(path);
            txtpath.append("/").append(texture.get_item("file").str());
            img = LoadImage(txtpath.c_str());
        }

        if (!img.data)
            return false;

        for (auto& el : items.elements())
        {
            auto& itm          = _items[el.get_item("id").c_str()];
            itm._region.x      = (float)el.get_item("x").get(0);
            itm._region.y      = (float)el.get_item("y").get(0);
            itm._region.width  = (float)el.get_item("w").get(0);
            itm._region.height = (float)el.get_item("h").get(0);
            itm._data          = el.get_item("d").get(0);
            itm._oxa           = el.get_item("oxa").get(0);
            itm._oya           = el.get_item("oya").get(0);
            itm._oxb           = el.get_item("oxb").get(0);
            itm._oyb           = el.get_item("oyb").get(0);
            auto dta           = el.get_item("img");
            if (dta.is_object())
            {
                itm._img = load_cb64(dta);
            }
            else
            {
                itm._img = ImageFromImage(img, itm._region);
                if (_trimed_width < itm._region.x + itm._region.width + _padding)
                {
                    _trimed_width = int32_t(itm._region.x + itm._region.width) + _padding;
                }
                if (_trimed_height < itm._region.y + itm._region.height + _padding)
                {
                    _trimed_height = int32_t(itm._region.y + itm._region.height) + _padding;
                }
            }
        }

        for (auto& el : composites.elements())
        {
            auto& itm = _compositions[el.get_item("id").c_str()];

            msg::Var items = el.get_item("items");
            for (auto& nde : items.elements())
            {
                auto& node       = itm._nodes.emplace_back();
                node._position.x = (float)nde.get_item("x").get_number(0.);
                node._position.y = (float)nde.get_item("y").get_number(0.);
                node._scale.x    = (float)nde.get_item("sx").get_number(1.);
                node._scale.y    = (float)nde.get_item("sy").get_number(1.);
                node._rotation   = (float)nde.get_item("r").get_number(0.);
                node._sprite     = get_sprite(nde.get_item("s").str());
            }
        }

        _trimed_width += _spacing;
        _trimed_height += _spacing;
        _reset_atlas_canvas = _reset_comp_canvas = true;
        add_to_history(path);
        return true;
    }

    bool app::save_atlas(const char* path)
    {
        msg::Var doc;
        msg::Var sprites;
        msg::Var composites;
        msg::Var metadata;
        msg::Var texture;

        metadata.set_item("width", _width);
        metadata.set_item("height", _height);
        metadata.set_item("padding", _padding);
        metadata.set_item("spacing", _spacing);
        metadata.set_item("trim_alpha", _trim);
        metadata.set_item("heuristics", _heuristic);

        Image image{};
        if (_trim)
            image = GenImageColor(_trimed_width, _trimed_height, {});
        else
            image = GenImageColor(_width, _height, {});

        for (auto& itm : _items)
        {
            msg::Var spr;
            spr.set_item("id", std::string_view(itm.first));
            sprites.push_back(spr);
            if (itm.second._packed)
            {
                spr.set_item("x", itm.second._region.x);
                spr.set_item("y", itm.second._region.y);
                ImageDraw(&image,
                          itm.second._img,
                          {0, 0, itm.second._region.width, itm.second._region.height},
                          itm.second._region,
                          WHITE);
            }
            else
            {
                spr.set_item("img", save_cb64(itm.second._img));
            }

            spr.set_item("w", itm.second._region.width);
            spr.set_item("h", itm.second._region.height);
            if (itm.second._data)
            {
                spr.set_item("d", itm.second._data);
            }
            if (itm.second._oxa)
            {
                spr.set_item("oxa", itm.second._oxa);
            }
            if (itm.second._oya)
            {
                spr.set_item("oya", itm.second._oya);
            }
            if (itm.second._oxb)
            {
                spr.set_item("oxb", itm.second._oxb);
            }
            if (itm.second._oyb)
            {
                spr.set_item("oyb", itm.second._oyb);
            }
        }

        for (auto& itm : _compositions)
        {
            msg::Var cmp;
            cmp.set_item("id", std::string_view(itm.first));
            composites.push_back(cmp);
            msg::Var nodes;
            for (auto& nde : itm.second._nodes)
            {
                msg::Var node;
                node.set_item("x", nde._position.x);
                node.set_item("y", nde._position.y);
                if (nde._scale.x != 1.f)
                    node.set_item("sx", nde._scale.x);
                if (nde._scale.y != 1.f)
                    node.set_item("sy", nde._scale.y);
                if (nde._rotation)
                    node.set_item("r", nde._rotation);
                node.set_item("s", get_sprite_id(nde._sprite));
                nodes.push_back(node);
            }
            cmp.set_item("items", nodes);
        }

        auto r = false;

        if (_embed)
        {
            texture = save_cb64(image);
        }
        else
        {
            std::string texturename(GetFileNameWithoutExt(path));
            texturename.append(".png");
            std::string txtpath = GetDirectoryPath(path);
            txtpath.append("/").append(texturename);
            r = ExportImage(image, txtpath.c_str());
            texture.set_item("file", std::string_view(texturename));
            texture.set_item("width", image.width);
            texture.set_item("height", image.height);
        }

        doc.set_item("items", sprites);
        doc.set_item("composites", composites);
        doc.set_item("metadata", metadata);
        doc.set_item("texture", texture);

        std::string txt;
        doc.to_string(txt);
        return SaveFileText(path, txt.data()) && r;
    }

    void app::add_to_history(const char* path)
    {
        if (!path)
            return;

        for (auto& el : _props.paths)
        {
            if (el == path)
                return;
        }

        for (size_t n = _props.paths.size() - 1; n != 0; --n)
        {
            _props.paths[n] = _props.paths[n - 1];
        }
        _props.paths[0] = path;
    }

    bool app::add_file(const char* path)
    {
        auto img = LoadImage(path);
        if (!img.data)
            return false;

        auto* name = GetFileNameWithoutExt(path);
        auto  it   = _items.find(name);

        if (it != _items.end())
        {
            UnloadImage(it->second._img);
            UnloadTexture(it->second._txt);
            _items.erase(name);
        }

        auto& spr = _items[name];
        spr._img  = img;

        return true;
    }

    bool app::add_files()
    {
        char const* filter_patterns[] = {
            "*.png",
            "*.jpg",
            "*.jpeg",
            "*.psd",
            "*.gif",
        };
        char const* filter_description = "Image file";
        if (auto file = tinyfd_openFileDialog("Open Sprite", "", (int)std::size(filter_patterns), filter_patterns, filter_description, 1))
        {
            auto files = std::split(file, "|");
            for (auto& f : files)
            {
                add_file(f.c_str());
            }
        }

        return false;
    }

    bool app::add_composition(const char* path)
    {
        std::string candidate_name = path;
        int         suffix         = 1;

        // Check for duplicates
        while (_compositions.find(candidate_name) != _compositions.end())
        {
            candidate_name = path;
            candidate_name += "_" + std::to_string(suffix++);
        }

        auto& comp = _compositions[candidate_name];
        return true;
    }

    bool app::remove_composition(composition* spr)
    {
        if (!spr)
            return false;

        for (auto& el : _compositions)
        {
            if (&el.second == spr)
            {
                if (spr == _active_comp)
                    _active_comp = nullptr;
                _compositions.erase(el.first);
                return true;
            }
        }
        return false;
    }

    bool app::remove_file(sprite* spr)
    {
        if (!spr)
            return false;

        for (auto& el : _items)
        {
            if (&el.second == spr)
            {
                if (spr == _active)
                    _active = nullptr;
                _items.erase(el.first);
                return true;
            }
        }
        return false;
    }

    std::string_view app::get_sprite_id(const sprite* spr) const
    {
        for (auto& el : _items)
        {
            if (&el.second == spr)
                return el.first;
        }
        return std::string_view();
    }

    const sprite* app::get_sprite(std::string_view spr) const
    {
        auto it = _items.find(std::string(spr));
        return _items.end() == it ? nullptr : &it->second;
    }

    bool app::repack()
    {
        _item_rect.clear();
        _item_pos.clear();
        _sprites.clear();

        for (auto& el : _items)
        {
            _sprites.emplace_back(&el.second);
            auto& rc  = _item_rect.emplace_back();
            auto& pos = _item_pos.emplace_back();
            rc.width  = el.second._img.width + _padding * 2;
            rc.height = el.second._img.height + _padding * 2;
        }

        float occupancy = 0;
        auto  ret       = maxRects(_width - _spacing * 2,
                            _height - _spacing * 2,
                            (int32_t)_item_rect.size(),
                            _item_rect.data(),
                            maxRectsFreeRectChoiceHeuristic(_heuristic),
                            0,
                            _item_pos.data(),
                            &occupancy);

        _trimed_width  = 0;
        _trimed_height = 0;

        for (int32_t n = 0; n < (int32_t)_item_rect.size(); ++n)
        {
            _sprites[n]->_packed        = _item_pos[n].used;
            _sprites[n]->_region.x      = (float)_item_pos[n].left + _padding + _spacing;
            _sprites[n]->_region.y      = (float)_item_pos[n].top + _padding + _spacing;
            _sprites[n]->_region.width  = (float)_sprites[n]->_img.width;
            _sprites[n]->_region.height = (float)_sprites[n]->_img.height;
            if (!_sprites[n]->_txt.id)
            {
                _sprites[n]->_txt = LoadTextureFromImage(_sprites[n]->_img);
              
                //  SetTextureFilter(_sprites[n]->_txt, RL_TEXTURE_FILTER_BILINEAR);
            }
            if (_sprites[n]->_packed)
            {
                if (_trimed_width < _sprites[n]->_region.x + _sprites[n]->_region.width + _padding)
                {
                    _trimed_width = int32_t(_sprites[n]->_region.x + _sprites[n]->_region.width + _padding);
                }

                if (_trimed_height < _sprites[n]->_region.y + _sprites[n]->_region.height + _padding)
                {
                    _trimed_height = int32_t(_sprites[n]->_region.y + _sprites[n]->_region.height + _padding);
                }
            }
        }
        _trimed_width += _spacing;
        _trimed_height += _spacing;

        return ret != -1;
    }

    void app::reset()
    {
        for (auto& el : _items)
        {
            UnloadImage(el.second._img);
            UnloadTexture(el.second._txt);
        }
        _items.clear();
        _compositions.clear();
        _path.clear();
        _active             = nullptr;
        _atlas_canvas.zoom  = {1.f};
        _comp_canvas.zoom   = {1.f};
        _heuristic          = {};
        _padding            = {};
        _width              = {512};
        _height             = {512};
        _trim               = {};
        _composite_mode     = false;
        _dirty              = true;
        _reset_atlas_canvas = _reset_comp_canvas = true;
    }

    ImVec2 app::get_texture_size() const
    {
        return ImVec2(!_trim ? (float)_width : (float)_trimed_width, !_trim ? (float)_height : (float)_trimed_height);
    }

    Image app::load_cb64(msg::Var ar) const
    {
        Image   out{};
        int32_t b64size  = 0;
        auto    b64data  = DecodeDataBase64((const unsigned char*)ar.get_item("data").c_str(), &b64size);
        int32_t datasize = 0;
        out.data         = DecompressData(b64data, b64size, &datasize);
        out.width        = ar.get_item("width").get(0);
        out.height       = ar.get_item("height").get(0);
        out.format       = ar.get_item("format").get(0);
        out.mipmaps      = 1;
        MemFree(b64data);
        return out;
    }

    msg::Var app::save_cb64(Image image) const
    {
        msg::Var out;

        int32_t dataSize = GetPixelDataSize(image.width, image.height, image.format);
        int32_t compSize = 0;
        auto*   cmpdata  = CompressData((unsigned char*)image.data, dataSize, &compSize);
        int32_t b64size  = 0;
        auto*   b64data  = EncodeDataBase64(cmpdata, compSize, &b64size);

        out.set_item("width", image.width);
        out.set_item("height", image.height);
        out.set_item("format", image.format);
        out.set_item("data", std::string_view(b64data, b64size));

        MemFree(cmpdata);
        MemFree(b64data);

        return out;
    }


    bool composition::draw(size_t nde, ImDrawList* dc, matrix2d& tr)
    {
        if (nde >= _nodes.size())
            return false;

        return _nodes[nde].draw(dc, tr, _mouse, get_active());
    }

    composition::node* composition::get_active()
    {
        if (_active_node >= _nodes.size())
            return nullptr;
        return &_nodes[_active_node];
    }

    void composition::update_drag(ImDrawList* dc, matrix2d& local, float zoom)
    {
        size_t hover_node = -1;
        bool   rotation_hover = false;
        bool   focused        = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);


        for (size_t n = 0; n < _nodes.size(); ++n)
        {
            if (draw(n, dc, local))
                hover_node = n;
        }

        if (_selected)
        {
            ImVec2 c       = local.transformPoint(ImVec2(_center.x, _center.y));
            auto   dst     = (_mouse - _center).distance_sqr();
            rotation_hover = dst < std::pow(90 / zoom, 2) && dst > std::pow(70 / zoom, 2); 

            dc->AddCircle(c, 5, 0x7fffffff);
            dc->AddCircle(c, 80, rotation_hover ? 0xff00ffff : 0x7fffffff);
        }

        if (!IsMouseButtonDown(0))
        {
            _drag = false;
            _rotate = false;
        }

        if (ImGui::BeginPopup("Options"))
        {
            if (ImGui::MenuItem("Bring to Front"))
            {
                reorder_node(_active_node, 10000);
            }
            if (ImGui::MenuItem("Move up"))
            {
                reorder_node(_active_node, 1);
            }
            if (ImGui::MenuItem("Move down"))
            {
                reorder_node(_active_node, -1);
            }
            if (ImGui::MenuItem("Bring to Back"))
            {
                reorder_node(_active_node, -10000);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
            {
                remove_node(_active_node);
            }
            ImGui::EndPopup();

            return;
        }

        if (focused && IsKeyPressed('A') && ImGui::GetIO().KeyCtrl)
        {
            select_all(true);
        }

        if (focused && IsMouseButtonPressed(1) && !std::isnan(_mouse.x))
        {
            if (hover_node != -1)
            {
                select(hover_node, ImGui::GetIO().KeyCtrl);
                ImGui::OpenPopup("Options");
            }
        }

        if (focused && IsMouseButtonPressed(0) && !std::isnan(_mouse.x))
        {
            if (rotation_hover)
            {
                _rotate   = true;
                _grag_pos = _mouse;
            }
            else if (hover_node != -1)
            {
                if(_nodes[hover_node]._selected)
                {
                    _drag     = true;
                    _grag_pos = _mouse;
                }
                else
                {
                    select(hover_node, ImGui::GetIO().KeyCtrl);
                }
            }
            else
            {
                select_all(false);
            }
        }
        _hover_node = hover_node;

        if (_drag)
        {
            auto offset = _mouse - _grag_pos;
            move_selected(offset);
            _grag_pos = _mouse;
        }

        if (_rotate && !std::isnan(_mouse.x))
        {
            auto a = _grag_pos - _center;
            auto b = _mouse - _center;
            float rot = (b.angle() - a.angle());
            rotate_selected(_center, rot * (180.0f / M_PI));
            _grag_pos = _mouse;
        }
    }

    size_t composition::node_index(const node* n) const
    {
        return std::distance(_nodes.data(), n);
    }

    void composition::select(size_t nde, bool ctrl)
    {
        if (!ctrl)
        {
            select_all(false);
        }

        _active_node = nde;

        if (nde >= _nodes.size())
            return;

        _nodes[nde]._selected = true;
        _selected             = true;
        _center               = get_selected_center();
    }

    void composition::select_all(float v)
    {
        _active_node = _nodes.size() - 1;
        for (auto& n : _nodes)
        {
            n._selected = v;
        }
        if (!v)
            _active_node = -1;
        else
            _center = get_selected_center();
        _selected = v;
    }

    void composition::move_selected(point2f offset)
    {
        for (auto& n : _nodes)
        {
            if (n._selected)
            {
                n._position += offset;
            }
        }
        _center = get_selected_center();
    }

    point2f composition::get_selected_center() const
    {
        point2f center         = {0.f, 0.f};
        int     selected_count = 0;

        for (const auto& n : _nodes)
        {
            if (n._selected)
            {
                center.x += n._position.x;
                center.y += n._position.y;
                ++selected_count;
            }
        }

        if (selected_count > 0)
        {
            center.x /= selected_count;
            center.y /= selected_count;
        }

        return center;
    }

    void composition::rotate_selected(point2f center, float rotation)
    {
        const auto radians   = rotation * (M_PI / 180.0f);
        const auto cos_theta = std::cos(radians);
        const auto sin_theta = std::sin(radians);

        for (auto& n : _nodes)
        {
            if (n._selected)
            {
                auto relative_pos = n._position - center;
                n._rotation += rotation;
                n._position          = point2f{relative_pos.x * cos_theta - relative_pos.y * sin_theta,
                                      relative_pos.x * sin_theta + relative_pos.y * cos_theta} + center;

                if (n._rotation >= 360.0f)
                    n._rotation -= 360.0f;
                else if (n._rotation < 0.0f)
                    n._rotation += 360.0f;
            }
        }
    }

    void composition::scale_selected(point2f center, point2f scale)
    {
        for (auto& n : _nodes)
        {
            if (n._selected)
            {
                n._position.x = center.x + (n._position.x - center.x) * scale.x;
                n._position.y = center.y + (n._position.y - center.y) * scale.y;
                n._scale.x *= scale.x;
                n._scale.y *= scale.y;
            }
        }
    }

    void composition::remove_sprite(sprite* spr)
    {
        for (auto& n : _nodes)
        {
            if (n._sprite == spr)
                n._sprite = nullptr;
        }
    }

    bool composition::remove_node(size_t n)
    {
        if (n >= _nodes.size())
            return false;

        _nodes.erase(_nodes.begin() + n);
        select_all(false);
        return true;
    }

    bool composition::reorder_node(size_t n, int32_t dir)
    {
        if (_nodes.empty() || n >= _nodes.size())
        {
            return false;
        }

        bool reselect = _active_node == n;
        int32_t new_pos = static_cast<int32_t>(n) + dir;
        new_pos = std::max(0, std::min(static_cast<int32_t>(_nodes.size() - 1), new_pos));

        if (new_pos == static_cast<int32_t>(n))
        {
            return true;
        }

        if (new_pos < static_cast<int32_t>(n))
        {
            std::rotate(_nodes.begin() + new_pos, _nodes.begin() + n, _nodes.begin() + n + 1);
        }
        else
        {
            std::rotate(_nodes.begin() + n, _nodes.begin() + n + 1, _nodes.begin() + new_pos + 1);
        }

        if (reselect)
        {
            _active_node = new_pos;
            _center      = get_selected_center();
        }
        return true;
    }

    bool composition::node::draw(ImDrawList* dc, matrix2d& local, point2f mpos, const node* active_node) const
    {
        bool hovered = false;

        if (!_sprite)
            return hovered;

        matrix2d tr(_position, _scale, point2f{_sprite->_oxa, _sprite->_oya}, _rotation);
        auto     lpos = tr.inverseTransformPoint(mpos);
        tr            = local * tr;

        if (lpos.x >= 0 && lpos.y >= 0 && lpos.x < _sprite->_region.width && lpos.y < _sprite->_region.height)
        {
            hovered = true;
        }

        dc->PushTextureID((ImTextureID)&_sprite->_txt);

        ImVec2 pos[4];
        pos[0] = tr.transformPoint(ImVec2{0, 0});
        pos[1] = tr.transformPoint(ImVec2{0, _sprite->_region.height});
        pos[2] = tr.transformPoint(ImVec2{_sprite->_region.width, _sprite->_region.height});
        pos[3] = tr.transformPoint(ImVec2{_sprite->_region.width, 0});

        dc->PrimReserve(6, 4);
        dc->PrimWriteIdx((ImDrawIdx)(dc->_VtxCurrentIdx));
        dc->PrimWriteIdx((ImDrawIdx)(dc->_VtxCurrentIdx + 1));
        dc->PrimWriteIdx((ImDrawIdx)(dc->_VtxCurrentIdx + 2));
        dc->PrimWriteIdx((ImDrawIdx)(dc->_VtxCurrentIdx));
        dc->PrimWriteIdx((ImDrawIdx)(dc->_VtxCurrentIdx + 2));
        dc->PrimWriteIdx((ImDrawIdx)(dc->_VtxCurrentIdx + 3));

        dc->PrimWriteVtx(pos[0], {0.f, 0.f}, 0xffffffff);
        dc->PrimWriteVtx(pos[1], {0.f, 1.f}, 0xffffffff);
        dc->PrimWriteVtx(pos[2], {1.f, 1.f}, 0xffffffff);
        dc->PrimWriteVtx(pos[3], {1.f, 0.f}, 0xffffffff);

        dc->PopTextureID();

        dc->AddPolyline(pos, 4, active_node == this || _selected ? 0xffffffff : 0x5fffffff, ImDrawFlags_Closed, 1.f);

        return hovered;
    }

} // namespace box