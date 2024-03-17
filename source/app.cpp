#include "app.hpp"

namespace box
{
    app::app()
    {
        _nodes.resize(0x7fff);
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
            int val[] = {_width, _height};
            if (ImGui::CollapsingHeader("Atlas", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Texture size");
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputInt2("##ts", val))
                {
                    _width  = val[0] > 0 ? val[0] : 1;
                    _height = val[1] ? val[1] : 1;
                    _dirty  = true;
                }

                ImGui::Text("Padding");
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputInt("##pd", &_padding))
                {
                    _padding = _padding > 0 ? _padding : 0;
                    _dirty   = true;
                }

                ImGui::Text("Heuristics algorithm");
                ImGui::SetNextItemWidth(-1);
                if (ImGui::Combo("##hr", &_heuristic, "Skyline_BL_sortHeight\0Skyline_BF_sortHeight\0"))
                {
                    _dirty = true;
                }

                if (ImGui::Checkbox("Trim size", &_trim))
                {
                    _dirty = true;
                }

                if (ImGui::Checkbox("Embed texture", &_embed))
                {
                }
            }

            if (_active && ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Name");
                ImGui::SetNextItemWidth(-1);
                ImGui::InputText("##sn", &_active_name);
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    for (auto& el : _items)
                    {
                        if (&el.second == _active)
                        {
                            auto nodeHandler  = _items.extract(el.first);
                            nodeHandler.key() = _active_name;
                            _items.insert(std::move(nodeHandler));
                            break;
                        }
                    }
                }

                val[0] = _active->_ox;
                val[1] = _active->_oy;
                ImGui::Text("Origin");
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputInt2("##so", val))
                {
                    _active->_ox = val[0];
                    _active->_oy = val[1];
                }
            }
        }
        ImGui::EndChildFrame();

        ImGui::End();
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

        if (ImGui::BeginChildFrame(1, {-1, -1}))
        {
            for (auto& spr : _items)
            {

                ImGui::PushStyleColor(ImGuiCol_Text, spr.second._packed ? 0xffffffff : 0xff0000ff);

                if (ImGui::Selectable(spr.first.c_str(), _active == &spr.second))
                {
                    _active      = &spr.second;
                    _active_name = spr.first;
                }

                ImGui::PopStyleColor();
            }
        }
        ImGui::EndChildFrame();

        ImGui::End();
    }

    void app::show_texture()
    {
        if (ImGui::Begin("Texture", 0, ImGuiWindowFlags_NoScrollWithMouse))
        {
            ImGui::SetNextItemWidth(300);
            ImGui::SliderFloat("##zm", &_zoom, 0.1f, 5.f);
            ImGui::SameLine();
            ImGui::Button(ICON_FA_EXPAND);
            ImGui::SameLine();
            ImGui::Button(ICON_FA_COMPRESS);

            ImGui::SameLine();
            ImGui::Checkbox("Show origin", &_visible_origin);
            ImGui::SameLine();
            ImGui::Checkbox("Show region", &_visible_region);

            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

            if (ImGui::BeginChild(10, {-1, -1}, 0, ImGuiWindowFlags_NoScrollWithMouse))
            {

                show_atlas();

                if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows))
                {
                    _zoom += ImGui::GetIO().MouseWheel * 0.1f;
                    if (_zoom < 0.1f)
                        _zoom = 0.1f;
                }
            }
            ImGui::EndChild();

            ImGui::PopStyleVar(2);
        }

        ImGui::End();
    }

    void app::show_atlas()
    {
        auto   flclr = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
        auto   bgclr = ImGui::GetColorU32(ImGuiCol_NavHighlight);
        auto   dc    = ImGui::GetWindowDrawList();
        ImVec2 zoom(_zoom, _zoom);
        ImVec2 from = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetContentRegionAvail();
        ImVec2 txtsize((float)_width, (float)_height);
        ImVec2 halftxt(txtsize * zoom / ImVec2(2, 2));
        ImVec2 center(from + size / ImVec2(2, 2));
        ImVec2 origin(center - halftxt);
        ImVec2 mpos = (ImGui::GetMousePos() - origin) / zoom;

        for (auto& el : _items)
        {
            if (!el.second._packed)
                continue;

            ImVec2 pos1(el.second._region.x, el.second._region.y);
            ImVec2 pos2(el.second._region.x + el.second._region.width, el.second._region.y + el.second._region.height);
            auto   clr = bgclr;

            if (mpos.x >= pos1.x && mpos.x < pos2.x && mpos.y >= pos1.y && mpos.y < pos2.y)
            {
                clr = flclr;
                if (IsMouseButtonPressed(0))
                {
                    _active = &el.second;
                    _active_name = el.first;
                }
            }

            dc->AddImage((ImTextureID)&el.second._txt, origin + pos1 * zoom, origin + pos2 * zoom);
            
            if (_visible_region)
            {
                dc->AddRect(origin + pos1 * zoom, origin + pos2 * zoom, clr);
            }
            if (_visible_origin)
            {
                ImVec2 oo = origin + (pos1 + ImVec2(el.second._ox, el.second._oy)) * zoom;
                dc->AddLine(oo - ImVec2(0, 10), oo + ImVec2(0, 10), flclr);
                dc->AddLine(oo - ImVec2(10, 0), oo + ImVec2(10, 0), flclr);
            }
        }

        dc->AddRect(origin, center + halftxt, bgclr);

        if (_active && _active->_packed)
        {
            ImVec2 pos1(_active->_region.x, _active->_region.y);
            ImVec2 pos2(_active->_region.x + _active->_region.width, _active->_region.y + _active->_region.height);

            dc->AddRect(origin + pos1 * zoom, origin + pos2 * zoom, flclr);

            ImVec2 oo = origin + (pos1 + ImVec2(_active->_ox, _active->_oy)) * zoom;

            dc->AddLine(oo - ImVec2(0, 10), oo + ImVec2(0, 10), flclr);
            dc->AddLine(oo - ImVec2(10, 0), oo + ImVec2(10, 0), flclr);
        }
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

        _width     = metadata.get_item("width").get(_width);
        _height    = metadata.get_item("height").get(_height);
        _padding   = metadata.get_item("padding").get(_padding);
        _trim      = metadata.get_item("trim_alpha").get(_trim);
        _heuristic = metadata.get_item("heuristics").get(_heuristic);

        Image img{};
        auto  dta = texture.get_item("data");
        _embed    = dta.is_string();

        if (_embed)
        {
            int  b64size  = 0;
            auto b64data  = DecodeDataBase64((const unsigned char*)dta.c_str(), &b64size);
            int  datasize = 0;
            img.data      = DecompressData(b64data, b64size, &datasize);
            img.width     = texture.get_item("width").get(0);
            img.height    = texture.get_item("height").get(0);
            img.mipmaps   = 1;
            img.format    = RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
            MemFree(b64data);
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
            itm._ox            = el.get_item("ox").get(0);
            itm._ox            = el.get_item("oy").get(0);
            auto dta           = el.get_item("img");
            if (dta.is_object())
            {
                int  b64size     = 0;
                auto b64data     = DecodeDataBase64((const unsigned char*)dta.get_item("d").c_str(), &b64size);
                int  datasize    = 0;
                itm._img.data    = DecompressData(b64data, b64size, &datasize);
                itm._img.width   = dta.get_item("w").get(0);
                itm._img.height  = dta.get_item("h").get(0);
                itm._img.format  = dta.get_item("f").get(0);
                itm._img.mipmaps = dta.get_item("m").get(1);
                MemFree(b64data);
            }
            else
            {
                itm._img = ImageFromImage(img, itm._region);
            }
        }

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
        metadata.set_item("trim_alpha", _trim);
        metadata.set_item("heuristics", _heuristic);

        auto image = GenImageColor(_width, _height, {});
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
                int dataSize = GetPixelDataSize(itm.second._img.width, itm.second._img.height, itm.second._img.format);
                int compSize = 0;
                auto*    cmpdata = CompressData((unsigned char*)itm.second._img.data, dataSize, &compSize);
                int      b64size = 0;
                auto*    b64data = EncodeDataBase64(cmpdata, compSize, &b64size);
                msg::Var img;
                img.set_item("w", itm.second._img.width);
                img.set_item("h", itm.second._img.height);
                img.set_item("f", itm.second._img.format);
                img.set_item("m", itm.second._img.mipmaps);
                img.set_item("d", std::string_view(b64data, b64size));
                spr.set_item("img", img);
                MemFree(cmpdata);
                MemFree(b64data);
            }

            spr.set_item("w", itm.second._region.width);
            spr.set_item("h", itm.second._region.height);
            if (itm.second._ox)
            {
                spr.set_item("ox", itm.second._ox);
            }
            if (itm.second._oy)
            {
                spr.set_item("oy", itm.second._oy);
            }
        }

        auto r = false;
        texture.set_item("width", image.width);
        texture.set_item("height", image.height);
        if (_embed)
        {
            int   dataSize = GetPixelDataSize(image.width, image.height, image.format);
            int   compSize = 0;
            auto* cmpdata  = CompressData((unsigned char*)image.data, dataSize, &compSize);
            int   b64size  = 0;
            auto* b64data  = EncodeDataBase64(cmpdata, compSize, &b64size);
            texture.set_item("data", std::string_view(b64data, b64size));
            MemFree(cmpdata);
            MemFree(b64data);
        }
        else
        {
            std::string texturename(GetFileNameWithoutExt(path));
            texturename.append(".png");
            std::string txtpath = GetDirectoryPath(path);
            txtpath.append("/").append(texturename);
            r = ExportImage(image, txtpath.c_str());
            texture.set_item("file", std::string_view(texturename));
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
        _rects.clear();
        _sprites.clear();
        for (auto& el : _items)
        {
            auto& rc = _rects.emplace_back();
            rc.w     = el.second._img.width + _padding * 2;
            rc.h     = el.second._img.height + _padding * 2;
            rc.id    = (int32_t)_sprites.size();
            _sprites.emplace_back(&el.second);
        }

        stbrp_init_target(&_context, _width, _height, _nodes.data(), (int32_t)_nodes.size());
        stbrp_setup_heuristic(&_context, _heuristic);

        auto r = stbrp_pack_rects(&_context, _rects.data(), (int32_t)_rects.size());

        for (auto& el : _rects)
        {
            _sprites[el.id]->_region.x      = (float)el.x + _padding;
            _sprites[el.id]->_region.y      = (float)el.y + _padding;
            _sprites[el.id]->_region.width  = (float)_sprites[el.id]->_img.width;
            _sprites[el.id]->_region.height = (float)_sprites[el.id]->_img.height;
            _sprites[el.id]->_packed        = el.was_packed;
            if (!_sprites[el.id]->_txt.id)
            {
                _sprites[el.id]->_txt = LoadTextureFromImage(_sprites[el.id]->_img);
            }
        }

        return r == 1;
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
} // namespace box