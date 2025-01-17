#pragma once

#include "include.hpp"

namespace box
{
    enum sprite_data : int32_t
    {
        Defualt,
        One = Defualt,
        Two,
        NinePatch,
    };

	struct sprite
	{
        Image     _img{};
        Texture   _txt{};
        Rectangle _region{};
        int32_t   _oxa{};
        int32_t   _oya{};
        int32_t   _oxb{};
        int32_t   _oyb{};
        int32_t   _data{sprite_data::Defualt};
        bool      _packed{};
    };

    struct composition
    {
        bool draw(size_t nde, ImDrawList* dc, matrix2d& tr);

        struct node
        {
            float         _rotation{0.f};
            point2f       _scale{1.f, 1.f};
            point2f       _position;
            const sprite* _sprite{};
            bool          _selected{};

            bool draw(ImDrawList* dc, matrix2d& tr, point2f mpos, const node* active_node) const;
        };

        std::vector<node> _nodes;
        point2f           _grag_pos;
        point2f           _mouse;
        point2f           _center;
        point2f           _rotation;
        size_t            _active_node{size_t(-1)};
        size_t            _hover_node{};
        bool              _drag{};
        bool              _rotate{};
        bool              _selected{};

        node*   get_active();
        void    update_drag(ImDrawList* dc, matrix2d& local, float zoom);
        size_t  node_index(const node* n) const;
        void    select(size_t nde, bool ctrl);
        void    select_all(float v);
        void    move_selected(point2f offset);
        void    rotate_selected(point2f center, float rotation);
        void    scale_selected(point2f center, point2f scale);
        point2f get_selected_center() const;
        void    remove_sprite(sprite* spr);
        bool    remove_node(size_t n);
        bool    reorder_node(size_t n, int32_t dir);
    };

    struct drag_data
    {
        sprite* _drag_origin{};
        point2f _drag_begin{};
        bool    _hovered_active[2]{};
        bool    _drag_active[2]{};
    };

    struct canvas_data
    {
        float    _zoom{1.0f};
        float    _ideal_zoom{1.0f};
        point2f  _ideal_offset;
        matrix2d _transform;
    };

	class app
    {
    public:
        app();
        ~app();

        void show();
        void show_menu();

        void show_properties();
        void show_atlas_properties();
        void show_composite_properties();
        void show_node_properties();
        void show_sprite_properties();
        void show_list();
        void show_composition();
        void show_texture();
        void show_canvas(canvas_data& canvas);
        bool show_align(int32_t& x, int32_t& y, float w, float h) const;

        bool open_atlas(const char* path);
        bool save_atlas(const char* path);
        bool add_file(const char* path);
        bool add_files();
        bool add_composition(const char* path);
        bool remove_composition(composition* spr);
        bool remove_file(sprite* spr);
        std::string_view get_sprite_id(const sprite* spr) const;
        const sprite* get_sprite(std::string_view spr) const;
        bool repack();
        void reset();

        void set_atlas_scale(canvas_data& canvas, const ImVec2& scale, const ImVec2& world_point);

        Image    load_cb64(msg::Var ar) const;
        msg::Var save_cb64(Image img) const;

        std::map<std::string, sprite> _items;
        std::map<std::string, composition> _compositions;
        sprite*                       _active{};
        composition*                  _active_comp{};
        composition::node             _drag_node{};
        drag_data                     _drag{};
        std::string                   _active_name;
        std::string                   _active_comp_name;
        std::vector<sprite*>          _sprites;
        int32_t                       _heuristic{};
        int32_t                       _padding{};
        int32_t                       _spacing{};
        int32_t                       _width{512};
        int32_t                       _height{512};
        int32_t                       _trimed_width{512};
        int32_t                       _trimed_height{512};
        bool                          _trim{};
        bool                          _embed{};
        bool                          _drop_node{};
        bool                          _visible_origin{};
        bool                          _visible_region{true};
        bool                          _visible_index{};
        bool                          _composite_mode{};
        bool                          _dirty{};
        std::string                   _path;
        std::string                   _str;
        std::vector<maxRectsSize>     _item_rect;
        std::vector<maxRectsPosition> _item_pos;
        canvas_data                   _atlas_canvas{};
        canvas_data                   _comp_canvas{};
        point2f                       _mouse{NAN, NAN};
        Texture                       _alpha_txt{};
    };
} // namespace box