#pragma once

#include "msgbuff.hpp"

namespace box::msg
{
		class VarMembers;
		class VarElements;

		template <typename T> inline T* MsgAlloc(size_t s)
		{ 
			// Allocate memory for `s` objects of type `T` without initializing them
			return static_cast<T*>(::operator new[](s * sizeof(T)));
		}

		template <typename T> inline T* MsgCreate()
		{
			T* p = MsgAlloc<T>(1);
			return new (p) T();
		}

		template <typename T> inline void MsgFree(T* p) { ::operator delete[](p); }

		template <typename T> inline void MsgDelete(T* p)
		{ 
			if (!p)
				return;
			p->~T();
			MsgFree(p);
		}

		template<typename T> struct Arr
		{
			uint32_t _ref = 1; uint32_t _size = 0; uint32_t _capacity = 0; T* _data = nullptr;
			~Arr() { MsgFree(_data); }
			void reserve(uint32_t n)
			{
				if (n <= _capacity) return;
				T* tmp = MsgAlloc<T>(n);
				if (_data) { ::memcpy(tmp, _data, _size * sizeof(T)); MsgFree(_data); }
				_data = tmp;
				::memset(_data + _capacity, 0, sizeof(T) * (n - _capacity));
				_capacity = n;
			}
			void push_back(const T& x) { reserve((_size + 4) & ~3); new(&_data[_size++])T(x); }
			void resize(uint32_t n) { reserve(n); _size = n; }
			T& back() { return _data[_size - 1]; }
			void erase(uint32_t n)
			{
				_data[n].~T();
				::memmove(_data + n, _data + n + 1, sizeof(T) * (--_size - n));
			}
			void erase(uint32_t n, uint32_t c)
			{
				for (uint32_t i = 0; i < c; i++)
					_data[n + i].~T();
				::memmove(_data + n, _data + n + c, sizeof(T) * (_size - n - c));
				_size -= c;
			}
		};

        class Var;
        struct Params;

		struct VarBase
		{
			enum class Tag { Undefined, Null, Int32, Int64, Flt32, Flt64, Bool, Function, Id, String, Array, Object  };
			using ArrData = Arr<Var>;

			using Fnc = void(*)(Params&);

			VarBase() = default;
			explicit VarBase(VarError err) : _tag(Tag::Undefined), _u64(err) {}
			explicit VarBase(int32_t v) : _tag(Tag::Int32), _u32(v) {}
			explicit VarBase(int64_t v) : _tag(Tag::Int64), _u64(v) {}
			explicit VarBase(uint32_t v) : _tag(Tag::Int32), _u32(v) {}
			explicit VarBase(uint64_t v) : _tag(Tag::Int64), _u64(v) {}
			explicit VarBase(nullptr_t) : _tag(Tag::Null), _u64(0) {}
			explicit VarBase(float v) : _tag(Tag::Flt32), _flt32(v) {}
			explicit VarBase(double v) : _tag(Tag::Flt64), _flt64(v) {}
			explicit VarBase(bool v) : _tag(Tag::Bool), _bool(v) {}
			explicit VarBase(Fnc v) : _tag(Tag::Function), _fnc(v){}

			Tag _tag;
			union
			{
				int64_t _u64;
				int32_t _u32;
				int16_t _u16;
				int8_t _u8;
				double _flt64;
				float _flt32;
				bool _bool;
                Fnc _fnc;
				ArrData* _arr;
				char _str[sizeof(double)];
			};
		};

		class Var : VarBase
		{
			struct Parser;
		public:
			using Member = std::pair<Var, Var>;

			~Var() { clear(); }
			Var() : VarBase{} {}
			Var(Var&& v) : VarBase{ v } { v._u64 = 0; v._tag = Tag::Undefined; }
			Var(const Var& v) : VarBase{ v } { if (_tag > Tag::Id) ++_arr->_ref; }
			Var(auto v) : VarBase{ v } {}
			Var(std::string_view v) : VarBase{} { (v.size() < sizeof(_u64)) ? setid(v) : setstr(v); }
			Var(const std::string& v) : Var(std::string_view(v)) {}
			Var(const char* v) : Var(std::string_view(v)) {}

			bool             is_undefined() const;
			bool             is_error() const;
			bool             is_null() const;
			bool             is_object() const;
			bool             is_array() const;
			bool             is_int() const;
			bool             is_int64() const;
			bool             is_float() const;
			bool             is_double() const;
			bool             is_string() const;
			bool             is_bool() const;
			bool             is_function() const;

			bool             get(bool def) const;
			int32_t          get(int32_t def) const;
			uint32_t         get(uint32_t def) const;
			int64_t          get(int64_t def) const;
			uint64_t         get(uint64_t def) const;
			float            get(float def) const;
			double           get(double def) const;
			const char*      get(const char* def) const;
			std::string_view get(std::string_view def) const;
            double           get_number(double def) const;

			std::string_view str() const;
			const char* c_str() const;

			Var call(Var* atts, size_t attc);

            template <typename... Args, typename = std::enable_if_t<(std::is_constructible_v<Var, Args> && ...)>>
            Var call(Args&&... args)
            {
                Var arguments[] = {std::forward<Args>(args)...};
                return call(arguments, sizeof...(args));
            }

#if defined(__cpp_char8_t)
			const char8_t* ch8_str() const;
			const char8_t* get(const char8_t* def) const;
#endif
		
			void             push_back(auto v) { push_back(Var(v)); };
			void             push_back(const Var& v);
			void             push_back(std::string_view key, const Var& v);

			Var              get_item(uint32_t n);
			void             set_item(uint32_t n, auto v) { set_item(n, Var(v)); }
			void             set_item(uint32_t n, const Var& v);

			Var              get_item(std::string_view key);
			Var              get_key(uint32_t n);
			void             set_item(std::string_view key, auto v) { set_item(key, Var(v)); }
			void             set_item(std::string_view key, const Var& v);

			void             make_array(uint32_t s);
			void             make_object(uint32_t s);

			uint32_t         size() const;
			void             clear();

			Var              operator[](uint32_t n);
			Var              operator[](std::string_view k);
			Var& operator=(const Var& c);

			VarMembers       members() const;
			VarElements      elements() const;

			VarError         from_string(const char* str);
			bool             to_string(std::string& str, bool pretty = false, uint32_t indent = 0);

			VarError         from_msg(const Value& in);
			bool             to_msg(Writer& out);

			VarError         error() const;

			Var              clone() const;
            void             erase();
			void             erase(uint32_t n);
			void             erase(std::string_view key);
			Tag              get_tag() const;
		private:
			void             setstr(std::string_view v);
			void             setid(std::string_view v);
			Var              from_msg_value(const Value& in);
		};

		struct Params
        {
            Var*   argv;
            size_t argc;
            Var    retval;
        };

		class VarMembers
		{
		public:
			constexpr VarMembers(Var::Member* b, Var::Member* e) : _begin(b), _end(e) {};
			constexpr Var::Member* begin() const { return _begin; }
			constexpr Var::Member* end() const { return _end; }
		private:
			Var::Member* _begin;
			Var::Member* _end;
		};



		class VarElements
		{
		public:
			constexpr VarElements(Var* b, Var* e) : _begin(b), _end(e) {};
			constexpr Var* begin() const { return _begin; }
			constexpr Var* end() const { return _end; }
		private:
			Var* _begin;
			Var* _end;
		};


		

		struct Var::Parser
		{
			Arr<char> _string;
			Arr<Var> _backlog;

			constexpr bool is_digit(int c) const { return c >= '0' && c <= '9'; }
			int parse_hex(Stream& s);
			Var parse_number(Stream& s);
			Var parse_string(Stream& s, Arr<char>& v);
			Var parse_value(Stream& s);
			Var parse(const char* str);

			Var make(bool arr, Var* p, uint32_t s);
		};



		inline int Var::Parser::parse_hex(Stream& s)
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

		inline Var Var::Parser::parse_number(Stream& s)
		{
			bool intOnly = true;
			uint64_t integer = 0;
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
						return Var();
					while (is_digit(s.peek()))
						exponent = (exponent * 10) - (s.getch() - '0');
				}
				else {
					if (s.peek() == '+')
						s.getch();
					if (!is_digit(s.peek()))
						return Var();
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
				return Var(integer);
			return Var(significand);
		}

		inline Var Var::Parser::parse_string(Stream& s, Arr<char>& v)
		{
			v.resize(0);

			for (uint32_t length = 0, offset = v._size;;)
			{
				v.resize(v._size + 32);

				char* first = v._data + offset + length;
				char* last = v._data + v._size;

				while (first < last) {
					int ch = s.getch();

					if (ch < ' ')
						return Var(VarError::invalid_string_char);

					if (ch == '"') {
						length = uint32_t(first - (v._data + offset));
						return Var(std::string_view(v._data, length));
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
								return Var(VarError::invalid_string_escape);
							if (ch >= 0xD800 && ch <= 0xDBFF) {
								if (s.getch() != '\\' || s.getch() != '\x75')
									return Var(VarError::invalid_surrogate_pair);
								int low = parse_hex(s);
								if (low < 0xDC00 || low > 0xDFFF)
									return Var(VarError::invalid_surrogate_pair);
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
							return Var(VarError::invalid_string_escape);
						}
					}

					*first++ = (char)ch;
				}
				length = uint32_t(first - (v._data + offset));
			}
		}

		inline Var Var::Parser::parse_value(Stream& s)
		{
			switch (s.skipws()) {
			case '"':
				s.getch();
				return parse_string(s, _string);
			case 'f':
				s.getch();
				if (s.getch() == 'a' && s.getch() == 'l' && s.getch() == 's' && s.getch() == 'e')
					return Var(false);
				return Var(VarError::invalid_literal_name);
			case 't':
				s.getch();
				if (s.getch() == 'r' && s.getch() == 'u' && s.getch() == 'e')
					return Var(true);
				return Var(VarError::invalid_literal_name);
			case 'n':
				s.getch();
				if (s.getch() == 'u' && s.getch() == 'l' && s.getch() == 'l')
					return Var(nullptr);
				return Var(VarError::invalid_literal_name);
			case '[': {
				s.getch();
				uint32_t frame = _backlog._size;
				if (s.skipws() != ']') {
				element:
					_backlog.push_back(parse_value(s));
					if (_backlog.back().is_error())
						return Var(_backlog.back());

					if (s.skipws() == ',') {
						s.getch();
						goto element;
					}
				}

				if (s.getch() != ']')
					return Var(VarError::missing_comma_or_bracket);

				uint32_t size = uint32_t(_backlog._size - frame);
				if (!size)
					return make(true, nullptr, 0);

				Var a = make(true, _backlog._data + frame, size);
				_backlog.resize(frame);
				return a;
			}
			case '{': {
				s.getch();
				uint32_t frame = _backlog._size;
				if (s.skipws() != '}') {
				member:
					if (s.peek() != '"')
						return Var(VarError::expecting_string);
					s.getch();
					_backlog.push_back(parse_string(s, _string));
					if (_backlog.back().is_error())
						return _backlog.back();

					if (s.skipws() != ':')
						return Var(VarError::missing_colon);
					s.getch();
					_backlog.push_back(parse_value(s));
					if (_backlog.back().is_error())
						return _backlog.back();

					if (s.skipws() == ',') {
						s.getch();
						s.skipws();
						goto member;
					}
				}

				if (s.getch() != '}')
					return Var(VarError::missing_comma_or_bracket);

				uint32_t size = _backlog._size - frame;
				if (!size) return make(false, nullptr, 0);

				Var a = make(false, _backlog._data + frame, size);
				_backlog.resize(frame);

				return a;
			}
			case '-':
				s.getch();
				if (is_digit(s.peek()))
				{
					Var n = parse_number(s);
					if (n.is_double())
						return Var(-n.get(0.0));
					if (n.is_int64())
						return Var(-n.get(0ll));
					return n;
				}
				break;
			default:
				if (is_digit(s.peek()))
					return parse_number(s);
				break;
			}
			return Var(VarError::expecting_value);
		}

		inline Var Var::Parser::parse(const char* str)
		{
			Stream s(str);
			Var v = parse_value(s);
			if (v.is_error() || s.skipws())
				v = Var(VarError::unexpected_character);
			for (uint32_t n = 0; n < _backlog._size; ++n)
				_backlog._data[n].clear();
			return v;
		}

		inline Var Var::Parser::make(bool arr, Var* p, uint32_t s)
		{
			Var v;
			if (arr)
				v.make_array(s);
			else
				v.make_object(s / 2);
			v._arr->_size = s;
			::memcpy(v._arr->_data, p, s * sizeof(Var));
			return v;
		}

		inline void Var::clear()
		{
			if (_tag > Tag::String)
			{
				if (--_arr->_ref == 0)
				{
					auto* eit = _arr->_data + _arr->_size;
					for (auto* it = _arr->_data; it != eit; ++it)
						it->clear();
					MsgDelete(_arr);
				}
			}
			else if (_tag == Tag::String)
			{
				if (--_arr->_ref == 0)
					MsgDelete(_arr);
			}
			_tag = Tag::Undefined;
			_u64 = 0;
		}

		inline VarError Var::error() const
		{
			if (_tag == Tag::Undefined)
				return VarError(_u64);
			return VarError::ok;
		}

		inline std::string_view Var::str() const
		{
			if (_tag == Tag::String)
				return std::string_view((char*)_arr->_data, _arr->_size);
			if (_tag == Tag::Id)
				return std::string_view(_str);
			return std::string_view();
		}

		inline const char* Var::c_str() const
		{
			if (_tag == Tag::String)
				return (char*)_arr->_data;
			if (_tag == Tag::Id)
				return _str;
			return "";
        }

        inline Var Var::call(Var* atts, size_t attc)
        {
            if (!is_function())
				return Var();
            Params p{atts, attc};
			_fnc(p);
            return p.retval;
        }

#if defined(__cpp_char8_t)
		inline const char8_t* Var::ch8_str() const {
			return (const char8_t*)c_str();
		}
#endif

		inline void Var::setstr(std::string_view v)
		{
			_tag = Tag::String;
			_arr = MsgCreate<ArrData>();
			_arr->_size = (uint32_t)v.size();
			const uint32_t s = _arr->_size + 1;
			_arr->reserve(s / sizeof(Var) + ((s % sizeof(Var)) != 0));
			memcpy(_arr->_data, v.data(), v.size());
		}

		inline void Var::setid(std::string_view v)
		{
			_tag = Tag::Id;
			memcpy(_str, v.data(), v.size());
		}

		inline void Var::push_back(const Var& v)
		{
			if (_tag != Tag::Array)
			{
				clear();
				_tag = Tag::Array;
				_arr = MsgCreate<ArrData>();
			}
			_arr->reserve((_arr->_size + 4) & ~3);
			new(&_arr->_data[_arr->_size]) Var(v);
			++_arr->_size;
		}

		inline void Var::set_item(std::string_view key, const Var& v)
		{
			if (_tag != Tag::Object)
			{
				clear();
				_tag = Tag::Object;
				_arr = MsgCreate<ArrData>();
			}
			for (Member& it : members())
			{
				if (key == it.first.c_str()) {
					it.second = v;
					return;
				}
			}
			_arr->reserve((_arr->_size + 5) & ~3);
			new(&_arr->_data[_arr->_size]) Var(key);
			++_arr->_size;
			new(&_arr->_data[_arr->_size]) Var(v);
			++_arr->_size;
		}

		inline Var Var::get_item(std::string_view key)
		{
			if (_tag != Tag::Object)
				return Var();
			auto* eit = _arr->_data + _arr->_size;
			for (auto* it = _arr->_data; it != eit; it += 2) {
				if (key == it->c_str())
					return it[1];
			}
			return Var();
		}

		inline void Var::push_back(std::string_view key, const Var& v)
		{
			if (_tag != Tag::Object)
			{
				clear();
				_tag = Tag::Object;
				_arr = MsgCreate<ArrData>();
			}
			_arr->reserve((_arr->_size + 5) & ~3);
			new(&_arr->_data[_arr->_size]) Var(key);
			++_arr->_size;
			new(&_arr->_data[_arr->_size]) Var(v);
			++_arr->_size;
		}

		inline Var Var::get_key(uint32_t n)
		{
			if (_tag == Tag::Object)
				return _arr->_data[n * 2];
			return Var();
		}

		inline void Var::make_array(uint32_t s)
		{
			if (_tag != Tag::Array)
			{
				clear();
				_tag = Tag::Array;
				_arr = MsgCreate<ArrData>();
			}
			_arr->reserve(s);
		}

		inline void Var::make_object(uint32_t s)
		{
			if (_tag != Tag::Object)
			{
				clear();
				_tag = Tag::Object;
				_arr = MsgCreate<ArrData>();
			}
			_arr->reserve(s * 2);
		}

		inline Var Var::operator[](uint32_t n)
		{
			return get_item(n);
		}

		inline Var Var::operator[](std::string_view k)
		{
			return get_item(k);
		}

		inline void Var::set_item(uint32_t n, const Var& v)
		{
			if (_tag != Tag::Array)
			{
				clear();
				_tag = Tag::Array;
				_arr = MsgCreate<ArrData>();
			}
			if (_arr->_size <= n) {
				auto s = (n + 4) & ~3;
				_arr->reserve(s);
				_arr->_size = n + 1;
			}
			else
				_arr->_data[n].clear();

			new(&_arr->_data[n]) Var(v);
		}

		inline Var Var::get_item(uint32_t n)
		{
			if (_tag == Tag::Array)
				return _arr->_data[n];
			if (_tag == Tag::Object)
				return _arr->_data[n * 2 + 1];
			return Var();
		}

		inline Var& Var::operator=(const Var& c)
		{
			if (&c == this) return *this;
			clear();
			_tag = c._tag;
			_u64 = c._u64;
			if (c._tag > Tag::Id)
				++_arr->_ref;
			return *this;
		}

		inline VarMembers Var::members() const
		{
			if (_tag == Tag::Object)
				return VarMembers((Member*)_arr->_data, (Member*)(_arr->_data + _arr->_size));
			return VarMembers(nullptr, nullptr);
		}

		inline VarElements Var::elements() const
		{
			if (_tag == Tag::Array)
				return VarElements(_arr->_data, _arr->_data + _arr->_size);
			return VarElements(nullptr, nullptr);
		}

		inline VarError Var::from_string(const char* str)
		{
			Parser p;
			*this = p.parse(str);
			return error();
		}

		inline bool Var::to_string(std::string& s, bool pretty, uint32_t indent)
		{
			bool ret = true;
			char buf[32];
			std::to_chars_result res;

			auto pushPretty = [](std::string& s, uint32_t indent) {
				s.push_back('\n');
				for (uint32_t i = 0; i < indent; i++) {
					s.push_back('\t');
				}
			};

			switch (_tag)
			{
			case Tag::Undefined:
			case Tag::Null:
				s.append("null", 4);
				break;
			case Tag::Int32:
				res = std::to_chars(buf, buf + std::size(buf), _u32);
				s.append(buf, res.ptr - buf);
				break;
			case Tag::Int64:
				res = std::to_chars(buf, buf + std::size(buf), _u64);
				s.append(buf, res.ptr - buf);
				break;
			case Tag::Flt32:
				// nan, inf, -inf is not supported by JSON format
				if (std::isnormal(_flt32) || _flt32 == 0.0f)
				{
					res = std::to_chars(buf, buf + std::size(buf), _flt32);
					s.append(buf, res.ptr - buf);
				}
				else {
					s.append("null", 4);
				}
				break;
			case Tag::Flt64:
				// nan, inf, -inf is not supported by JSON format
				if (std::isnormal(_flt64) || _flt64 == 0.0)
				{
					res = std::to_chars(buf, buf + std::size(buf), _flt64);
					s.append(buf, res.ptr - buf);
				}
				else {
					s.append("null", 4);
				}
				break;
			case Tag::Bool:
				_bool ? s.append("true", 4) : s.append("false", 5);
				break;
			case Tag::Id:
			case Tag::String:
			{
				s.push_back('\"');
				std::string_view val(str());
				const char* end = val.data() + val.size();
				for (const char* p = val.data(); p < end; ++p)
				{
					char c = *p;
					switch (c) {
					case '\b': s.append(R"(\b)", 2); break;    // Escape backspace
					case '\f': s.append(R"(\f)", 2); break;    // Escape form feed
					case '\n': s.append(R"(\n)", 2); break;    // Escape newline
					case '\r': s.append(R"(\r)", 2); break;    // Escape carriage return
					case '\t': s.append(R"(\t)", 2); break;    // Escape tab
					case '\\': s.append(R"(\\)", 2); break;   // Escape backslash
					case '\"': s.append(R"(\")", 2); break;   // Escape double quote
					default:
						if (static_cast<unsigned char>(c) < 0x20) {
							// Escape control characters below ASCII 0x20 as \u00XX
							s.append(R"(\u00)");
							s.push_back("0123456789ABCDEF"[std::to_integer<int>((std::byte(c) >> 4) & std::byte(0xF))]);
							s.push_back("0123456789ABCDEF"[std::to_integer<int>(std::byte(c) & std::byte(0xF))]);
						}
						else {
							s.push_back(c); // Add regular character as-is
						}
						break;
					}
				}
				s.push_back('\"');
			}
			break;

			case Tag::Array:
			{
				if (_arr->_size)
				{
					indent++;
					char comma = '[';
					for (auto& it : elements())
					{
						s.push_back(comma);
						if (pretty)
							pushPretty(s, indent);
						ret &= it.to_string(s, pretty, indent);
						comma = ',';
					}
					indent--;
					if (pretty)
						pushPretty(s, indent);
					s.push_back(']');
				}
				else {
					s.append("[]", 2);
				}
			}
			break;

			case Tag::Object:
			{
				if (_arr->_size)
				{
					indent++;
					char comma = '{';
					for (auto& it : members())
					{
						s.push_back(comma);
						if (pretty)
							pushPretty(s, indent);
						ret &= it.first.to_string(s, pretty, indent);
						s.push_back(':');
						ret &= it.second.to_string(s, pretty, indent);
						comma = ',';
					}
					indent--;
					if (pretty)
						pushPretty(s, indent);
					s.push_back('}');
				}
				else {
					s.append("{}", 2);
				}
			}
			break;

			default:
				return false;
			}

			return ret;
		}

		inline Var Var::from_msg_value(const Value& in)
		{
			const auto node = in.get_node();
			switch (node._tag)
			{
			case Node::Tag::Null:
				return nullptr;
			case Node::Tag::True:
				return true;
			case Node::Tag::False:
				return false;
			case Node::Tag::Int:
				switch (node._size)
				{
				case 1: return in.get(int8_t(0));
				case 2: return in.get(int16_t(0));
				case 4: return in.get(int32_t(0));
				case 8: return in.get(int64_t(0));
				}
				return VarError::invalid_number;
			case Node::Tag::Float:
				switch (node._size)
				{
				case 4: return in.get(0.f);
				case 8: return in.get(0.);
				}
				return VarError::invalid_number;
			case Node::Tag::Id:
			case Node::Tag::String:
				return in.str();
			case Node::Tag::Array:
			{
				Var r;
				r.make_array(in.size());
				for (auto& it : in.elements())
					r.push_back(from_msg_value(it));
				return r;
			}
			case Node::Tag::Object:
			{
				Var r;
				r.make_object(in.size());
				for (auto& it : in.members())
					r.push_back(it.first.str(), from_msg_value(it.second));
				return r;
			}
			}
			return VarError::expecting_value;
		}

		inline VarError Var::from_msg(const Value& in)
		{
			*this = from_msg_value(in);
			return error();
		}

		inline bool Var::to_msg(Writer& out)
		{
			bool ret = true;
			switch (_tag)
			{
			case Tag::Null: out.value(nullptr); return true;
			case Tag::Bool: out.value(_bool); return true;
			case Tag::Int32: out.value(_u32); return true;
			case Tag::Int64: out.value(_u64); return true;
			case Tag::Flt32: out.value(_flt32); return true;
			case Tag::Flt64: out.value(_flt64); return true;
			case Tag::Id: out.value(std::string_view(_str)); return true;
			case Tag::String: out.value(std::string_view((char*)_arr->_data, _arr->_size)); return true;
			case Tag::Array:
				out.begin_array();
				for (auto& it : elements())
					ret &= it.to_msg(out);
				out.end_array();
				return ret;
			case Tag::Object:
				out.begin_object();
				for (auto& it : members()) {
					ret &= it.first.to_msg(out);
					ret &= it.second.to_msg(out);
				}
				out.end_object();
				return ret;
			}
			return false;
		}

		inline uint32_t Var::size() const
		{
			if (_tag == Tag::Array)
				return _arr->_size;
			if (_tag == Tag::Object)
				return _arr->_size / 2;
			return 0;
		}

		inline bool Var::is_undefined() const
		{
			return _tag == Tag::Undefined;
		}

		inline bool Var::is_error() const
		{
			return _tag == Tag::Undefined && _u64;
		}

		inline bool Var::is_null() const
		{
			return _tag == Tag::Null;
		}

		inline bool Var::is_object() const
		{
			return _tag == Tag::Object;
		}

		inline bool Var::is_array() const
		{
			return _tag == Tag::Array;
		}

		inline bool Var::is_int() const
		{
			return _tag == Tag::Int32;
		}

		inline bool Var::is_int64() const
		{
			return _tag == Tag::Int64;
		}

		inline bool Var::is_float() const
		{
			return _tag == Tag::Flt32;
		}

		inline bool Var::is_double() const
		{
			return _tag == Tag::Flt64;
		}

		inline bool Var::is_string() const
		{
			return _tag == Tag::Id || _tag == Tag::String;
		}

		inline bool Var::is_bool() const
		{
			return _tag == Tag::Bool;
        }

        inline bool Var::is_function() const
        {
            return _tag == Tag::Function;
        }

		inline bool Var::get(bool def) const
		{
			if (_tag == Tag::Bool)
				return _bool;
			return def;
		}

		inline int32_t Var::get(int32_t def) const
		{
			if (_tag == Tag::Int32)
				return _u32;
			if (_tag == Tag::Int64)
				return int32_t(_u64);
			return def;
		}

		inline uint32_t Var::get(uint32_t def) const
		{
			if (_tag == Tag::Int32)
				return _u32;
			if (_tag == Tag::Int64)
				return uint32_t(_u64);
			return def;
		}

		inline int64_t Var::get(int64_t def) const
		{
			if (_tag == Tag::Int64)
				return _u64;
			if (_tag == Tag::Int32)
				return _u32;
			return def;
		}

		inline uint64_t Var::get(uint64_t def) const
		{
			if (_tag == Tag::Int64)
				return _u64;
			if (_tag == Tag::Int32)
				return _u32;
			return def;
		}

		inline float Var::get(float def) const
		{
			if (_tag == Tag::Flt32)
				return _flt32;
			if (_tag == Tag::Flt64)
				return float(_flt64);
			return def;
		}

		inline double Var::get(double def) const
		{
			if (_tag == Tag::Flt64)
				return _flt64;
			if (_tag == Tag::Flt32)
				return _flt32;
			return def;
		}

		inline const char* Var::get(const char* def) const
		{
			if (_tag == Tag::String)
				return (char*)_arr->_data;
			if (_tag == Tag::Id)
				return _str;
			return def;
		}

#if defined(__cpp_char8_t)
		inline const char8_t* Var::get(const char8_t* def) const
		{
			return (const char8_t*)get((const char*)def);
		}
#endif

		inline std::string_view Var::get(std::string_view def) const
		{
			if (_tag == Tag::String)
				return std::string_view((char*)_arr->_data, _arr->_size);
			if (_tag == Tag::Id)
				return _str;
			return def;
        }

        inline double Var::get_number(double def) const
        {
            if (_tag == Tag::Int32)
                return _u32;
            if (_tag == Tag::Int64)
                return double(_u64);
            if (_tag == Tag::Flt32)
                return _flt32;
            if (_tag == Tag::Flt64)
                return _flt64;
            return def;
        }

		inline Var::Tag Var::get_tag() const
		{
			return _tag;
		}

		inline Var Var::clone() const
		{
			if (this->is_array())
			{
				Var new_arr;
				new_arr.make_array(this->size());
				for (auto& k : this->elements())
					new_arr.push_back(k.clone());
				return new_arr;
			}
			else if (this->is_object())
			{
				Var new_obj;
				new_obj.make_object(this->size());
				for (auto& k : this->members())
					new_obj.set_item(k.first.c_str(), k.second.clone());
				return new_obj;
			}
			else if (this->is_string())
			{
				return Var(this->str());
			}
			return *this;
		}

        inline void Var::erase()
        {
            if (_tag == Tag::Array)
                this->_arr->resize(0);
            else if (_tag == Tag::Object)
                this->_arr->resize(0);
        }

		inline void Var::erase(uint32_t n)
		{
			if (_tag == Tag::Array)
				this->_arr->erase(n);
			else if (_tag == Tag::Object)
				this->_arr->erase(n, 2);
		}

		inline void Var::erase(std::string_view key)
		{
			if (_tag != Tag::Object)
				return;
			uint32_t n = 0;
			for (auto& k : this->members())
			{
				if (k.first.str() == key)
				{
					this->_arr->erase(n, 2);
					break;
				}
				n += 2;
			}
		}
}