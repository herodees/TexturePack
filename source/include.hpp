#pragma once

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "utils/msgvar.hpp"
#include "utils/math.hpp"
#include "utils/matrix2d.hpp"

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstdint>
#include <algorithm>
#include <array>
#include <set>
#include <queue>
#include <functional>
#include <chrono>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include "rlImGui.h"
#include "tinyfiledialogs.h"
#ifdef __cplusplus

extern "C"
{
#endif

#include "maxrects.h"

#ifdef __cplusplus
}
#endif

namespace std
{
	struct string_hash
	{
		using is_transparent = void;
		[[nodiscard]] size_t operator()(const char* txt) const {
			return std::hash<std::string_view>{}(txt);
		}
		[[nodiscard]] size_t operator()(std::string_view txt) const {
			return std::hash<std::string_view>{}(txt);
		}
		[[nodiscard]] size_t operator()(const std::string& txt) const {
			return std::hash<std::string>{}(txt);
		}
	};

	template <size_t N>
    struct string_literal
    {
        constexpr string_literal(const char (&str)[N])
        {
            copy_n(str, N, value);
        }

        char value[N];
    };

	inline std::vector<std::string> split(std::string_view s, std::string_view delimiter)
    {
        size_t                   pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string_view         token;
        std::vector<std::string> res;

        while ((pos_end = s.find(delimiter, pos_start)) != std::string_view::npos)
        {
            token     = s.substr(pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back(std::string(token));
        }

        res.push_back(std::string(s.substr(pos_start)));
        return res;
    }
}