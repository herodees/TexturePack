#pragma once

#include "include.hpp"

namespace box
{
	struct sprite
	{
        Image     _img{};
        Texture   _txt{};
        Rectangle _region{};
        int32_t   _oxa{};
        int32_t   _oya{};
        int32_t   _oxb{};
        int32_t   _oyb{};
        int32_t   _data{};
        bool      _packed{};
	};


	class app
    {
    public:
        app();
        ~app();

        void show();
        void show_menu();

        void show_properties();
        void show_list();
        void show_texture();
        void show_canvas(matrix2d& transform);

        bool open_atlas(const char* path);
        bool save_atlas(const char* path);
        bool add_file(const char* path);
        bool add_files();
        bool remove_file(sprite* spr);
        bool set_origin(sprite* spr, ImVec2 off) const;
        bool repack();
        void reset();

        void set_atlas_scale(const ImVec2& scale, const ImVec2& world_point);

        Image    load_cb64(msg::Var ar) const;
        msg::Var save_cb64(Image img) const;

        std::map<std::string, sprite> _items;
        sprite*                       _active{};
        sprite*                       _drag_origin{};
        point2f                       _drag_begin;
        bool                          _hovered_active{};
        std::string                   _active_name;
        std::vector<sprite*>          _sprites;
        float                         _zoom{1.0f};
        float                         _ideal_zoom{1.0f};
        point2f                       _ideal_offset;
        int32_t                       _heuristic{};
        int32_t                       _padding{};
        int32_t                       _spacing{};
        int32_t                       _width{512};
        int32_t                       _height{512};
        int32_t                       _trimed_width{512};
        int32_t                       _trimed_height{512};
        bool                          _trim{};
        bool                          _embed{};
        bool                          _visible_origin{};
        bool                          _visible_region{true};
        bool                          _dirty{};
        std::string                   _path;
        std::string                   _str;
        std::vector<maxRectsSize>     _item_rect;
        std::vector<maxRectsPosition> _item_pos;
        matrix2d                      _transform;
        point2f                       _mouse{NAN, NAN};
        Texture                       _alpha_txt{};
    };
} // namespace box