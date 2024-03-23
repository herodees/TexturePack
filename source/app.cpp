#include "app.hpp"

namespace box
{
    app::app()
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

    void app::show_sprite_properties()
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
                show_align(_active->_oxb, _active->_oyb, _active->_region.width, _active->_region.height);
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
                ImGui::SameLine();
                ImGui::Image((ImTextureID)&spr.second._txt,
                             ImVec2(scle * spr.second._txt.width, scle * spr.second._txt.height));
                ImGui::SameLine(ico.x + 10);
                ImGui::Text(spr.first.c_str());

                ImGui::PopStyleColor();
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
            ImGui::SetNextItemWidth(200);
            if (ImGui::SliderFloat("##zm", &_zoom, 0.1f, 5.f))
            {
                set_atlas_scale({_zoom, _zoom}, {0,0});
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_EXPAND))
            {
                _zoom = 1.f;
                _transform.reset();
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_COMPRESS))
            {
                _zoom = _ideal_zoom;
                _transform.reset();
                _transform.scale(_zoom, _zoom);
                _transform.tx = _ideal_offset.x;
                _transform.ty = _ideal_offset.y;
            }

            ImGui::SameLine();
            ImGui::Checkbox("Show origin", &_visible_origin);
            ImGui::SameLine();
            ImGui::Checkbox("Show region", &_visible_region);

            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

            if (ImGui::BeginChild(10, {-1, -1}, 0, ImGuiWindowFlags_NoScrollWithMouse))
            {
                show_canvas(_transform);
            }
            ImGui::EndChild();

            ImGui::PopStyleVar(2);
        }

        ImGui::End();
    }

    void app::show_canvas(matrix2d& transform)
    {
        ImVec2 txtsize(!_trim ? (float)_width : (float)_trimed_width, !_trim ? (float)_height : (float)_trimed_height);
        ImVec2 from = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetContentRegionAvail();

        matrix2d local({from.x, from.y}, {1.f, 1.f}, {(size.x - txtsize.x) / -2, (size.y - txtsize.y) / -2}, 0);

        auto iz         = size / txtsize;
        _ideal_zoom     = iz.x < iz.y ? iz.x : iz.y;
        _ideal_offset.x = (size.x - txtsize.x) / -2;
        _ideal_offset.y = (size.y - txtsize.y) / -2;

        local = local * transform;
        if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows))
            _mouse = {NAN, NAN};
        else
            _mouse = local.inverseTransformPoint(GetMousePosition());

        auto flclr = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
        auto bgclr = ImGui::GetColorU32(ImGuiCol_NavHighlight);
        auto dc    = ImGui::GetWindowDrawList();
        bool hover = false;
        dc->Flags  = ImDrawListFlags_None;

        //dc->AddRectFilled(local.transformPoint(ImVec2()), local.transformPoint(txtsize), 0x1fffffff);
        dc->AddImage((ImTextureID)&_alpha_txt,
                     local.transformPoint(ImVec2()),
                     local.transformPoint(txtsize),
                     {0, 0},
                     {txtsize.x / 16.f, txtsize.y / 16.f},
                     0x3fffffff);
        dc->AddRect(local.transformPoint(ImVec2()), local.transformPoint(txtsize), 0x5fffffff);

        auto draw_origin = [dc](ImVec2 origin, uint32_t clr)
            {
            dc->AddLine(origin - ImVec2(0, 10), origin + ImVec2(0, 10), clr);
            dc->AddLine(origin - ImVec2(10, 0), origin + ImVec2(10, 0), clr);
            dc->AddRect(origin - ImVec2(3, 3), origin + ImVec2(4, 4), clr);
            };

        for (auto& spr : _items)
        {
            if (!spr.second._packed)
                continue;
            bool isactive = _active == nullptr || _active == &spr.second;

            const uint32_t al = isactive ? 0xffffffff : 0x5fffffff;
            ImVec2 p1(spr.second._region.x, spr.second._region.y);
            ImVec2 p2(spr.second._region.x + spr.second._region.width, spr.second._region.y + spr.second._region.height);
            dc->AddImage((ImTextureID)&spr.second._txt,
                         local.transformPoint(p1), local.transformPoint(p2),
                         {0, 0},
                         {1, 1},
                         al);

            auto clr = bgclr;

            if (_mouse.x >= p1.x && _mouse.x < p2.x && _mouse.y >= p1.y && _mouse.y < p2.y)
            {
                hover = true;
                clr = flclr;
                if (IsMouseButtonPressed(0) && !_drag._hovered_active[0] && !_drag._hovered_active[1])
                {
                    _active      = &spr.second;
                    _active_name = spr.first;
                }
            }

            if (_visible_region || _active == &spr.second)
            {
                dc->AddRect(local.transformPoint(p1), local.transformPoint(p2), clr);
            }
            if (_visible_origin || _active == &spr.second)
            {
                auto origin = local.transformPoint(p1 + ImVec2{float(spr.second._oxa), float(spr.second._oya)});
                origin -= ImVec2(1, 1);
                auto origin2 = local.transformPoint(p1 + ImVec2{float(spr.second._oxb), float(spr.second._oyb)});
                origin2 -= ImVec2(1, 1);

                clr = flclr;
                if (_active == &spr.second)
                {
                    _drag._hovered_active[0] = _mouse.distance_sqr({spr.second._region.x + spr.second._oxa,
                                                                    spr.second._region.y + spr.second._oya}) < pow2(6);

                    _drag._hovered_active[1] = _mouse.distance_sqr({spr.second._region.x + spr.second._oxb,
                                                                    spr.second._region.y + spr.second._oyb}) < pow2(6);

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
                    ImVec2 pp1 = local.transformPoint(p1 + ImVec2((float)spr.second._oxa, (float)spr.second._oya));
                    ImVec2 pp2 = local.transformPoint(p1 + ImVec2((float)spr.second._oxb, (float)spr.second._oyb));

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
                    _drag._drag_origin = &spr.second;
                    _drag._drag_begin  = {_drag._drag_origin->_oxa, _drag._drag_origin->_oya};
                }
                else if (_drag._hovered_active[1])
                {
                    _drag._drag_active[1] = true;
                    _drag._drag_origin = &spr.second;
                    _drag._drag_begin  = {_drag._drag_origin->_oxb, _drag._drag_origin->_oyb};
                }
            }
        }

        if (!isnan(_mouse.x) && !isnan(_mouse.y) && ImGui::GetIO().MouseWheel)
        {
            _zoom += ImGui::GetIO().MouseWheel * 0.1f;
            if (_zoom < 0.1f)
                _zoom = 0.1f;
            set_atlas_scale({_zoom, _zoom}, {_mouse.x, _mouse.y});
        }
        else if (!hover && !isnan(_mouse.x) && IsMouseButtonPressed(0) && !_drag._drag_origin)
        {
            _active = nullptr;
        }

        if (IsMouseButtonDown(0) && (_drag._drag_active[0] || _drag._drag_active[1]))
        {
            auto off = ImGui::GetMouseDragDelta(0);
            ImGui::BeginTooltip();
            if (_drag._drag_active[0])
            {
                _drag._drag_origin->_oxa = int32_t(_drag._drag_begin.x + off.x / _zoom);
                _drag._drag_origin->_oya = int32_t(_drag._drag_begin.y + off.y / _zoom);
                ImGui::Text("%d x %d", _drag._drag_origin->_oxa, _drag._drag_origin->_oya);
            }
            if (_drag._drag_active[1])
            {
                _drag._drag_origin->_oxb = int32_t(_drag._drag_begin.x + off.x / _zoom);
                _drag._drag_origin->_oyb = int32_t(_drag._drag_begin.y + off.y / _zoom);
                ImGui::Text("%d x %d", _drag._drag_origin->_oxb, _drag._drag_origin->_oyb);
            }
            ImGui::EndTooltip();
        }
        else
        {
            _drag._drag_active[0] = false;
            _drag._drag_active[1] = false;
            _drag._drag_origin = nullptr;
        }
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

        _trimed_width += _spacing;
        _trimed_height += _spacing;

        return true;
    }

    bool app::save_atlas(const char* path)
    {
        msg::Var doc;
        msg::Var sprites;
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
        doc.set_item("metadata", metadata);
        doc.set_item("texture", texture);

        std::string txt;
        doc.to_string(txt);
        return SaveFileText(path, txt.data()) && r;
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
        _path.clear();
        _active    = nullptr;
        _zoom      = {1.0f};
        _heuristic = {};
        _padding   = {};
        _width     = {512};
        _height    = {512};
        _trim      = {};
        _dirty     = true;
    }

    void app::set_atlas_scale(const ImVec2& scale, const ImVec2& world_point)
    {
        matrix2d new_transform({0, 0}, {scale.x, scale.y}, {0, 0}, 0);
        auto s = _transform.transformPoint(world_point);
        auto c = s - new_transform.transformPoint(world_point);
        new_transform.tx = c.x;
        new_transform.ty = c.y;
        _transform = new_transform;
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


} // namespace box