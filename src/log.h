#ifndef LOG_H
#define LOG_H

#include "main.h"

class MyID3D10Buffer;
class MyID3D10Texture1D;
class MyID3D10Texture2D;
class MyID3D10Texture3D;
class MyID3D10ShaderResourceView;
class MyID3D10RenderTargetView;
class MyID3D10DepthStencilView;
class Config;
class Overlay;

template<class T>
struct LogItem;
// Example:
// struct LogItem<T> {
//     const T* t;
//     void log_item(Logger *logger) const;
// };

template<size_t N>
struct LogSkip {
    const bool skip;
    operator bool() const { return skip; }
    LogSkip<N - 1> operator*() const {
        return {skip};
    }
};
template<size_t N>
struct LogIf : LogSkip<N> {
    template<class T, std::enable_if_t<std::is_constructible_v<T, bool>, int> = 0>
    explicit LogIf(T cond) : LogSkip<N>{!cond} {}
};

template<class T>
struct DefaultLogger {
    const T &a;
    explicit DefaultLogger(const T &a) : a(a) {}
};

template<int L, class T>
struct NumLenLoggerBase {
    T a;
    explicit NumLenLoggerBase(T a) : a(a) {}
};
template<int L, class T>
NumLenLoggerBase<L, T> NumLenLogger(T a) {
    return {a};
}

template<class T>
auto constptr_Logger(T *a) {
    return (const T*)a;
}

template<class T>
struct NumHexLogger {
    T a;
    explicit NumHexLogger(T a) : a(a) {}
};

template<class T>
struct NumBinLogger {
    T a;
    explicit NumBinLogger(T a) : a(a) {}
};

struct HotkeyLogger {
    std::vector<BYTE> &a;
};

template<auto TT, class T>
struct ArrayLoggerBase {
    T *a;
    size_t n;
};
template<class T> T deref_ftor(T *a) { return *a; }
template<class T> T *ref_ftor(T *a) { return a; }
template<class T, class TT> T ctor_ftor(TT *a) { return T(*a); }
template<class T, class TT> T ctor_ref_ftor(TT *a) { return T(a); }
template<class T>
ArrayLoggerBase<deref_ftor<T>, T> ArrayLoggerDeref(T *a, size_t n) {
    return {a, n};
}
template<class T>
ArrayLoggerBase<ref_ftor<T>, T> ArrayLoggerRef(T *a, size_t n) {
    return {a, n};
}

template<class T, class TT>
ArrayLoggerBase<ctor_ftor<T, TT>, TT> ArrayLoggerCtor(TT *a, size_t n) {
    return {a, n};
}

template<template<class> class T, class TT>
auto ArrayLoggerCtor(TT *a, size_t n) {
    return ArrayLoggerCtor<decltype(T(*a))>(a, n);
}

struct StringLogger {
    LPCSTR a;
    explicit StringLogger(LPCSTR a);
};

struct RawStringLogger {
    LPCSTR a;
    LPCSTR p;
    RawStringLogger(LPCSTR a, LPCSTR p);
};

struct ByteArrayLogger {
    const char *b;
    UINT n;
    ByteArrayLogger(const void *b, UINT n);
};

struct CharLogger {
    CHAR a;
    explicit CharLogger(CHAR a);
};

struct ShaderLogger {
    const void *a;
    std::string source;
    DWORD lang = 0;
    explicit ShaderLogger(const void *a);
};

struct D3D10_CLEAR_Logger {
    UINT a;
    explicit D3D10_CLEAR_Logger(UINT a);
};

struct D3D10_BIND_Logger {
    UINT a;
    explicit D3D10_BIND_Logger(UINT a);
};

struct D3D10_CPU_ACCESS_Logger {
    UINT a;
    explicit D3D10_CPU_ACCESS_Logger(UINT a);
};

struct D3D10_RESOURCE_MISC_Logger {
    UINT a;
    explicit D3D10_RESOURCE_MISC_Logger(UINT a);
};

struct D3D10_SUBRESOURCE_DATA_Logger {
    const D3D10_SUBRESOURCE_DATA *pInitialData;
    UINT ByteWidth;
    D3D10_SUBRESOURCE_DATA_Logger(const D3D10_SUBRESOURCE_DATA *, UINT);
};

struct ID3D10Resource_id_Logger {
    UINT64 id;
    explicit ID3D10Resource_id_Logger(UINT64);
};

struct MyID3D10Resource_Logger {
    const ID3D10Resource *r;
    explicit MyID3D10Resource_Logger(const ID3D10Resource *);
};

class Logger {
    class Impl;
    Impl *impl;
    std::ostream &oss;

    bool log_begin();
    void log_end();

    void log_assign();
    void log_sep();
    void log_fun_name(LPCSTR n);
    void log_fun_begin();
    template<class T>
    void log_fun_arg(LPCSTR n, T v) {
        log_item(n);
        log_assign();
        log_item(v);
    }
    void log_fun_args();
    template <class T>
    void log_fun_args(T r) {
        log_fun_end(r);
    }
    template<class T, class... Ts>
    void log_fun_args(LPCSTR n, T v, Ts... as) {
        log_fun_arg(n, v);
        log_fun_args_next(as...);
    }
    void log_fun_args_next();
    template<class T>
    void log_fun_args_next(T r) {
        log_fun_args(r);
    }
    template<class... Ts>
    void log_fun_args_next(LPCSTR n, Ts... as) {
        log_fun_sep();
        log_fun_args(n, as...);
    }

    template<size_t N, class T, class... Ts>
    void log_fun_args(const LogSkip<N> skip, LPCSTR n, T v, Ts... as) {
        if (skip)
            log_fun_args(*skip, as...);
        else
            log_fun_args(n, v, *skip, as...);
    }
    template<class... Ts>
    void log_fun_args(const LogSkip<0> skip, Ts... as) {
        log_fun_args(as...);
    }
    template<size_t N, class T, class... Ts>
    void log_fun_args_next(const LogSkip<N> skip, LPCSTR n, T v, Ts... as) {
        if (skip)
            log_fun_args_next(*skip, as...);
        else
            log_fun_args_next(n, v, *skip, as...);
    }
    template<class... Ts>
    void log_fun_args_next(const LogSkip<0> skip, Ts... as) {
        log_fun_args_next(as...);
    }

    void log_fun_sep();
    void log_fun_end();
    template<class T>
    void log_fun_ret(T v) {
        log_assign();
        log_item(v);
    }
    template<class... Ts>
    void log_fun_name_begin(LPCSTR n, Ts... as) {
        log_fun_name(n);
        log_fun_begin();
        log_fun_args(as...);
    }
    template<class T>
    void log_fun_end(T r) {
        log_fun_end();
        log_fun_ret(r);
    }
    void log_struct_begin();
    void log_struct_sep();
    void log_struct_end();
    void log_array_begin();
    void log_array_sep();
    void log_array_end();
    void log_string_begin();
    void log_string_end();
    void log_char_begin();
    void log_char_end();
    void log_null();

    template<class T>
    std::enable_if_t<std::is_arithmetic_v<T>> log_item(T a) {
        oss << +a;
    }

    void log_item(bool a);
    void log_item(CHAR a);
    void log_item(WCHAR a);
    void log_item(LPCSTR a);
    void log_item(LPCWSTR a);
    void log_item(const std::string &a);

    void log_items_base();
    template<class T, class... Ts>
    void log_items_base(T a, Ts... as) {
        log_item(a);
        log_items_base(as...);
    }

    template<class T>
    void log_item(T *a) {
        log_item((const T *)a);
    }

    template<class T>
    void log_item(const T *a) {
        if (a) log_item(NumHexLogger(a));
        else log_null();
    }

    template<int L, class T>
    void log_item(NumLenLoggerBase<L, T> a) {
        oss << std::setfill('0') << std::setw(L) << +a.a;
        oss.flags(std::ios::fmtflags{});
    }

    template<class T>
    void log_item(NumHexLogger<T> a) {
        oss << std::hex << std::showbase << +a.a;
        oss.flags(std::ios::fmtflags{});
    }

    template<class T>
    void log_item(NumBinLogger<T> a) {
        auto b = (LPBYTE)&a.a;
        UINT n = sizeof(a.a);
        UINT bits = std::numeric_limits<BYTE>::digits;
        BYTE m = (BYTE)1 << (bits - 1);
        oss << "0b";
        for (UINT i = n; i > 0;) {
            --i;
            BYTE c = b[i];
            for (UINT j = 0; j < bits; ++j) {
                oss << (c & m ? "1" : "0");
                c <<= 1;
            }
        }
    }

    template<auto TT, class T>
    void log_item(ArrayLoggerBase<TT, T> a) {
        if (!a.a) { log_null(); return; }
        log_array_begin();
        bool first = true;
        for (T *t = a.a; t != a.a + a.n; ++t) {
            if (first) {
                first = false;
            } else {
                log_array_sep();
            }
            log_item(TT(t));
        }
        log_array_end();
    }

    template<class T>
    void log_enum(const ENUM_MAP(T) &map, T a, bool hex = false) {
        typename ENUM_MAP(T)::const_iterator it = map.find(a);
        if (it != map.end()) {
            log_item(it->second);
        } else {
if constexpr (std::is_enum_v<T>) {
            auto v = std::underlying_type_t<T>(a);
            if (hex) {
                log_item(NumHexLogger(v));
            } else {
                log_item(+v);
            }
} else {
            if (hex) {
                log_item(NumHexLogger(a));
            } else {
                log_item(+a);
            }
}
        }
    }

    void log_flag_sep();
    template<class T>
    void log_flag(const FLAG_MAP(T) &map, T a) {
        T t = 0;
        bool first = true;
        for (typename FLAG_MAP(T)::value_type v : map) {
            if (a & v.first) {
                if (first) {
                    first = false;
                } else {
                    log_flag_sep();
                }
                log_item(v.second);
                t |= v.first;
            }
        }
        t = a & ~t;
        if (first) {
            log_item(NumHexLogger(t));
        } else {
            if (t) {
                log_flag_sep();
                log_item(NumHexLogger(t));
            }
        }
    }

    void log_item(StringLogger a);
    void log_item(RawStringLogger a);
    void log_item(ByteArrayLogger a);
    void log_item(CharLogger a);
    void log_item(ShaderLogger a);
    void log_item(D3D10_CLEAR_Logger a);
    void log_item(D3D10_BIND_Logger a);
    void log_item(D3D10_CPU_ACCESS_Logger a);
    void log_item(D3D10_RESOURCE_MISC_Logger a);
    void log_item(HotkeyLogger a);
    void log_item(const GUID *guid);
    void log_item(const D3D10_SAMPLER_DESC *sampler_desc);
    void log_item(D3D10_FILTER a);
    void log_item(D3D10_TEXTURE_ADDRESS_MODE a);
    void log_item(D3D10_COMPARISON_FUNC a);
    void log_item(const D3D10_INPUT_ELEMENT_DESC *input_element_descs);
    void log_item(DXGI_FORMAT a);
    void log_item(D3D10_INPUT_CLASSIFICATION a);
    void log_item(const D3D10_BOX *box);
    void log_item(const D3D10_BUFFER_DESC *buffer_desc);
    void log_item(D3D10_USAGE a);
    void log_item(const D3D10_TEXTURE1D_DESC *desc);
    void log_item(const D3D10_TEXTURE2D_DESC *desc);
    void log_item(const D3D10_TEXTURE3D_DESC *desc);
    void log_item(const DXGI_SAMPLE_DESC a);
    void log_item(const DXGI_SAMPLE_DESC *a);
    void log_item(const D3D10_SHADER_RESOURCE_VIEW_DESC *a);
    void log_item(const D3D10_RENDER_TARGET_VIEW_DESC *a);
    void log_item(const D3D10_DEPTH_STENCIL_VIEW_DESC *a);
    void log_item(D3D10_PRIMITIVE_TOPOLOGY a);
    void log_item(const D3D10_VIEWPORT *a);
    void log_item(D3D10_SUBRESOURCE_DATA_Logger a);
    void log_item(D3D10_MAP a);
    void log_item(D3D10_MAP_FLAG a);
    void log_item(ID3D10Resource_id_Logger a);
    void log_item(const MyID3D10Buffer *a);
    void log_item(const MyID3D10Texture1D *a);
    void log_item(const MyID3D10Texture2D *a);
    void log_item(const MyID3D10Texture3D *a);
    void log_item(const MyID3D10ShaderResourceView *a);
    void log_item(const MyID3D10RenderTargetView *a);
    void log_item(const MyID3D10DepthStencilView *a);
    void log_item(MyID3D10Resource_Logger a);
    void log_item(const D3D10_BLEND_DESC *a);
    void log_item(D3D10_BLEND a);
    void log_item(D3D10_BLEND_OP a);
    void log_item(const D3D10_DEPTH_STENCIL_DESC *a);
    void log_item(D3D10_DEPTH_WRITE_MASK a);
    void log_item(const D3D10_DEPTH_STENCILOP_DESC *a);
    void log_item(D3D10_STENCIL_OP a);
    void log_item(D3D10_SRV_DIMENSION a);
    void log_item(D3D10_RTV_DIMENSION a);
    void log_item(D3D10_DSV_DIMENSION a);
    void log_item(const D3D10_TEX2D_SRV *a);
    void log_item(const D3D10_TEX2D_RTV *a);
    void log_item(const D3D10_TEX2D_DSV *a);

    void log_struct_members();
    template<class T>
    void log_struct_members(T a) {
        log_item(a);
    }
    template<class T, class TT, class... Ts>
    void log_struct_members(T a, TT aa, Ts... as) {
        log_struct_members(a);
        log_struct_sep();
        log_struct_members(aa, as...);
    }
    template<class... Ts>
    void log_struct(Ts... as) {
        log_struct_begin();
        log_struct_members(as...);
        log_struct_end();
    }
    void log_struct_members_named();
    template<class T>
    void log_struct_members_named(LPCSTR n, T a) {
        log_item(n);
        log_assign();
        log_item(a);
    }
    template<class T, class TT, class... Ts>
    void log_struct_members_named(LPCSTR n, T a, LPCSTR nn, TT aa, Ts... as) {
        log_struct_members_named(n, a);
        log_struct_sep();
        log_struct_members_named(nn, aa, as...);
    }
    template<class... Ts>
    void log_struct_named(Ts... as) {
        log_struct_begin();
        log_struct_members_named(as...);
        log_struct_end();
    }

public:
    Logger(LPCTSTR file_name);
    ~Logger();

    void set_overlay(Overlay *overlay);
    void set_config(Config *config);

    bool get_started();
    void next_frame();

    template<class... Ts>
    void log_fun(std::string n, Ts... as) {
        if (log_begin()) {
            log_fun_name_begin(n.c_str(), as...);
            log_end();
        }
    }

private:
    template<class T> friend struct LogItem;
    template<class T>
    void log_item(LogItem<T> a) {
        a.log_item(this);
    }
    template<class T>
    auto log_item(const T &a)->decltype(log_item(LogItem<T>{&a})) {
        log_item(LogItem<T>{&a});
    }
    template<class T>
    auto log_item(const T *a)->decltype(log_item(LogItem<T>{a})) {
        log_item(LogItem<T>{a});
    }

    template<class T>
    auto log_item(const T &a)
        ->std::enable_if_t<std::is_class_v<T>, decltype(log_item(&a))>
    {
        log_item(&a);
    }
};
extern Logger *default_logger;

#define LOG_STRUCT_MEMBER(n) "." #n, (STRUCT)->n
#define LOG_STRUCT_MEMBER_TYPE(n, t, ...) "." #n, t((STRUCT)->n, ## __VA_ARGS__)
#define LOG_ARG(n) #n, n
#define LOG_ARG_TYPE(n, t, ...) #n, t(n, ## __VA_ARGS__)

#if ENABLE_LOGGER

#define LOG_STARTED ((LOGGER)->get_started())
#define LOG_FUN(_, ...) do { if LOG_STARTED (LOGGER)->log_fun(__func__, ## __VA_ARGS__); } while (0)
#define LOG_MFUN_DEF(n, ...) do { if LOG_STARTED (LOGGER)->log_fun(std::string(#n "::") + __func__, LOG_ARG(this), ## __VA_ARGS__); } while (0)

#else

#define LOG_STARTED false
#define LOG_FUN(_, ...) (void)0
#define LOG_MFUN_DEF(n, ...) (void)0

#endif

#endif
