#pragma once

#include "include.hpp"
#include <imstb_rectpack.h>

namespace box
{
	struct sprite
	{
        Image     _img{};
        Texture   _txt{};
        Rectangle _region{};
        bool      _packed{};
        int32_t   _ox{};
        int32_t   _oy{};
	};


	class app
	{
    public:
        app();
        ~app() = default;

		void show();
        void show_menu();

		void show_properties();
        void show_list();
        void show_texture();
        void show_atlas();

        bool open_atlas(const char* path);
        bool save_atlas(const char* path);
        bool add_file(const char* path);
        bool add_files();
        bool remove_file(sprite* spr);
        bool set_origin(sprite* spr, ImVec2 off) const;
        bool repack();
        void reset();

        std::map<std::string, sprite> _items;
        sprite*                       _active{};
        std::string                   _active_name;
        stbrp_context                 _context{};
        std::vector<stbrp_node>       _nodes;
        std::vector<stbrp_rect>       _rects;
        std::vector<sprite*>          _sprites;
        float                         _zoom{1.0f};
        int32_t                       _heuristic{};
        int32_t                       _padding{};
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
    };
} // namespace box