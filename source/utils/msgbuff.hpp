#pragma once

#include <vector>
#include <string_view>
#include <string>
#include <charconv>
#include <cassert>
#include <cmath>

#define EMSG_BUFFER std::vector<char>
#define EMSG_MAX_STACK_SIZE 128

namespace box
{
    namespace msg
    {
        using Buffer = EMSG_BUFFER;
        class Value;

        enum VarError
        {
            ok,
            expecting_string,
            expecting_value,
            invalid_literal_name,
            invalid_number,
            invalid_string_char,
            invalid_string_escape,
            invalid_surrogate_pair,
            missing_colon,
            missing_comma_or_bracket,
            unexpected_character,
        };

        struct Node
        {
            enum Tag : uint8_t { Null, End, True, False, Int, Float, Id, String, Data, Array, Object, };

            constexpr Node() : _tag(Tag::Null), _size(0) {}
            constexpr Node(Tag t) : _tag(t), _size(0) {}
            constexpr Node(Tag t, uint8_t s) : _tag(t), _size(s) {}
            constexpr explicit operator char() const { return (char)(uint8_t&)(*this); }
            constexpr bool operator==(Node o) const { return ((uint8_t&)(*this)) == ((uint8_t&)(o)); }

            Tag _tag : 4;
            uint8_t _size : 4;
        };

        class Writer
        {
            struct Stack
            {
                std::pair<uint32_t, uint32_t> _data[EMSG_MAX_STACK_SIZE];
                uint32_t _size = 0;
                inline void push(uint32_t a, uint32_t b) { assert(_size < EMSG_MAX_STACK_SIZE - 1); _data[_size].first = a; _data[_size++].second = b; }
                inline std::pair<uint32_t, uint32_t> pop() { assert(_size); return _data[--_size]; }
            };
        public:
            Writer(Buffer& buff) : _target(buff) { _stack._size = 0; };

            Writer&           clear();

            Writer&           begin_array();
            Writer&           end_array();
                              
            Writer&           begin_object();
            Writer&           end_object();
              
            Writer&           key(std::string_view k);

            template <typename T>
            Writer&           operator ()(const T& v);
            template <typename T>
            Writer&           operator ()(std::string_view k, const T& v);

            template <typename T>
            Writer&           member(std::string_view k, const T& v);

            Writer&           value(const char* v, size_t s = -1);
            Writer&           value(std::string_view v);
            Writer&           value(const std::string& v);
            Writer&           value(int16_t v);
            Writer&           value(uint16_t v);
            Writer&           value(int32_t v);
            Writer&           value(uint32_t v);
            Writer&           value(int64_t v);
            Writer&           value(uint64_t v);
            Writer&           value(float v);
            Writer&           value(double v);
            Writer&           value(bool v);
            Writer&           value(nullptr_t v);
            template <typename CB>
            Writer&           value(CB cb);
            Writer&           data_value(const void* v, size_t s);
            Writer&           ptr(const void* v);
            Writer&           copy(const Value& v);

            Buffer&           data();
            const Buffer&     data() const;
        private:
            template <typename T>
            void              write(const T& v);
            void              write(const void* d, size_t s);
            void              write_node(Node::Tag t, uint8_t size);

            size_t            _nodes = 0;
            Buffer&           _target;
            Stack             _stack;
        };

        class ValMembers;
        class ValElements;

        class Value
        {
        public:
            Value();
            Value(const char* r, uint32_t d);
            Value(const Buffer& mv);
            Value(const Value& mv);

            bool              is_undefined() const;
            bool              is_null() const;
            bool              is_object() const;
            bool              is_array() const;
            bool              is_int() const;
            bool              is_int64() const;
            bool              is_float() const;
            bool              is_double() const;
            bool              is_string() const;
            bool              is_bool() const;

            bool              get(bool def) const;
            int32_t           get(int32_t def) const;
            uint32_t          get(uint32_t def) const;
            int64_t           get(int64_t def) const;
            uint64_t          get(uint64_t def) const;
            float             get(float def) const;
            double            get(double def) const;
            const char*       get(const char* def) const;
            double            get_number(double def) const;
            std::string_view  get(std::string_view def) const;
            std::string_view  str() const;
            const char*       c_str() const;
            std::string_view  data_str() const;
            template <class T>
            T*                ptr() const;

            uint32_t          size() const;

            Value             operator[](uint32_t n) const;
            Value             operator[](std::string_view k) const;

            ValMembers        members() const;
            ValElements       elements() const;

            bool              to_string(std::string& out) const;

            uint32_t          offset() const;
            const char*       storage() const;

            Node              get_node() const;
        protected:
            template <typename T>
            const T*          get_raw(uint32_t off) const;
            Value             first() const;
            Value             last() const;
            Value             next() const;

            const char*       _storage;
            uint32_t          _offset;

            friend class      ValMembers;
            friend class      ValElements;

            inline static const Node _undefined = Node(Node::Tag::End, 0);
        };



        class Pack
        {
            struct Parser;
        public:
            Pack() = default;
            Pack(const Pack& p) = default;
            Pack(Pack&& p) = default;
            Pack&             operator=(Pack&& p) = default;
            Pack&             operator=(const Pack& p) = default;

            Writer            create();
            Value             operator()() const;
            Value             get() const;

            Buffer&           data();
            const Buffer&     data() const;

            void              swap(Pack& p);

            VarError          from_string(const char* str);
            bool              to_string(std::string& str);

        protected:
            Buffer            _buffer;
        };



        class ValMembers
        {
        public:
            struct Iterator
            {
                const Iterator& operator*() const { return *this; }
                const Iterator* operator->() { return this; }
                Iterator&       operator++() { first = second.next(); second = first.next(); return *this; }
                Iterator        operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }
                friend bool     operator== (const Iterator& a, const Iterator& b) { return a.first.offset() == b.first.offset(); };
                friend bool     operator!= (const Iterator& a, const Iterator& b) { return a.first.offset() != b.first.offset(); };

                Value first;
                Value second;
            };

            ValMembers(const Value& b, const Value& e) : _begin(b), _end(e) {};
            Iterator begin() const { return Iterator{ _begin, _begin.next() }; }
            Iterator end() const { return Iterator{ _end, _end }; }
        private:
            Value _begin;
            Value _end;
        };



        class ValElements
        {
        public:
            struct Iterator
            {
                const Value& operator*() const { return _v; }
                const Value* operator->() { return &_v; }
                Iterator&    operator++() { _v = _v.next(); return *this; }
                Iterator     operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }
                friend bool  operator== (const Iterator& a, const Iterator& b) { return a._v.offset() == b._v.offset(); };
                friend bool  operator!= (const Iterator& a, const Iterator& b) { return a._v.offset() != b._v.offset(); };

                Value _v;
            };

            ValElements(const Value& b, const Value& e) : _begin(b), _end(e) {};
            Iterator begin() const { return Iterator{ _begin }; }
            Iterator end() const { return Iterator{ _end }; }
        private:
            Value _begin;
            Value _end;
        };



        struct Stream
        {
            Stream(const char* s) : _s(s) {}
            const char* _s;
            const char* c_str() const { return _s; }
            int peek() const { return static_cast<unsigned char>(*_s); }
            int getch() { return static_cast<unsigned char>(*_s++); }
            int skipws() { while (peek() == '\x20' || peek() == '\x9' || peek() == '\xD' || peek() == '\xA') getch(); return peek(); }
        };

        struct Pack::Parser
        {
            std::vector<char> _string;
            Writer _out;

            Parser(Buffer& buff) : _out(buff) {}
            constexpr bool is_digit(int c) const { return c >= '0' && c <= '9'; }
            int parse_hex(Stream& s);
            VarError parse_number(Stream& s, bool negative);
            VarError parse_string(Stream& s, std::vector<char>& v);
            VarError parse_value(Stream& s);
            VarError parse(const char* str);
        };

        inline int Pack::Parser::parse_hex(Stream& s)
        {
            int cp = 0;
            for (int i = 0; i < 4; ++i) {
                if (is_digit(s.peek()))
                    cp = (cp * 16) + (s.getch() - '0');
                else if ((s.peek() | 0x20) >= 'a' && (s.peek() | 0x20) <= 'f')
                    cp = (cp * 16) + ((s.getch() | 0x20) - 'a' + 10);
                else
                    return -1;
            }
            return cp;
        }

        inline VarError Pack::Parser::parse_number(Stream& s, bool negative)
        {
            bool intOnly = true;
            unsigned long long integer = 0;
            double significand = 0;
            int fraction = 0;
            int exponent = 0;

            integer = s.getch() - '0';
            if (integer)
                while (is_digit(s.peek()) && integer < 0x1999999999999999ull)
                    integer = (integer * 10) + (s.getch() - '0');

            if (integer >= 0x1999999999999999ull) {
                significand = static_cast<double>(integer);
                while (is_digit(s.peek()))
                    significand = (significand * 10) + (s.getch() - '0');
            }

            if (s.peek() == '.') {
                intOnly = false;
                s.getch();
                while (is_digit(s.peek()) && integer < 0x1FFFFFFFFFFFFFull) {
                    integer = (integer * 10) + (s.getch() - '0');
                    --fraction;
                }

                while (is_digit(s.peek()))
                    s.getch();
            }

            if (integer < 0x1999999999999999ull)
                significand = static_cast<double>(integer);

            if ((s.peek() | 0x20) == 'e') {
                intOnly = false;
                s.getch();
                if (s.peek() == '-') {
                    s.getch();
                    if (!is_digit(s.peek()))
                        return VarError::invalid_number;
                    while (is_digit(s.peek()))
                        exponent = (exponent * 10) - (s.getch() - '0');
                }
                else {
                    if (s.peek() == '+')
                        s.getch();
                    if (!is_digit(s.peek()))
                        return VarError::invalid_number;
                    while (is_digit(s.peek()))
                        exponent = (exponent * 10) + (s.getch() - '0');
                }
            }

            static constexpr double exp10[] = { 1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22, 1e23, 1e24, 1e25, 1e26, 1e27, 1e28, 1e29, 1e30, 1e31, 1e32, 1e33, 1e34, 1e35, 1e36, 1e37, 1e38, 1e39, 1e40, 1e41, 1e42, 1e43, 1e44, 1e45, 1e46, 1e47, 1e48, 1e49, 1e50, 1e51, 1e52, 1e53, 1e54, 1e55, 1e56, 1e57, 1e58, 1e59, 1e60, 1e61, 1e62, 1e63, 1e64, 1e65, 1e66, 1e67, 1e68, 1e69, 1e70, 1e71, 1e72, 1e73, 1e74, 1e75, 1e76, 1e77, 1e78, 1e79, 1e80, 1e81, 1e82, 1e83, 1e84, 1e85, 1e86, 1e87, 1e88, 1e89, 1e90, 1e91, 1e92, 1e93, 1e94, 1e95, 1e96, 1e97, 1e98, 1e99, 1e100, 1e101, 1e102, 1e103, 1e104, 1e105, 1e106, 1e107, 1e108, 1e109, 1e110, 1e111, 1e112, 1e113, 1e114, 1e115, 1e116, 1e117, 1e118, 1e119, 1e120, 1e121, 1e122, 1e123, 1e124, 1e125, 1e126, 1e127, 1e128, 1e129, 1e130, 1e131, 1e132, 1e133, 1e134, 1e135, 1e136, 1e137, 1e138, 1e139, 1e140, 1e141, 1e142, 1e143, 1e144, 1e145, 1e146, 1e147, 1e148, 1e149, 1e150, 1e151, 1e152, 1e153, 1e154, 1e155, 1e156, 1e157, 1e158, 1e159, 1e160, 1e161, 1e162, 1e163, 1e164, 1e165, 1e166, 1e167, 1e168, 1e169, 1e170, 1e171, 1e172, 1e173, 1e174, 1e175, 1e176, 1e177, 1e178, 1e179, 1e180, 1e181, 1e182, 1e183, 1e184, 1e185, 1e186, 1e187, 1e188, 1e189, 1e190, 1e191, 1e192, 1e193, 1e194, 1e195, 1e196, 1e197, 1e198, 1e199, 1e200, 1e201, 1e202, 1e203, 1e204, 1e205, 1e206, 1e207, 1e208, 1e209, 1e210, 1e211, 1e212, 1e213, 1e214, 1e215, 1e216, 1e217, 1e218, 1e219, 1e220, 1e221, 1e222, 1e223, 1e224, 1e225, 1e226, 1e227, 1e228, 1e229, 1e230, 1e231, 1e232, 1e233, 1e234, 1e235, 1e236, 1e237, 1e238, 1e239, 1e240, 1e241, 1e242, 1e243, 1e244, 1e245, 1e246, 1e247, 1e248, 1e249, 1e250, 1e251, 1e252, 1e253, 1e254, 1e255, 1e256, 1e257, 1e258, 1e259, 1e260, 1e261, 1e262, 1e263, 1e264, 1e265, 1e266, 1e267, 1e268, 1e269, 1e270, 1e271, 1e272, 1e273, 1e274, 1e275, 1e276, 1e277, 1e278, 1e279, 1e280, 1e281, 1e282, 1e283, 1e284, 1e285, 1e286, 1e287, 1e288, 1e289, 1e290, 1e291, 1e292, 1e293, 1e294, 1e295, 1e296, 1e297, 1e298, 1e299, 1e300, 1e301, 1e302, 1e303, 1e304, 1e305, 1e306, 1e307, 1e308 };
            exponent += fraction;
            if (exponent < 0) {
                if (exponent < -308) {
                    significand /= exp10[308];
                    exponent += 308;
                }
                significand = exponent < -308 ? 0.0 : significand / exp10[-exponent];
            }
            else {
                significand *= exp10[exponent < 308 ? exponent : 308];
            }

            if (intOnly)
                _out.value(negative ? -(long long)integer : (long long)integer);
            else
                _out.value(negative ? -significand : significand);
            return VarError::ok;
        }

        inline VarError Pack::Parser::parse_string(Stream& s, std::vector<char>& v)
        {
            v.resize(0);
            for (size_t length = 0, offset = v.size();;)
            {
                v.resize(v.size() + 32);
                char* first = v.data() + offset + length;
                char* last = v.data() + v.size();
                while (first < last)
                {
                    int ch = s.getch();

                    if (ch < ' ')
                        return VarError::invalid_string_char;

                    if (ch == '"') {
                        length = size_t(first - (v.data() + offset));
                        _out.value(std::string_view(v.data(), length));
                        return VarError::ok;
                    }

                    if (ch == '\\') {
                        switch (s.getch()) {
                            // clang-format off
                        case '\x22': ch = '"'; break;
                        case '\x2F': ch = '/'; break;
                        case '\x5C': ch = '\\'; break;
                        case '\x62': ch = '\b'; break;
                        case '\x66': ch = '\f'; break;
                        case '\x6E': ch = '\n'; break;
                        case '\x72': ch = '\r'; break;
                        case '\x74': ch = '\t'; break;
                            // clang-format on
                        case '\x75':
                            if ((ch = parse_hex(s)) < 0)
                                return VarError::invalid_string_escape;
                            if (ch >= 0xD800 && ch <= 0xDBFF) {
                                if (s.getch() != '\\' || s.getch() != '\x75')
                                    return VarError::invalid_surrogate_pair;
                                int low = parse_hex(s);
                                if (low < 0xDC00 || low > 0xDFFF)
                                    return VarError::invalid_surrogate_pair;
                                ch = 0x10000 + ((ch & 0x3FF) << 10) + (low & 0x3FF);
                            }
                            if (ch < 0x80) {
                                *first++ = (char)ch;
                            }
                            else if (ch < 0x800) {
                                *first++ = 0xC0 | ((char)(ch >> 6));
                                *first++ = 0x80 | (ch & 0x3F);
                            }
                            else if (ch < 0x10000) {
                                *first++ = 0xE0 | ((char)(ch >> 12));
                                *first++ = 0x80 | ((ch >> 6) & 0x3F);
                                *first++ = 0x80 | (ch & 0x3F);
                            }
                            else {
                                *first++ = 0xF0 | ((char)(ch >> 18));
                                *first++ = 0x80 | ((ch >> 12) & 0x3F);
                                *first++ = 0x80 | ((ch >> 6) & 0x3F);
                                *first++ = 0x80 | (ch & 0x3F);
                            }
                            continue;
                        default:
                            return VarError::invalid_string_escape;
                        }
                    }
                    *first++ = (char)ch;
                }
                length = size_t(first - (v.data() + offset));
            }
            return VarError::invalid_string_char;
        }

        inline VarError Pack::Parser::parse_value(Stream& s)
        {
            switch (s.skipws()) {
            case '"':
                s.getch();
                return parse_string(s, _string);
            case 'f':
                s.getch();
                if (s.getch() == 'a' && s.getch() == 'l' && s.getch() == 's' && s.getch() == 'e') {
                    _out.value(false);
                    return VarError::ok;
                }
                return VarError::invalid_literal_name;
            case 't':
                s.getch();
                if (s.getch() == 'r' && s.getch() == 'u' && s.getch() == 'e') {
                    _out.value(true);
                    return VarError::ok;
                }
                return VarError::invalid_literal_name;
            case 'n':
                s.getch();
                if (s.getch() == 'u' && s.getch() == 'l' && s.getch() == 'l') {
                    _out.value(nullptr);
                    return VarError::ok;
                }
                return VarError::invalid_literal_name;
            case '[': {
                s.getch();
                _out.begin_array();
                if (s.skipws() != ']') {
                div:
                    VarError r = parse_value(s);
                    if (r) return r;
                    if (s.skipws() == ',') {
                        s.getch();
                        goto div;
                    }
                }

                if (s.getch() != ']')
                    return VarError::missing_comma_or_bracket;

                _out.end_array();
                return VarError::ok;
            }
            case '{': {
                s.getch();
                _out.begin_object();
                if (s.skipws() != '}') {
                member:
                    if (s.peek() != '"')
                        return VarError::expecting_string;
                    s.getch();
                    VarError r = parse_string(s, _string);
                    if (r) return r;
                    if (s.skipws() != ':') return VarError::missing_colon;
                    s.getch();
                    r = parse_value(s);
                    if (r) return r;

                    if (s.skipws() == ',') {
                        s.getch();
                        s.skipws();
                        goto member;
                    }
                }

                if (s.getch() != '}')
                    return VarError::missing_comma_or_bracket;

                _out.end_object();
                return VarError::ok;
            }
            case '-':
                s.getch();
                if (is_digit(s.peek()))
                    return parse_number(s, true);
                break;
            default:
                if (is_digit(s.peek()))
                    return parse_number(s, false);
                break;
            }
            return VarError::expecting_value;
        }

        inline VarError Pack::Parser::parse(const char* str)
        {
            Stream s(str);
            VarError v = parse_value(s);
            if (v && s.skipws()) {
                _out.clear();
            }
            return v;
        }



        inline void Writer::write(const void* d, size_t s)
        {
            _target.insert(_target.end(), (const char*)d, (const char*)d + s);
        }

        inline void Writer::write_node(Node::Tag t, uint8_t size)
        {
            _target.push_back((char)Node(t, size));
            ++_nodes;
        }

        inline Writer& Writer::clear()
        {
            _target.clear();
            _nodes = 0;
            _stack._size = 0;
            return *this;
        }

        inline Writer& Writer::begin_array()
        {
            write_node(Node::Tag::Array, 8);
            _stack.push((uint32_t)_target.size(), (uint32_t)_nodes);
            write(0);
            write(0);
            return *this;
        }

        inline Writer& Writer::end_array()
        {
            _target.push_back((char)Node(Node::Tag::End));
            auto pair = _stack.pop();
            uint32_t* prev = ((uint32_t*)&_target[pair.first]);
            prev[0] = uint32_t(_target.size() - pair.first - 8);
            prev[1] = uint32_t(_nodes - pair.second);
            _nodes -= prev[1];
            return *this;
        }

        inline Writer& Writer::begin_object()
        {
            if (!_stack._size && _nodes == 1)
                clear();

            write_node(Node::Tag::Object, 8);
            _stack.push((uint32_t)_target.size(), (uint32_t)_nodes);
            write(0);
            write(0);
            return *this;
        }

        inline Writer& Writer::end_object()
        {
            _target.push_back((char)Node(Node::Tag::End));
            auto pair = _stack.pop();
            uint32_t* prev = ((uint32_t*)&_target[pair.first]);
            prev[0] = uint32_t(_target.size() - pair.first - 8);
            prev[1] = uint32_t(_nodes - pair.second);
            _nodes -= prev[1];
            return *this;
        }

        inline Writer& Writer::value(const char* v, size_t s)
        {
            return value(std::string_view(v, s == -1 ? strlen(v) : s));
        }

        inline Writer& Writer::key(std::string_view k)
        {
            return value(k);
        }

        inline Writer& Writer::value(std::string_view v)
        {
            if (v.size() < 15)
            {
                write_node(Node::Tag::Id, uint8_t(v.size() + 1));
                write(v.data(), v.size());
                write('\0');
            }
            else
            {
                write_node(Node::Tag::String, 4);
                write(uint32_t(v.size() + 1));
                write(v.data(), v.size());
                write('\0');
            }
            return *this;
        }

        inline Writer& Writer::value(const std::string& v)
        {
            return value(std::string_view(v));
        }

        inline Writer& Writer::value(int16_t v)
        {
            return value(int32_t(v));
        }

        inline Writer& Writer::value(uint16_t v)
        {
            return value(uint32_t(v));
        }

        inline Writer& Writer::value(int32_t v)
        {
            write_node(Node::Tag::Int, 4);
            write(v);
            return *this;
        }

        inline Writer& Writer::value(uint32_t v)
        {
            write_node(Node::Tag::Int, 4);
            write(v);
            return *this;
        }

        inline Writer& Writer::value(int64_t v)
        {
            write_node(Node::Tag::Int, 8);
            write(v);
            return *this;
        }

        inline Writer& Writer::value(uint64_t v)
        {
            write_node(Node::Tag::Int, 8);
            write(v);
            return *this;
        }

        inline Writer& Writer::value(float v)
        {
            write_node(Node::Tag::Float, 4);
            write(v);
            return *this;
        }

        inline Writer& Writer::value(double v)
        {
            write_node(Node::Tag::Float, 8);
            write(v);
            return *this;
        }

        inline Writer& Writer::value(bool v)
        {
            write_node(v ? Node::Tag::True : Node::Tag::False, 0);
            return *this;
        }

        inline Writer& Writer::value(nullptr_t v)
        {
            write_node(Node::Tag::Null, 0);
            return *this;
        }

        inline Writer& Writer::data_value(const void* v, size_t s)
        {
            write_node(Node::Tag::Data, 4);
            write(uint32_t(s));
            write(v, s);
            return *this;
        }

        inline Writer& Writer::ptr(const void* v)
        {
            write_node(Node::Tag::Int, sizeof(v));
            write(v);
            return *this;
        }

        inline Writer& Writer::copy(const Value& v)
        {
            auto node = v.get_node();
            switch (node._tag)
            {
            case Node::Null: return value(nullptr);
            case Node::True: return value(true);
            case Node::False: return value(false);
            case Node::Int: return node._size == 4 ? value(v.get(0)) : value(v.get(0ll));
            case Node::Float: return node._size == 4 ? value(v.get(0.f)) : value(v.get(0.));
            case Node::Id:
            case Node::String: return value(v.str());
            case Node::Data: return value(v.data_str());
            case Node::Array:
            {
                begin_array();
                for (auto& it : v.elements())
                    copy(it);
                return end_array();
            }
            case Node::Object:
            {
                begin_object();
                for (auto& it : v.members())
                    key(it.first.str()).copy(it.second);
                return end_object();
            }
            }
            return *this;
        }

        inline Buffer& Writer::data()
        {
            assert(!_stack._size && "Missing EndArray or EndObject!");
            assert(_nodes == 1 && "Missing EndArray or EndObject!");
            return _target;
        }

        inline const Buffer& Writer::data() const
        {
            assert(!_stack._size && "Missing EndArray or EndObject!");
            assert(_nodes == 1 && "Missing EndArray or EndObject!");
            return _target;
        }



        inline Value::Value()
            : _storage((const char*)&Value::_undefined), _offset(0)
        {
        }

        inline Value::Value(const char* r, uint32_t d)
            : _storage(r), _offset(d)
        {
        }

        inline Value::Value(const Buffer& mv)
            : _storage(mv.data()), _offset(0)
        {
            if (mv.size() < sizeof(Node) + sizeof(uint32_t))
                _storage = (const char*)&Value::_undefined;
        }

        inline Value::Value(const Value& mv)
            : _storage(mv._storage), _offset(mv._offset)
        {
        }

        inline bool Value::is_undefined() const
        {
            return get_node()._tag == Node::Tag::End;
        }

        inline bool Value::is_null() const
        {
            return get_node()._tag == Node::Tag::Null;
        }

        inline bool Value::is_object() const
        {
            return get_node()._tag == Node::Tag::Object;
        }

        inline bool Value::is_array() const
        {
            return get_node()._tag == Node::Tag::Array;
        }

        inline bool Value::is_int() const
        {
            return get_node() == Node(Node::Tag::Int, 4);
        }

        inline bool Value::is_int64() const
        {
            return get_node() == Node(Node::Tag::Int, 8);
        }

        inline bool Value::is_float() const
        {
            return get_node() == Node(Node::Tag::Float, 4);
        }

        inline bool Value::is_double() const
        {
            return get_node() == Node(Node::Tag::Float, 8);
        }

        inline bool Value::is_string() const
        {
            return get_node()._tag == Node::Tag::Id || get_node()._tag == Node::Tag::String;
        }

        inline bool Value::is_bool() const
        {
            const Node node = get_node();
            if (node._tag == Node::Tag::True || node._tag == Node::Tag::False) return true;
            return false;
        }

        inline bool Value::get(bool def) const
        {
            const Node node = get_node();
            if (node._tag == Node::Tag::True) return true;
            if (node._tag == Node::Tag::False) return false;
            return def;
        }

        inline int32_t Value::get(int32_t def) const
        {
            if (get_node() == Node(Node::Tag::Int, 4))
                return *get_raw<int32_t>(_offset + 1);
            if (get_node() == Node(Node::Tag::Int, 8))
                return int32_t(*get_raw<int64_t>(_offset + 1));
            return def;
        }

        inline uint32_t Value::get(uint32_t def) const
        {
            if (get_node() == Node(Node::Tag::Int, 4))
                return *get_raw<uint32_t>(_offset + 1);
            if (get_node() == Node(Node::Tag::Int, 8))
                return int32_t(*get_raw<uint64_t>(_offset + 1));
            return def;
        }

        inline int64_t Value::get(int64_t def) const
        {
            if (get_node() == Node(Node::Tag::Int, 8))
                return *get_raw<int64_t>(_offset + 1);
            if (get_node() == Node(Node::Tag::Int, 4))
                return *get_raw<int32_t>(_offset + 1);
            return def;
        }

        inline uint64_t Value::get(uint64_t def) const
        {
            if (get_node() == Node(Node::Tag::Int, 8))
                return *get_raw<uint64_t>(_offset + 1);
            if (get_node() == Node(Node::Tag::Int, 4))
                return *get_raw<uint32_t>(_offset + 1);
            return def;
        }

        inline float Value::get(float def) const
        {
            if (get_node() == Node(Node::Tag::Float, 4))
                return *get_raw<float>(_offset + 1);
            if (get_node() == Node(Node::Tag::Float, 8))
                return float(*get_raw<double>(_offset + 1));
            return def;
        }

        inline double Value::get(double def) const
        {
            if (get_node() == Node(Node::Tag::Float, 8))
                return *get_raw<double>(_offset + 1);
            if (get_node() == Node(Node::Tag::Float, 4))
                return *get_raw<float>(_offset + 1);
            return def;
        }

        inline const char* Value::get(const char* def) const
        {
            const Node node = get_node();
            if (node._tag == Node::Tag::Id)
                return get_raw<char>(_offset + 1);
            if (node._tag == Node::Tag::String)
                return get_raw<char>(_offset + 5);
            return def;
        }

        inline double Value::get_number(double def) const
        {
            const Node node = get_node();
            if (node == Node(Node::Tag::Float, 4))
                return *get_raw<float>(_offset + 1);
            if (node == Node(Node::Tag::Float, 8))
                return (*get_raw<double>(_offset + 1));
            if (node == Node(Node::Tag::Int, 8))
                return double(*get_raw<int64_t>(_offset + 1));
            if (node == Node(Node::Tag::Int, 4))
                return double(*get_raw<int32_t>(_offset + 1));
            return def;
        }

        inline std::string_view Value::get(std::string_view def) const
        {
            const Node node = get_node();
            if (node._tag == Node::Tag::Id)
                return std::string_view(get_raw<char>(_offset + 1), node._size - 1);
            if (node._tag == Node::Tag::String)
                return std::string_view(get_raw<char>(_offset + 5), *get_raw<uint32_t>(_offset + 1) - 1);
            return def;
        }

        inline std::string_view Value::str() const
        {
            const Node node = get_node();
            if (node._tag == Node::Tag::Id)
                return std::string_view(get_raw<char>(_offset + 1), node._size - 1);
            if (node._tag == Node::Tag::String)
                return std::string_view(get_raw<char>(_offset + 5), *get_raw<uint32_t>(_offset + 1) - 1);
            return std::string_view("");
        }

        inline const char* Value::c_str() const
        {
            const Node node = get_node();
            if (node._tag == Node::Tag::Id)
                return get_raw<char>(_offset + 1);
            if (node._tag == Node::Tag::String)
                return get_raw<char>(_offset + 5);
            return "";
        }

        inline std::string_view Value::data_str() const
        {
            const Node node = get_node();
            if (node._tag == Node::Tag::Data)
                return std::string_view(get_raw<char>(_offset + 5), *get_raw<uint32_t>(_offset + 1));
            return std::string_view();
        }

        inline uint32_t Value::size() const
        {
            const Node node = get_node();
            if (node._tag == Node::Tag::Array)
                return *get_raw<uint32_t>(_offset + 5);
            if (node._tag == Node::Tag::Object)
                return *get_raw<uint32_t>(_offset + 5) / 2;
            return 0;
        }

        inline Value Value::operator[](uint32_t n) const
        {
            if (get_node()._tag != Node::Tag::Array)
                return Value();

            Value v = first();
            for (; n; --n)
                v = v.next();

            return v;
        }

        inline Value Value::operator[](std::string_view k) const
        {
            if (get_node()._tag != Node::Tag::Object)
                return Value();

            for (Value v = first(); !v.is_undefined(); v = v.next().next())
            {
                if (v.str() == k)
                    return v.next();
            }

            return Value();
        }

        inline ValMembers Value::members() const
        {
            return ValMembers(first(), last());
        }

        inline ValElements Value::elements() const
        {
            return ValElements(first(), last());
        }

        inline bool Value::to_string(std::string& s) const
        {
            static char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

            bool ret = true;
            char buf[32];
            std::to_chars_result res;

            const Node node = get_node();
            switch (node._tag)
            {
            case Node::Tag::Null:
                s.append("null", 4);
                break;
            case Node::Tag::True:
                s.append("true", 4);
                break;
            case Node::Tag::False:
                s.append("false", 5);
                break;
            case Node::Tag::Int:
                if (node._size == 4)
                {
                    res = std::to_chars(buf, buf + std::size(buf), *get_raw<int32_t>(_offset + 1));
                    s.append(buf, res.ptr - buf);
                }
                else
                {
                    res = std::to_chars(buf, buf + std::size(buf), *get_raw<int64_t>(_offset + 1));
                    s.append(buf, res.ptr - buf);
                }
                break;
            case Node::Tag::Float:
                if (node._size == 4)
                {
                    res = std::to_chars(buf, buf + std::size(buf), *get_raw<float>(_offset + 1));
                    s.append(buf, res.ptr - buf);
                }
                else
                {
                    res = std::to_chars(buf, buf + std::size(buf), *get_raw<double>(_offset + 1));
                    s.append(buf, res.ptr - buf);
                }
            break;
            case Node::Tag::Id:
            case Node::Tag::String:
            {
                s.push_back('\"');
                for (const char* p = c_str(); *p;)
                {
                    char c = *p++;
                    switch (c) {
                    case '\b': s.append("\\b", 2); break;
                    case '\f': s.append("\\f", 2); break;
                    case '\n': s.append("\\n", 2); break;
                    case '\r': s.append("\\r", 2); break;
                    case '\t': s.append("\\t", 2); break;
                    case '\\': s.append("\\\\", 2); break;
                    case '"':  s.append("\\\"", 2); break;
                    default:   s.push_back(c);
                    }
                }
                s.push_back('\"');
            }
            break;
            case Node::Tag::Data:
            {
                auto dta = data_str();
                s.push_back('\"');
                s.push_back('#');
                for (auto& v : dta)
                {
                    s.push_back(hex_chars[uint8_t(v & 0xF0) >> 4]);
                    s.push_back(hex_chars[uint8_t(v & 0x0F) >> 0]);
                }
                s.push_back('\"');
            }
            break;
            case Node::Tag::Array:
            {
                if (size() && ret)
                {
                    char comma = '[';
                    for (auto& v : elements())
                    {
                        s.push_back(comma);
                        ret &= v.to_string(s);
                        comma = ',';
                    }
                    s.append("]", 1);
                }
                else
                    s.append("[]", 2);
            }
            break;
            case Node::Tag::Object:
            {
                if (size() && ret)
                {
                    char comma = '{';
                    for (auto& v : members())
                    {
                        s.push_back(comma);
                        s.push_back('"');
                        s.append(v.first.c_str());
                        s.push_back('"');
                        s.push_back(':');
                        ret &= v.second.to_string(s);
                        comma = ',';
                    }
                    s.append("}", 1);
                }
                else
                    s.append("{}", 2);
            }
            break;

            default:
                return false;
            }

            return ret;
        }

        inline uint32_t Value::offset() const
        {
            return _offset;
        }

        inline const char* Value::storage() const
        {
            return _storage;
        }

        inline Node Value::get_node() const
        {
            return (Node&)_storage[_offset];
        }

        inline Value Value::first() const
        {
            if (is_undefined())
                return Value();
            return Value(_storage, _offset + 9);
        }

        inline Value Value::last() const
        {
            if (is_undefined())
                return Value();
            return Value(_storage, _offset + get_node()._size + *get_raw<uint32_t>(_offset + 1));
        }

        inline Value Value::next() const
        { 
            const Node node = get_node();
            if (node._tag < Node::Tag::String)
            {
                return Value(_storage, _offset + node._size + 1);
            }
            return Value(_storage, _offset + node._size + 1 + *get_raw<uint32_t>(_offset + 1));
        }

        template<class T>
        inline T* Value::ptr() const
        {
            if constexpr (sizeof(void*) == 8)
            {
                if (get_node() == Node(Node::Tag::Int, 8))
                    return (T*)intptr_t(*get_raw<uint64_t>(_offset + 1));
            }
            else
            {
                if (get_node() == Node(Node::Tag::Int, 4))
                    return (T*)intptr_t(*get_raw<uint32_t>(_offset + 1));
            }
            return nullptr;
        }

        template<typename T>
        inline const T* Value::get_raw(uint32_t off) const
        {
            return (const T*)(&_storage[off]);
        }

        template<typename T>
        inline Writer& Writer::member(std::string_view k, const T& v)
        {
            return key(k).value(v);
        }

        template<typename T>
        inline Writer& Writer::operator()(const T& v)
        {
            return value(v);
        }

        template<typename T>
        inline Writer& Writer::operator()(std::string_view k, const T& v)
        {
            return key(k).value(v);
        }

        template<typename CB>
        inline Writer& Writer::value(CB cb)
        {
            cb(*this);
            return *this;
        }

        template<typename T>
        inline void Writer::write(const T& v)
        {
            write(&v, sizeof(v));
        }

        inline Writer Pack::create()
        {
            return Writer(_buffer);
        }

        inline Value Pack::get() const
        {
            return Value(_buffer);
        }

        inline Value Pack::operator()() const
        {
            return Value(_buffer);
        }

        inline Buffer& Pack::data()
        {
            return _buffer;
        }

        inline const Buffer& Pack::data() const
        {
            return _buffer;
        }

        inline void Pack::swap(Pack& p)
        {
            _buffer.swap(p._buffer);
        }

        inline VarError Pack::from_string(const char* str)
        {
            Parser p(_buffer);
            return p.parse(str);
        }

        inline bool Pack::to_string(std::string& str)
        {
            return get().to_string(str);
        }
    }
}
