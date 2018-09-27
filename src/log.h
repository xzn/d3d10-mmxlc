#ifndef LOG_H
#define LOG_H

#include "main.h"

template<class T>
struct DefaultLogger {
    T a;
    DefaultLogger(T a) : a(a) {}
};

template<int L, class T>
struct NumLenLoggerBase {
    T a;
    NumLenLoggerBase(T a) : a(a) {}
};
template<int L, class T>
NumLenLoggerBase<L, T> NumLenLogger(T a) {
    return {a};
}

template<class T>
struct NumHexLogger {
    T a;
    NumHexLogger(T a) : a(a) {}
};

struct HotkeyLogger {
    std::vector<BYTE> &a;
};

template<class TT, class T>
struct ArrayLoggerBase {
    T *a;
    size_t n;
};
template<class T>
struct Deref {
    T operator()(T *a) const { return *a; }
};
template<class T>
struct Ref {
    T *operator()(T *a) const { return a; }
};
template<class T>
ArrayLoggerBase<Deref<T>, T> ArrayLoggerDeref(T *a, size_t n) {
    return {a, n};
}
template<class T>
ArrayLoggerBase<Ref<T>, T> ArrayLoggerRef(T *a, size_t n) {
    return {a, n};
}

struct StringLogger {
    LPCSTR a;
    StringLogger(LPCSTR a);
};

struct RawStringLogger {
    LPCSTR a;
    LPCSTR p;
    RawStringLogger(LPCSTR a, LPCSTR p);
};

struct CharLogger {
    CHAR a;
    CharLogger(CHAR a);
};

struct ShaderLogger {
    const void *a;
    SIZE_T n;
    ShaderLogger(const void *a, SIZE_T n);
};

struct D3D10_CLEAR_Logger {
    UINT a;
    D3D10_CLEAR_Logger(UINT a);
};

struct D3D10_BIND_Logger {
    UINT a;
    D3D10_BIND_Logger(UINT a);
};

struct D3D10_CPU_ACCESS_Logger {
    UINT a;
    D3D10_CPU_ACCESS_Logger(UINT a);
};

struct D3D10_RESOURCE_MISC_Logger {
    UINT a;
    D3D10_RESOURCE_MISC_Logger(UINT a);
};

class Logger {
    LPCTSTR file_name;
    bool started;

    UINT64 start_count;
    UINT64 frame_count;
    HANDLE file;
    std::ostringstream oss;
    CRITICAL_SECTION oss_cs;

    bool file_init();
    void file_shutdown();

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
    void log_fun_args_next();
    template <class T>
    void log_fun_args_next(T r) {
        log_fun_end_next(r);
    }
    template <class T>
    void log_fun_args_next(LPCSTR n, T v) {
        log_fun_arg(n, v);
        log_fun_end();
    }
    template <class T, class R>
    void log_fun_args_next(LPCSTR n, T v, R r) {
        log_fun_arg(n, v);
        log_fun_end_next(r);
    }
    template<class T, class TT, class... Ts>
    void log_fun_args_next(LPCSTR n, T v, LPCSTR nn, TT vv, Ts... as) {
        log_fun_arg(n, v);
        log_fun_sep();
        log_fun_args_next(nn, vv, as...);
    }
    void log_fun_sep();
    void log_fun_end();
    template<class T>
    void log_fun_ret(T v) {
        log_assign();
        log_item(v);
    }
    template<class... Ts>
    void log_fun_name_begin_next(LPCSTR n, Ts... as) {
        log_fun_name(n);
        log_fun_begin();
        log_fun_args_next(as...);
    }
    template<class T>
    void log_fun_end_next(T r) {
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
    void log_item(T a) {
        oss << a;
    }
    void log_item(LPCWSTR a);

    template<class T>
    void log_item(T *a) {
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

    template<class TT, class T>
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
            log_item(TT()(t));
        }
        log_array_end();
    }

    template<class T>
    void log_enum(const ENUM_MAP(T) &map, T a, bool hex = false) {
        typename ENUM_MAP(T)::const_iterator it = map.find(a);
        if (it != map.end()) {
            log_item(it->second);
        } else {
            if (hex) {
                log_item(NumHexLogger(a));
            } else {
                log_item(+a);
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

    void log_items_base();
    template<class T, class... Ts>
    void log_items_base(T a, Ts... as) {
        log_item(a);
        log_items_base(as...);
    }

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

    LPCTSTR get_file_name();
    bool get_started();
    bool start();
    void stop();

    void next_frame();

    template<class... Ts>
    void log_items(Ts... as) {
        if (log_begin()) {
            log_items_base(as...);
            log_end();
        }
    }

    template<class... Ts>
    void log_fun(std::string n, Ts... as) {
        if (log_begin()) {
            log_fun_name_begin_next(n.c_str(), as...);
            log_end();
        }
    }

    template<class... Ts>
    bool log_fun_custom_begin(std::string n, Ts... as) {
        if (log_begin()) {
            log_fun_name(n.c_str());
            log_fun_begin();
            log_fun_custom_args(as...);
            return true;
        }
        return false;
    }
    void log_fun_custom_args();
    template <class T>
    void log_fun_custom_args(LPCSTR n, T v) {
        log_fun_arg(n, v);
    }
    template<class T, class TT, class... Ts>
    void log_fun_custom_args(LPCSTR n, T v, LPCSTR nn, TT vv, Ts... as) {
        log_fun_custom_args(n, v);
        log_fun_sep();
        log_fun_custom_args(nn, vv, as...);
    }
    template<class... Ts>
    void log_fun_custom_args_next(Ts... as) {
        log_fun_sep();
        log_fun_custom_args(as...);
    }
    void log_fun_custom_end();
    template<class T>
    void log_fun_custom_end(T r) {
        log_fun_end_next(r);
        log_end();
    }
};
extern Logger *default_logger;

#define LOG_STRUCT_MEMBER(n) "." #n, STRUCT->n
#define LOG_STRUCT_MEMBER_TYPE(n, t, ...) "." #n, t(STRUCT->n, ## __VA_ARGS__)
#define LOG_ARG(n) #n, n
#define LOG_ARG_TYPE(n, t, ...) #n, t(n, ## __VA_ARGS__)

#if ENABLE_LOGGER

#define LOG_STARTED (LOGGER->get_started())
#define LOG_FUN(_, ...) do { if LOG_STARTED LOGGER->log_fun(__func__, ## __VA_ARGS__); } while (0)
#define LOG_FUN_BEGIN(_, ...) (LOG_STARTED && LOGGER->log_fun_custom_begin(__func__, ## __VA_ARGS__))
#define LOG_FUN_ARGS(...) (LOGGER->log_fun_custom_args(__VA_ARGS__))
#define LOG_FUN_ARGS_NEXT(...) (LOGGER->log_fun_custom_args_next(__VA_ARGS__))
#define LOG_FUN_END(...) (LOGGER->log_fun_custom_end(__VA_ARGS__))
#define LOG_MFUN_DEF(n, ...) do { if LOG_STARTED LOGGER->log_fun(std::string(#n "::") + __func__, LOG_ARG(this), ## __VA_ARGS__); } while (0)
#define LOG_MFUN_BEGIN_DEF(n, ...) (LOG_STARTED && LOGGER->log_fun_custom_begin(std::string(#n "::") + __func__, LOG_ARG(this), ## __VA_ARGS__))
#define LOG_ITEMS(...) do { if LOG_STARTED LOGGER->log_items(__VA_ARGS__); } while (0)

#else

#define LOG_STARTED false
#define LOG_FUN(_, ...) do {} while (0)
#define LOG_FUN_BEGIN(_, ...) false
#define LOG_FUN_ARGS(...) do {} while (0)
#define LOG_FUN_ARGS_NEXT(...) do {} while (0)
#define LOG_FUN_END(...) do {} while (0)
#define LOG_MFUN_DEF(n, ...) do {} while (0)
#define LOG_MFUN_BEGIN_DEF(n, ...) false
#define LOG_ITEMS(...) do {} while (0)

#endif

#endif
