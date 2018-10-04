#ifndef MAIN_H
#define MAIN_H

#define ENABLE_LOGGER 1
#define ENABLE_SLANG_SHADER 1

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <d3d10.h>
#define VK_VALUE_BEGIN 1
#define VK_VALUE_END (VK_OEM_CLEAR + 1)

#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <deque>
#include <string>
#include <string_view>
#define _tstring std::basic_string<TCHAR>
#define _tstring_view std::basic_string_view<TCHAR>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <type_traits>
#include <limits>
#include <algorithm>
#include <codecvt>

#define CONCAT_BASE(a, b) a ## b
#define CONCAT(a, b) CONCAT_BASE(a, b)
#define STRINGIFY_BASE(n) #n
#define STRINGIFY(n) STRINGIFY_BASE(n)

struct _tstring_view_icmp {
    bool operator()(const _tstring_view &a, const _tstring_view &b) const;
};
#define MAP_ENUM(t) std::map<_tstring_view, t, _tstring_view_icmp>
#define MAP_ENUM_ITEM(n) { _T(#n), n },
#define ENUM_MAP(t) std::map<t, std::string>
#define ENUM_MAP_ITEM(n) { n, #n },
#define FLAG_MAP(t) std::vector<std::pair<t, std::string>>
#define ENUM_MAP_ITEM(n) { n, #n },

#define MOD_NAME "MMXLC Interpolation Mod"
#define LOG_FILE_NAME _T("interp-mod.log")
#define INI_FILE_NAME _T("interp-mod.ini")
#define BASE_DLL_NAME _T("\\dinput8.dll")

#endif
