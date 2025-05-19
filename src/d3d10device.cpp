#include "d3d10device.h"
#include "d3d10pixelshader.h"
#include "d3d10vertexshader.h"
#include "d3d10buffer.h"
#include "d3d10texture1d.h"
#include "d3d10texture2d.h"
#include "d3d10texture3d.h"
#include "d3d10samplerstate.h"
#include "d3d10depthstencilstate.h"
#include "d3d10inputlayout.h"
#include "d3d10rendertargetview.h"
#include "d3d10shaderresourceview.h"
#include "d3d10depthstencilview.h"
#include "conf.h"
#include "log.h"
#include "overlay.h"
#include "tex.h"
#include "unknown_impl.h"
#include "../smhasher/MurmurHash3.h"
#include "../RetroArch/gfx/drivers/d3d10.h"
#include "half/include/half.hpp"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10Device, ## __VA_ARGS__)

#define NOISE_WIDTH 256
#define NOISE_HEIGHT 256
#define X1_WIDTH 256
#define X1_HEIGHT 224
#define X4_WIDTH 320
#define X6_WIDTH 512
#define X4_HEIGHT 240
#define X6_HEIGHT X4_HEIGHT
#define X1_WIDTH_FILTERED (X1_WIDTH * 2)
#define X1_HEIGHT_FILTERED (X1_HEIGHT * 2)
#define X4_WIDTH_FILTERED (X4_WIDTH * 2)
#define X4_HEIGHT_FILTERED (X4_HEIGHT * 2)
#define X6_WIDTH_FILTERED (X6_WIDTH * 2)
#define X6_HEIGHT_FILTERED (X6_HEIGHT * 2)
#define PS_BYTECODE_LENGTH_T1_THRESHOLD 1000
#define PS_HASH_T1 0xa54c4b2
#define PS_HASH_T2 0xc9b117d5
#define PS_HASH_T3 0x1f4c05ac

#define SO_B_LEN (sizeof(float) * 4 * 4 * 6 * 100)
#define MAX_SAMPLERS 16
#define MAX_SHADER_RESOURCES 128
#define MAX_CONSTANT_BUFFERS  14
#define TEX_DIMS_CB_INDEX 13

namespace {

template<typename T>
bool almost_equal(T a, T b) { return (a > b ? a - b : b - a) <= 1; }

UINT64 xorshift128p_state[2] = {};

bool xorshift128p_state_init() {
    if (!(
        QueryPerformanceCounter((LARGE_INTEGER *)&xorshift128p_state[0]) &&
        QueryPerformanceCounter((LARGE_INTEGER *)&xorshift128p_state[1]))
    ) {
        xorshift128p_state[0] = GetTickCount64();
        xorshift128p_state[1] = GetTickCount64();
    }
    return xorshift128p_state[0] && xorshift128p_state[1];
}

bool xorshift128p_state_init_status = xorshift128p_state_init();

void get_resolution_mul(
    UINT &render_width,
    UINT &render_height,
    UINT width,
    UINT height
) {
    UINT width_quo = render_width / width;
    UINT width_rem = render_width % width;
    if (width_rem) ++width_quo;
    UINT height_quo = render_height / height;
    UINT height_rem = render_height % height;
    if (height_rem) ++height_quo;
    render_width = width * width_quo;
    render_height = height * height_quo;
}

}

// From wikipedia
UINT64 xorshift128p() {
    UINT64 *s = xorshift128p_state;
    UINT64 a = s[0];
    UINT64 const b = s[1];
    s[0] = b;
    a ^= a << 23;       // a
    a ^= a >> 17;       // b
    a ^= b ^ (b >> 26); // c
    s[1] = a;
    return a + b;
}

namespace {

template<class = void, class...>
struct DXGIBuffer_Logger;

template<>
struct DXGIBuffer_Logger<> {
    const char *&buffer;
};

template<class T, class... Ts>
struct DXGIBuffer_Logger : DXGIBuffer_Logger<Ts...> {};

struct DXGIBufferType {

template<size_t N>
struct Typeless;

template<size_t N>
struct Unused;

template<size_t N>
struct SInt;

template<size_t N>
struct UInt;

template<size_t N>
struct SNorm;

template<size_t N>
struct UNorm;

struct Float;
struct Half;

};

}

template<class... Ts>
struct LogItem<DXGIBuffer_Logger<Ts...>> : DXGIBufferType {
    DXGIBuffer_Logger<Ts...> &t;
    LogItem(DXGIBuffer_Logger<Ts...> &t) : t(t) {}

    static constexpr bool comp_size(size_t n) {
        return n && n <= 32 && !(n % 8);
    }

    template<size_t N, class... As>
    static void log_comp(Logger *l, DXGIBuffer_Logger<Typeless<N>, As...> &t) {
        static_assert(comp_size(N));
        UINT32 v = 0;
        for (size_t i = 0; i < N; i += 8)
            v |= (UINT32)*(UINT8 *)t.buffer++ << i;
        l->log_item(NumHexLogger(v));
    }

    template<size_t N, class... As>
    static void log_comp(Logger *l, DXGIBuffer_Logger<SInt<N>, As...> &t) {
        static_assert(comp_size(N));
        UINT32 v = 0;
        for (size_t i = 0; i < N; i += 8)
            v |= (UINT32)*(UINT8 *)t.buffer++ << i;
        v <<= (32 - N);
        auto s = (INT32)v;
        s >>= (32 - N);
        l->log_item(s);
    }

    template<size_t N, class... As>
    static void log_comp(Logger *l, DXGIBuffer_Logger<UInt<N>, As...> &t) {
        static_assert(comp_size(N));
        UINT32 v = 0;
        for (size_t i = 0; i < N; i += 8)
            v |= (UINT32)*(UINT8 *)t.buffer++ << i;
        l->log_item(v);
    }

    template<size_t N, class... As>
    static void log_comp(Logger *l, DXGIBuffer_Logger<SNorm<N>, As...> &t) {
        static_assert(comp_size(N));
        UINT32 v = 0;
        for (size_t i = 0; i < N; i += 8)
            v |= (UINT32)*(UINT8 *)t.buffer++ << i;
        v <<= (32 - N);
        auto s = (INT32)v;
        s >>= (32 - N);
        auto n = std::max((double)s / (((UINT32)1 << (N - 1)) - 1), -1.);
        l->log_item(n);
    }

    template<size_t N, class... As>
    static void log_comp(Logger *l, DXGIBuffer_Logger<UNorm<N>, As...> &t) {
        static_assert(comp_size(N));
        UINT32 v = 0;
        for (size_t i = 0; i < N; i += 8)
            v |= (UINT32)*(UINT8 *)t.buffer++ << i;
        auto n = (double)v / (((UINT64)1 << N) - 1);
        l->log_item(n);
    }

    template<class... As>
    static void log_comp(Logger *l, DXGIBuffer_Logger<Float, As...> &t) {
        auto f = *(float *)t.buffer;
        t.buffer += sizeof(f);
        l->log_item(f);
    }

    template<class... As>
    static void log_comp(Logger *l, DXGIBuffer_Logger<Half, As...> &t) {
        auto f = *(half_float::half *)t.buffer;
        t.buffer += sizeof(f);
        l->log_item((float)f);
    }

    template<class A, class... As>
    void log_comps(Logger *l, DXGIBuffer_Logger<A, As...> &t) {
        log_comp(l, t);
        log_comps_next(l, *(DXGIBuffer_Logger<As...> *)&t);
    }
    template<class A, class... As>
    void log_comps_next(Logger *l, DXGIBuffer_Logger<A, As...> &t) {
        l->log_array_sep();
        log_comps(l, t);
    }
    template<size_t N, class... As>
    void log_comps_next(Logger *l, DXGIBuffer_Logger<Unused<N>, As...> &t) {
        static_assert(comp_size(N));
        t.buffer += N / 8;
        log_comps_next(l, *(DXGIBuffer_Logger<As...> *)&t);
    }

    void log_comps(Logger *l, DXGIBuffer_Logger<> &t) {}
    void log_comps_next(Logger *l, DXGIBuffer_Logger<> &t) {
        log_comps(l, t);
    }

    void log_item(Logger *l) {
        l->log_array_begin();
        log_comps(l, t);
        l->log_array_end();
    }
};

#define TEX_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM

const ENUM_MAP(D3D10_INPUT_CLASSIFICATION) D3D10_INPUT_CLASSIFICATION_ENUM_MAP = {
    ENUM_MAP_ITEM(D3D10_INPUT_PER_VERTEX_DATA),
    ENUM_MAP_ITEM(D3D10_INPUT_PER_INSTANCE_DATA),
};

template<>
struct LogItem<D3D10_INPUT_CLASSIFICATION> {
    const D3D10_INPUT_CLASSIFICATION a;
    void log_item(Logger *l) const {
        l->log_enum(D3D10_INPUT_CLASSIFICATION_ENUM_MAP, a);
    }
};

struct MyVertexBufferElement_Logger {
    const char *buffer;
    const MyID3D10InputLayout *input_layout;
};

template<>
struct LogItem<MyVertexBufferElement_Logger> : DXGIBufferType {
    const MyVertexBufferElement_Logger *a;
    LogItem(const MyVertexBufferElement_Logger *a) : a(a) {}
    void log_item(Logger *l) const {
        l->log_struct_begin();
        auto descs = a->input_layout->get_descs();
        auto cached = a->buffer;
        for (UINT i = 0; i < a->input_layout->get_descs_num(); ++i, ++descs) {
            if (i) l->log_struct_sep();
            l->log_item(descs->SemanticName);
            l->log_struct_member_access();
            l->log_item(descs->SemanticIndex);
            l->log_assign();
            if (descs->InputSlot) {
                l->log_struct_named(
#define STRUCT descs
                    LOG_STRUCT_MEMBER(InputSlot)
#undef STRUCT
                );
            } else {
                if (descs->AlignedByteOffset != D3D10_APPEND_ALIGNED_ELEMENT)
                    cached = a->buffer + descs->AlignedByteOffset;
                if (cached) switch (descs->Format) {
                    case DXGI_FORMAT_R32G32B32A32_TYPELESS: {
                        l->log_item(DXGIBuffer_Logger<
                            Typeless<32>, Typeless<32>, Typeless<32>, Typeless<32>
                        >{cached});
                        break;
                    }

                    case DXGI_FORMAT_R32G32B32A32_FLOAT:
                        l->log_item(DXGIBuffer_Logger<
                            Float, Float, Float, Float
                        >{cached});
                        break;

                    case DXGI_FORMAT_R32G32B32A32_UINT:
                        l->log_item(DXGIBuffer_Logger<
                            UInt<32>, UInt<32>, UInt<32>, UInt<32>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R32G32B32A32_SINT:
                        l->log_item(DXGIBuffer_Logger<
                            SInt<32>, SInt<32>, SInt<32>, SInt<32>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R32G32B32_TYPELESS:
                        l->log_item(DXGIBuffer_Logger<
                            Typeless<32>, Typeless<32>, Typeless<32>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R32G32B32_FLOAT:
                        l->log_item(DXGIBuffer_Logger<
                            Float, Float, Float
                        >{cached});
                        break;

                    case DXGI_FORMAT_R32G32B32_UINT:
                        l->log_item(DXGIBuffer_Logger<
                            UInt<32>, UInt<32>, UInt<32>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R32G32B32_SINT:
                        l->log_item(DXGIBuffer_Logger<
                            SInt<32>, SInt<32>, SInt<32>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
                        l->log_item(DXGIBuffer_Logger<
                            Typeless<16>, Typeless<16>, Typeless<16>, Typeless<16>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16G16B16A16_FLOAT:
                        l->log_item(DXGIBuffer_Logger<
                            Half, Half, Half, Half
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16G16B16A16_UNORM:
                        l->log_item(DXGIBuffer_Logger<
                            UNorm<16>, UNorm<16>, UNorm<16>, UNorm<16>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16G16B16A16_UINT:
                        l->log_item(DXGIBuffer_Logger<
                            UInt<16>, UInt<16>, UInt<16>, UInt<16>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16G16B16A16_SNORM:
                        l->log_item(DXGIBuffer_Logger<
                            SNorm<16>, SNorm<16>, SNorm<16>, SNorm<16>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16G16B16A16_SINT:
                        l->log_item(DXGIBuffer_Logger<
                            SInt<16>, SInt<16>, SInt<16>, SInt<16>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R32G32_TYPELESS:
                        l->log_item(DXGIBuffer_Logger<
                            Typeless<32>, Typeless<32>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R32G32_FLOAT:
                        l->log_item(DXGIBuffer_Logger<
                            Float, Float
                        >{cached});
                        break;

                    case DXGI_FORMAT_R32G32_UINT:
                        l->log_item(DXGIBuffer_Logger<
                            UInt<32>, UInt<32>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R32G32_SINT:
                        l->log_item(DXGIBuffer_Logger<
                            SInt<32>, SInt<32>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
                        l->log_item(DXGIBuffer_Logger<
                            Typeless<8>, Typeless<8>, Typeless<8>, Typeless<8>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R8G8B8A8_UNORM:
                        l->log_item(DXGIBuffer_Logger<
                            UNorm<8>, UNorm<8>, UNorm<8>, UNorm<8>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R8G8B8A8_UINT:
                        l->log_item(DXGIBuffer_Logger<
                            UInt<8>, UInt<8>, UInt<8>, UInt<8>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R8G8B8A8_SNORM:
                        l->log_item(DXGIBuffer_Logger<
                            SNorm<8>, SNorm<8>, SNorm<8>, SNorm<8>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R8G8B8A8_SINT:
                        l->log_item(DXGIBuffer_Logger<
                            SInt<8>, SInt<8>, SInt<8>, SInt<8>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16G16_TYPELESS:
                        l->log_item(DXGIBuffer_Logger<
                            Typeless<16>, Typeless<16>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16G16_FLOAT:
                        l->log_item(DXGIBuffer_Logger<
                            Half, Half
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16G16_UNORM:
                        l->log_item(DXGIBuffer_Logger<
                            UNorm<16>, UNorm<16>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16G16_UINT:
                        l->log_item(DXGIBuffer_Logger<
                            UInt<16>, UInt<16>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16G16_SNORM:
                        l->log_item(DXGIBuffer_Logger<
                            SNorm<16>, SNorm<16>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16G16_SINT:
                        l->log_item(DXGIBuffer_Logger<
                            SInt<16>, SInt<16>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R32_TYPELESS:
                        l->log_item(DXGIBuffer_Logger<
                            Typeless<32>
                        >{cached});
                        break;

                    case DXGI_FORMAT_D32_FLOAT:
                    case DXGI_FORMAT_R32_FLOAT:
                        l->log_item(DXGIBuffer_Logger<
                            Float
                        >{cached});
                        break;

                    case DXGI_FORMAT_R32_UINT:
                        l->log_item(DXGIBuffer_Logger<
                            UInt<32>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R32_SINT:
                        l->log_item(DXGIBuffer_Logger<
                            SInt<32>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R8G8_TYPELESS:
                        l->log_item(DXGIBuffer_Logger<
                            Typeless<8>, Typeless<8>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R8G8_UNORM:
                        l->log_item(DXGIBuffer_Logger<
                            UNorm<8>, UNorm<8>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R8G8_UINT:
                        l->log_item(DXGIBuffer_Logger<
                            UInt<8>, UInt<8>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R8G8_SNORM:
                        l->log_item(DXGIBuffer_Logger<
                            SNorm<8>, SNorm<8>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R8G8_SINT:
                        l->log_item(DXGIBuffer_Logger<
                            SInt<8>, SInt<8>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16_TYPELESS:
                        l->log_item(DXGIBuffer_Logger<
                            Typeless<16>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16_FLOAT:
                        l->log_item(DXGIBuffer_Logger<
                            Half
                        >{cached});
                        break;

                    case DXGI_FORMAT_D16_UNORM:
                    case DXGI_FORMAT_R16_UNORM:
                        l->log_item(DXGIBuffer_Logger<
                            UNorm<16>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16_UINT:
                        l->log_item(DXGIBuffer_Logger<
                            UInt<16>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16_SNORM:
                        l->log_item(DXGIBuffer_Logger<
                            SNorm<16>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R16_SINT:
                        l->log_item(DXGIBuffer_Logger<
                            SInt<16>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R8_TYPELESS:
                        l->log_item(DXGIBuffer_Logger<
                            Typeless<8>
                        >{cached});
                        break;

                    case DXGI_FORMAT_A8_UNORM:
                    case DXGI_FORMAT_R8_UNORM:
                        l->log_item(DXGIBuffer_Logger<
                            UNorm<8>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R8_UINT:
                        l->log_item(DXGIBuffer_Logger<
                            UInt<8>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R8_SNORM:
                        l->log_item(DXGIBuffer_Logger<
                            SNorm<8>
                        >{cached});
                        break;

                    case DXGI_FORMAT_R8_SINT:
                        l->log_item(DXGIBuffer_Logger<
                            SInt<8>
                        >{cached});
                        break;

                    default:
                        l->log_struct_named(
#define STRUCT descs
                            LOG_STRUCT_MEMBER(Format)
#undef STRUCT
                        );
                        cached = NULL;
                        break;
                } else {
                    l->log_struct_named(
#define STRUCT descs
                        LOG_STRUCT_MEMBER(Format)
#undef STRUCT
                    );
                }
            }
        }
        l->log_struct_end();
    }
};

struct MyVertexBuffer_Logger {
    const MyID3D10InputLayout *input_layout;
    const MyID3D10Buffer *vertex_buffer;
    UINT stride;
    UINT offset;
    UINT VertexCount;
    UINT StartVertexLocation;
};

template<>
struct LogItem<MyVertexBuffer_Logger> {
    const MyVertexBuffer_Logger *a;
    void log_item(Logger *l) const {
        l->log_struct_begin();
        auto vb_cached_state = a->vertex_buffer->get_cached_state();
        if (vb_cached_state) {
            if (auto cached = a->vertex_buffer->get_cached()) {
                l->log_struct_sep();
                l->log_item("vb_cached");
                l->log_assign();
                l->log_array_begin();

                cached += a->offset + a->stride * a->StartVertexLocation;
                for (UINT i = 0; i < a->VertexCount; ++i, cached += a->stride) {
                    if (i) l->log_array_sep();
                    l->log_item(
                        MyVertexBufferElement_Logger{cached, a->input_layout}
                    );
                }

                l->log_array_end();
            }
        } else {
            l->log_struct_members_named(
                LOG_ARG(vb_cached_state)
            );
        }
        l->log_struct_end();
    }
};

struct MyIndexedVertexBuffer_Logger {
    const MyID3D10InputLayout *input_layout;
    const MyID3D10Buffer *vertex_buffer;
    UINT stride;
    UINT offset;
    const MyID3D10Buffer *index_buffer;
    DXGI_FORMAT index_format;
    UINT index_offset;
    UINT IndexCount;
    UINT StartIndexLocation;
    INT BaseVertexLocation;
};

template<>
struct LogItem<MyIndexedVertexBuffer_Logger> {
    const MyIndexedVertexBuffer_Logger *a;
    void log_item(Logger *l) const {
        l->log_struct_begin();
        auto vb_cached_state = a->vertex_buffer->get_cached_state();
        auto ib_cached_state = a->index_buffer->get_cached_state();
        if (vb_cached_state && ib_cached_state) {
            auto vb_cached = a->vertex_buffer->get_cached();
            auto ib_cached = a->index_buffer->get_cached();
            if (ib_cached) {
                l->log_struct_sep();
                l->log_item("vb_cached");
                l->log_assign();
                l->log_array_begin();

                bool ib_32 = a->index_format == DXGI_FORMAT_R32_UINT;
                if (ib_32 || a->index_format == DXGI_FORMAT_R16_UINT) {
                    vb_cached +=
                        a->offset + a->stride * a->BaseVertexLocation;
                    ib_cached +=
                        a->index_offset +
                        (ib_32 ? sizeof(UINT32) : sizeof(UINT16)) *
                            a->StartIndexLocation;
                    for (UINT i = 0; i < a->IndexCount; ++i) {
                        if (i) l->log_array_sep();
                        UINT vb_i = ib_32 ?
                            ((UINT32 *)ib_cached)[i] :
                            ((UINT16 *)ib_cached)[i];
                        l->log_item(
                            MyVertexBufferElement_Logger{
                                vb_cached + a->stride * vb_i,
                                a->input_layout
                            }
                        );
                    }
                }

                l->log_array_end();
            }
        } else {
            l->log_struct_members_named(
                LOG_ARG(vb_cached_state),
                LOG_ARG(ib_cached_state)
            );
        }
        l->log_struct_end();
    }
};

namespace {

struct SOBuffer_Logger {
    const MyID3D10VertexShader *vs;
    const MyID3D10PixelShader *ps;
    const char *data;
    const MyID3D10ShaderResourceView *const *srvs;
    const D3D10_VIEWPORT &vp;
    const UINT64 n;
};

struct ConstantBuffer_Logger {
    const std::vector<std::tuple<std::string, std::string>> &uniform_list;
    const MyID3D10Buffer *const *pscbs;
};

// From Boost
template<class T>
void hash_combine(size_t &s, const T &v) {
    s ^= std::hash<T>()(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

template<class T, size_t I = std::tuple_size_v<T> - 1>
struct tuple_hash {
    static void apply(size_t& s, const T &t) {
        tuple_hash<T, I - 1>::apply(s, t);
        hash_combine(s, std::get<I>(t));
    }
};

template<class T>
struct tuple_hash<T, 0>
{
    static void apply(size_t &s, const T &t) {
        hash_combine(s, std::get<0>(t));
    }
};

}

namespace std {

template<class... Ts>
struct hash<tuple<Ts...>> {
    size_t operator()(const tuple<Ts...> &t) const {
        size_t s = 0;
        tuple_hash<tuple<Ts...>>::apply(s, t);
        return s;
    }
};

}

template<>
struct LogItem<SOBuffer_Logger> {
    const SOBuffer_Logger& a;

    struct Pos {
        double x = -1;
        double y = -1;
        operator bool() const {
            return x >= 0 && y >= 0;
        }
        bool operator<(const Pos &r) const {
            return
                std::tie(x, y)
                < std::tie(r.x, r.y);
        }
    };

    void log_item(Logger *l) const {
        l->log_array_begin();
        auto data = (const float *)a.data;
        auto &sampler_map = a.ps->get_texcoord_sampler_map();
        std::unordered_map<std::tuple<std::string, UINT>, std::set<Pos>> coords;
        for (UINT64 i = 0; i < a.n; ++i) {
            if ((const char *)data >= a.data + SO_B_LEN) break;
            if (i) l->log_array_sep();
            l->log_struct_begin();
            bool sep = false;
            for (auto &entry : a.vs->get_decl_entries()) {
                if ((const char *)(data + entry.ComponentCount) >= a.data + SO_B_LEN) {
                    data += entry.ComponentCount;
                    break;
                }
                if (sep) l->log_struct_sep();
                sep = true;
                l->log_item(entry.SemanticName);
                l->log_struct_member_access();
                l->log_item(entry.SemanticIndex);
                l->log_assign();

                UINT width = 0, height = 0;
                bool w =
                    entry.StartComponent < 4 &&
                    entry.ComponentCount + entry.StartComponent >= 4;
                float d = w ? data[3 - entry.StartComponent] : 0;
                if (!d) d = 1;
                bool position =
                    w && entry.SemanticName == std::string("SV_Position");
                if (position) {
                    width = a.vp.Width;
                    height = a.vp.Height;
                }
                bool texcoord =
                    !position &&
                        entry.SemanticName == std::string("TEXCOORD");
                if (texcoord) {
                    if (entry.SemanticIndex >= sampler_map.size()) {
                        texcoord = false;
                    } else {
                        UINT srv_i = sampler_map[entry.SemanticIndex];
                        if (srv_i == (UINT)-1) {
                            texcoord = false;
                        } else {
                            l->log_item(srv_i);
                            auto srv = a.srvs[srv_i];
                            if (
                                !srv ||
                                srv->get_desc().ViewDimension != D3D10_SRV_DIMENSION_TEXTURE2D
                            ) {
                                texcoord = false;
                            } else {
                                auto tex = (const MyID3D10Texture2D *)srv->get_resource();
                                width = tex->get_desc().Width;
                                height = tex->get_desc().Height;
                            }
                        }
                    }
                }
                if (width && height) {
                    l->log_struct_begin();
                    l->log_item(width);
                    l->log_struct_sep();
                    l->log_item(height);
                    l->log_struct_end();
                    l->log_struct_member_access();
                }
                Pos pixel_pos{};

                l->log_array_begin();
                for (
                    UINT i = 0, c = entry.StartComponent;
                    i < entry.ComponentCount;
                    ++i, ++c
                ) {
                    if (i) l->log_array_sep();
                    float v = *data++;
                    l->log_item(v);
                    if (c > 1) continue;
                    if (position) {
                        switch (c) {
                            case 0:
                                pixel_pos.x = (v / d + 1) / 2 * width - 0.5;
                                l->log_struct_begin();
                                l->log_item(pixel_pos.x);
                                l->log_struct_end();
                                break;

                            case 1:
                                pixel_pos.y = (-v / d + 1) / 2 * height - 0.5;
                                l->log_struct_begin();
                                l->log_item(pixel_pos.y);
                                l->log_struct_end();
                                break;

                            default:
                                break;
                        }
                    } else if (texcoord) {
                        switch (c) {
                            case 0:
                                pixel_pos.x = v * width - 0.5;
                                l->log_struct_begin();
                                l->log_item(pixel_pos.x);
                                l->log_struct_end();
                                break;

                            case 1:
                                pixel_pos.y = v * height - 0.5;
                                l->log_struct_begin();
                                l->log_item(pixel_pos.y);
                                l->log_struct_end();
                                break;

                            default:
                                break;
                        }
                    }
                }
                l->log_array_end();

                if (pixel_pos)
                    coords[
                        std::make_tuple(
                            std::string(entry.SemanticName),
                            entry.SemanticIndex
                        )
                    ].insert(std::move(pixel_pos));
            }
            l->log_struct_end();
        }
        l->log_array_end();
        l->log_struct_member_access();
        l->log_struct_begin();
        bool sep = false;
        for (auto &item : coords) {
            if (sep) l->log_struct_sep();
            sep = true;
            l->log_item(std::get<0>(item.first));
            l->log_struct_member_access();
            l->log_item(std::get<1>(item.first));
            l->log_assign();
            l->log_array_begin();
            bool sep = false;
            for (auto &pos : item.second) {
                if (sep) l->log_array_sep();
                sep = true;
                l->log_struct_begin();
                l->log_item(pos.x);
                l->log_struct_sep();
                l->log_item(pos.y);
                l->log_struct_end();
            }
            l->log_array_end();
        }
        l->log_struct_end();
    }
};

template<>
struct LogItem<ConstantBuffer_Logger> {
    const ConstantBuffer_Logger& a;
    void log_item(Logger *l) const {
        l->log_array_begin();
        for (auto &item : a.uniform_list) {
            auto type = std::get<0>(item);
            auto name = std::get<1>(item);
        }
        l->log_array_end();
    }
};

class MyID3D10Device::Impl {
    friend class MyID3D10Device;

    IUNKNOWN_PRIV(ID3D10Device)
    OverlayPtr overlay = {NULL};
    Config *config = NULL;

    ID3D10Buffer *tex_dims_cb = NULL;
    struct {
        float width;
        float height;
        float paddings[2];
    } tex_dims = {};

    d3d10_video_t *d3d10_2d = NULL;
    d3d10_video_t *d3d10_snes = NULL;
    d3d10_video_t *d3d10_psone = NULL;
    d3d10_video_t *d3d10_3d = NULL;
    UINT64 frame_count = 0;

    struct FilterState {
        MyID3D10ShaderResourceView *srv;
        MyID3D10Texture2D *rtv_tex;
        MyID3D10PixelShader *ps;
        MyID3D10VertexShader *vs;
        bool t1;
        MyID3D10SamplerState *psss;
        bool x4;
        bool x6;
        UINT start_vertex_location;
        ID3D10Buffer *vertex_buffer;
        UINT vertex_stride;
        UINT vertex_offset;
    } filter_state = {};
    bool can_filter = false;
    bool need_filter = false;
    void clear_filter() {
        if (filter_state.srv) filter_state.srv->Release();
        if (filter_state.rtv_tex) filter_state.rtv_tex->Release();
        if (filter_state.ps) filter_state.ps->Release();
        if (filter_state.vs) filter_state.vs->Release();
        if (filter_state.psss) filter_state.psss->Release();
        if (filter_state.vertex_buffer) filter_state.vertex_buffer->Release();
        filter_state = {};
    }

    bool render_interp = false;
    bool render_linear = false;
    bool render_enhanced = false;
    UINT linear_test_width = 0;
    UINT linear_test_height = 0;

    void update_config() {
        if (!config) return;

        if (config->linear_test_updated) {
            config->begin_config();

            linear_test_width = config->linear_test_width;
            linear_test_height = config->linear_test_height;

            config->linear_test_updated = false;
            config->end_config();
        }

#define GET_SET_CONFIG_BOOL(v, m) do { \
    bool v = config->v; \
    if (render_ ## v != v) { \
        render_ ## v = v; \
        if (v) overlay(m " enabled"); else overlay(m " disabled"); \
    } \
} while (0)

        GET_SET_CONFIG_BOOL(interp, "Interp fix");
        GET_SET_CONFIG_BOOL(linear, "Force linear filtering");
        GET_SET_CONFIG_BOOL(enhanced, "Enhanced Type 1 filter");

#define SLANG_SHADERS \
    X(2d) \
    X(snes) \
    X(psone) \
    X(3d)

#define X(v) \
    config->slang_shader_ ## v ## _updated ||

if constexpr (ENABLE_SLANG_SHADER) {
        if (SLANG_SHADERS false) {

#undef X

#define X(v) \
    bool slang_shader_ ## v ## _updated = \
        config->slang_shader_ ## v ## _updated; \
    std::string slang_shader_ ## v; \
    if (slang_shader_ ## v ## _updated) { \
        slang_shader_ ## v = config->slang_shader_ ## v; \
        config->slang_shader_ ## v ## _updated = false; \
    }

            config->begin_config();

            SLANG_SHADERS

            config->end_config();

#undef X

#define X(v) \
    if (slang_shader_ ## v ## _updated) { \
        if (!d3d10_ ## v) { \
            if (!(d3d10_ ## v = my_d3d10_gfx_init( \
                inner, \
                TEX_FORMAT \
            ))) { \
                overlay( \
                    "Failed to initialize slang shader " \
                    #v \
                ); \
            } \
        } \
        if (d3d10_ ## v) { \
            if (!slang_shader_ ## v.size()) { \
                my_d3d10_gfx_set_shader( \
                    d3d10_ ## v, \
                    NULL \
                ); \
                overlay("Slang shader " #v " disabled"); \
            } else if (my_d3d10_gfx_set_shader( \
                d3d10_ ## v, \
                slang_shader_ ## v.c_str() \
            )) { \
                overlay( \
                    "Slang shader " #v " set to ", \
                    slang_shader_ ## v \
                ); \
            } else { \
                overlay( \
                    "Failed to set slang shader " #v " to ", \
                    slang_shader_ ## v \
                ); \
            } \
        } \
    }

            SLANG_SHADERS

#undef X

#undef SLANG_SHADERS

        }
}
    }

    void create_sampler(
        D3D10_FILTER filter,
        ID3D10SamplerState *&sampler,
        D3D10_TEXTURE_ADDRESS_MODE address =
            D3D10_TEXTURE_ADDRESS_CLAMP
    ) {
        D3D10_SAMPLER_DESC desc = {
            .Filter = filter,
            .AddressU = address,
            .AddressV = address,
            .AddressW = address,
            .MipLODBias = 0,
            .MaxAnisotropy = 16,
            .ComparisonFunc = D3D10_COMPARISON_NEVER,
            .BorderColor = {},
            .MinLOD = 0,
            .MaxLOD = D3D10_FLOAT32_MAX
        };
        inner->CreateSamplerState(&desc, &sampler);
    }

    void create_texture(
        UINT width,
        UINT height,
        ID3D10Texture2D *&texture,
        DXGI_FORMAT format = TEX_FORMAT,
        UINT bind_flags =
            D3D10_BIND_SHADER_RESOURCE |
            D3D10_BIND_RENDER_TARGET
    ) {
        D3D10_TEXTURE2D_DESC desc = {
            .Width = width,
            .Height = height,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = format,
            .SampleDesc = {.Count = 1, .Quality = 0},
            .Usage = D3D10_USAGE_DEFAULT,
            .BindFlags = bind_flags,
            .CPUAccessFlags = 0,
            .MiscFlags = 0
        };
        inner->CreateTexture2D(&desc, NULL, &texture);
    }

    void create_tex_and_views(
        UINT width,
        UINT height,
        TextureAndViews *tex
    ) {
        create_texture(width, height, tex->tex);
        create_srv(
            tex->tex,
            tex->srv
        );
        create_rtv(
            tex->tex,
            tex->rtv
        );
        tex->width = width;
        tex->height = height;
    }

    void create_rtv(
        ID3D10Texture2D *tex,
        ID3D10RenderTargetView *&rtv,
        DXGI_FORMAT format = TEX_FORMAT
    ) {
        D3D10_RENDER_TARGET_VIEW_DESC desc = {
            .Format = format,
            .ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D,
        };
        desc.Texture2D.MipSlice = 0;
        inner->CreateRenderTargetView(tex, &desc, &rtv);
    }

    void create_srv(
        ID3D10Texture2D *tex,
        ID3D10ShaderResourceView *&srv,
        DXGI_FORMAT format = TEX_FORMAT
    ) {
        D3D10_SHADER_RESOURCE_VIEW_DESC desc = {
            .Format = format,
            .ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D,
        };
        D3D10_TEXTURE2D_DESC tex_desc;
        tex->GetDesc(&tex_desc);
        desc.Texture2D.MostDetailedMip = 0;
        desc.Texture2D.MipLevels = tex_desc.MipLevels;
        inner->CreateShaderResourceView(tex, &desc, &srv);
    }

    void create_dsv(
        ID3D10Texture2D *tex,
        ID3D10DepthStencilView *&dsv,
        DXGI_FORMAT format
    ) {
        D3D10_DEPTH_STENCIL_VIEW_DESC desc = {
            .Format = format,
            .ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D,
        };
        desc.Texture2D.MipSlice = 0;
        inner->CreateDepthStencilView(tex, &desc, &dsv);
    }

    bool set_render_tex_views_and_update(
        MyID3D10Texture2D *tex,
        UINT width,
        UINT height,
        UINT orig_width,
        UINT orig_height,
        bool need_vp
    ) {
if constexpr (ENABLE_CUSTOM_RESOLUTION) {
        if (
            need_vp &&
            need_render_vp &&
            (
                render_width != width ||
                render_height != height ||
                render_orig_width != orig_width ||
                render_orig_height != orig_height
            )
        ) return false;
        if (
            !almost_equal(tex->get_orig_width(), orig_width) ||
            !almost_equal(tex->get_orig_height(), orig_height)
        ) return false;
        D3D10_TEXTURE2D_DESC desc = tex->get_desc();
        if (
            !almost_equal(desc.Width, width) ||
            !almost_equal(desc.Height, height)
        ) {
            if (tex->get_sc()) return false;

            desc.Width = width;
            desc.Height = height;
            auto &t = tex->get_inner();
            t->Release();
            inner->CreateTexture2D(&desc, NULL, &t);

            for (MyID3D10RenderTargetView *rtv : tex->get_rtvs()) {
                auto &v = rtv->get_inner();
                cached_rtvs_map.erase(v);
                v->Release();
                inner->CreateRenderTargetView(t, &rtv->get_desc(), &v);
                cached_rtvs_map.emplace(v, rtv);
            }
            for (MyID3D10ShaderResourceView *srv : tex->get_srvs()) {
                auto &v = srv->get_inner();
                cached_srvs_map.erase(v);
                v->Release();
                inner->CreateShaderResourceView(t, &srv->get_desc(), &v);
                cached_srvs_map.emplace(v, srv);
            }
            for (MyID3D10DepthStencilView *dsv : tex->get_dsvs()) {
                auto &v = dsv->get_inner();
                cached_dsvs_map.erase(v);
                v->Release();
                inner->CreateDepthStencilView(t, &dsv->get_desc(), &v);
                cached_dsvs_map.emplace(v, dsv);
            }

            tex->get_desc() = desc;
        }
        if (
            almost_equal(width, orig_width) &&
            almost_equal(height, orig_height)
        ) return false;
        if (need_vp && !need_render_vp) {
            render_width = width;
            render_height = height;
            render_orig_width = orig_width;
            render_orig_height = orig_height;
            need_render_vp = true;
        }
        return true;
}
        return false;
    }

    bool set_render_tex_views_and_update(
        ID3D10Resource *r,
        bool need_vp = false
    ) {
if constexpr (ENABLE_CUSTOM_RESOLUTION) {
        D3D10_RESOURCE_DIMENSION type;
        if (
            r->GetType(&type),
            type != D3D10_RESOURCE_DIMENSION_TEXTURE2D
        ) return false;
        auto tex = (MyID3D10Texture2D *)r;
        bool ret = set_render_tex_views_and_update(
            tex,
            render_size.sc_width,
            render_size.sc_height,
            cached_size.sc_width,
            cached_size.sc_height,
            need_vp
        );
if constexpr (ENABLE_CUSTOM_RESOLUTION > 1) {
        ret = ret || set_render_tex_views_and_update(
            tex,
            render_3d ?
                render_3d_width :
                render_size.render_3d_width,
            render_3d ?
                render_3d_height :
                render_size.render_3d_height,
            cached_size.render_3d_width,
            cached_size.render_3d_height,
            need_vp
        );
}
        return ret;
}
        return false;
    }

    void create_tex_and_views_nn(
        TextureAndViews *tex,
        UINT width,
        UINT height
    ) {
        UINT render_width = render_size.render_width;
        UINT render_height = render_size.render_height;
        get_resolution_mul(
            render_width,
            render_height,
            width,
            height
        );
        create_tex_and_views(
            render_width,
            render_height,
            tex
        );
    }

    void create_tex_and_view_1(
        TextureViewsAndBuffer *tex,
        UINT render_width,
        UINT render_height,
        UINT width,
        UINT height
    ) {
        create_texture(
            render_width,
            render_height,
            tex->tex
        );
        create_srv(
            tex->tex,
            tex->srv
        );
        create_rtv(
            tex->tex,
            tex->rtv
        );
        tex->width = render_width;
        tex->height = render_height;
        float ps_cb_data[4] = {
            (float)width,
            (float)height,
            (float)(1.0 / width),
            (float)(1.0 / height)
        };
        D3D10_BUFFER_DESC desc = {
            .ByteWidth = sizeof(ps_cb_data),
            .Usage = D3D10_USAGE_IMMUTABLE,
            .BindFlags = D3D10_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = 0,
            .MiscFlags = 0
        };
        D3D10_SUBRESOURCE_DATA data = {
            .pSysMem = ps_cb_data,
            .SysMemPitch = 0,
            .SysMemSlicePitch = 0
        };
        inner->CreateBuffer(&desc, &data, &tex->ps_cb);
    }

    void create_tex_and_view_1_v(
        std::vector<TextureViewsAndBuffer *> &tex_v,
        UINT width,
        UINT height
    ) {
        bool last = false;
        do {
            UINT width_next = width * 2;
            UINT height_next = height * 2;
            last =
                width_next >= render_size.render_width &&
                height_next >= render_size.render_height;
            TextureViewsAndBuffer *tex =
                new TextureViewsAndBuffer{};
            create_tex_and_view_1(
                tex,
                width_next,
                height_next,
                width,
                height
            );
            tex_v.push_back(tex);
            width = width_next;
            height = height_next;
        } while (!last);
    }

    void create_tex_and_depth_views_2(
        UINT width,
        UINT height,
        TextureAndDepthViews *tex
    ) {
        UINT render_width = render_size.render_width;
        UINT render_height = render_size.render_height;
        get_resolution_mul(
            render_width,
            render_height,
            width,
            height
        );
        create_tex_and_views(
            render_width,
            render_height,
            tex
        );
        create_texture(
            tex->width,
            tex->height,
            tex->tex_ds,
            DXGI_FORMAT_R24G8_TYPELESS,
            D3D10_BIND_DEPTH_STENCIL
        );
        create_dsv(
            tex->tex_ds,
            tex->dsv,
            DXGI_FORMAT_D24_UNORM_S8_UINT
        );
    }

    struct FilterTemp {
        ID3D10SamplerState *sampler_nn;
        ID3D10SamplerState *sampler_linear;
        ID3D10SamplerState *sampler_wrap;
        TextureAndViews *tex_nn_x1;
        TextureAndViews *tex_nn_x4;
        TextureAndViews *tex_nn_x6;
        TextureAndDepthViews *tex_t2;
        TextureAndViews *tex_x8;
        std::vector<TextureViewsAndBuffer *> tex_1_x1;
        std::vector<TextureViewsAndBuffer *> tex_1_x4;
        std::vector<TextureViewsAndBuffer *> tex_1_x6;
    } filter_temp = {};

    void filter_temp_init() {
        filter_temp_shutdown();
        create_sampler(
            D3D10_FILTER_MIN_MAG_MIP_POINT,
            filter_temp.sampler_nn
        );
        create_sampler(
            D3D10_FILTER_MIN_MAG_MIP_LINEAR,
            filter_temp.sampler_linear
        );
        create_sampler(
            D3D10_FILTER_MIN_MAG_MIP_POINT,
            filter_temp.sampler_wrap,
            D3D10_TEXTURE_ADDRESS_WRAP
        );
        filter_temp.tex_nn_x1 = new TextureAndViews{};
        create_tex_and_views_nn(
            filter_temp.tex_nn_x1,
            X1_WIDTH,
            X1_HEIGHT
        );
        filter_temp.tex_nn_x4 = new TextureAndViews{};
        create_tex_and_views_nn(
            filter_temp.tex_nn_x4,
            X4_WIDTH,
            X4_HEIGHT
        );
        filter_temp.tex_nn_x6 = new TextureAndViews{};
        create_tex_and_views_nn(
            filter_temp.tex_nn_x6,
            X6_WIDTH,
            X6_HEIGHT
        );
        filter_temp.tex_t2 = new TextureAndDepthViews{};
        create_tex_and_depth_views_2(
            NOISE_WIDTH,
            NOISE_HEIGHT,
            filter_temp.tex_t2
        );
        create_tex_and_view_1_v(
            filter_temp.tex_1_x1,
            X1_WIDTH_FILTERED,
            X1_HEIGHT_FILTERED
        );
        create_tex_and_view_1_v(
            filter_temp.tex_1_x4,
            X4_WIDTH_FILTERED,
            X4_HEIGHT_FILTERED
        );
        create_tex_and_view_1_v(
            filter_temp.tex_1_x6,
            X6_WIDTH_FILTERED,
            X6_HEIGHT_FILTERED
        );
        filter_temp_init_x8();
    }

    void filter_temp_init_x8() {
        if (filter_temp.tex_x8)
            delete filter_temp.tex_x8;
        filter_temp.tex_x8 = new TextureAndViews{};
        create_tex_and_views(
            render_3d ?
                render_3d_width :
                render_size.render_3d_width,
            render_3d ?
                render_3d_height :
                render_size.render_3d_height,
            filter_temp.tex_x8
        );
    }

    void filter_temp_shutdown() {
        if (filter_temp.sampler_nn)
            filter_temp.sampler_nn->Release();
        if (filter_temp.sampler_linear)
            filter_temp.sampler_linear->Release();
        if (filter_temp.sampler_wrap)
            filter_temp.sampler_wrap->Release();
        if (filter_temp.tex_nn_x1)
            delete filter_temp.tex_nn_x1;
        if (filter_temp.tex_nn_x4)
            delete filter_temp.tex_nn_x4;
        if (filter_temp.tex_nn_x6)
            delete filter_temp.tex_nn_x6;
        if (filter_temp.tex_t2)
            delete filter_temp.tex_t2;
        if (filter_temp.tex_x8)
            delete filter_temp.tex_x8;
        for (auto tex : filter_temp.tex_1_x1)
            delete tex;
        for (auto tex : filter_temp.tex_1_x4)
            delete tex;
        for (auto tex : filter_temp.tex_1_x6)
            delete tex;
        filter_temp = {};
    }

    struct LinearFilterConditions {
        PIXEL_SHADER_ALPHA_DISCARD alpha_discard;
        bool tex_has_lut;
        std::vector<const D3D10_SAMPLER_DESC *> samplers_descs;
        std::vector<const D3D10_TEXTURE2D_DESC *> texs_descs;
    } linear_conditions = {};

    friend class LogItem<LinearFilterConditions>;

#define MAX_LINEAR_RTVS 32
    struct LinearRtv {
        UINT width;
        UINT height;
        ID3D10RenderTargetView *rtv;
        ~LinearRtv() {
            if (rtv) rtv->Release();
            rtv = NULL;
        }
    };
    std::deque<LinearRtv> linear_rtvs;

    bool linear_restore = false;
    bool stream_out = false;
    bool stream_out_available() {
        return
            !cached_gs &&
            cached_ps && cached_vs && cached_vs->get_sogs() &&
            so_bt && so_bs && so_q && *cached_pssrvs &&
            cached_pt == D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    }

    void linear_conditions_begin() {
        linear_restore = false;
        stream_out = false;
        linear_conditions = {};
        if (!render_linear && !LOG_STARTED) return;

        if (cached_ps) {
            linear_conditions.alpha_discard =
                cached_ps->get_alpha_discard();
            linear_conditions.tex_has_lut =
                cached_ps->get_tex_has_lut();
        }

        ID3D10SamplerState *sss[MAX_SAMPLERS] = {};
        auto sss_inner = sss;
        auto pssss = cached_pssss;
        for (
            auto srvs = cached_pssrvs;
            *srvs;
            ++srvs, ++pssss, ++sss_inner
        ) {
            auto srv = *srvs;
            linear_conditions.texs_descs.push_back(
                srv->get_desc().ViewDimension ==
                    D3D10_SRV_DIMENSION_TEXTURE2D ?
                    &((MyID3D10Texture2D *)srv->get_resource())->
                        get_desc() :
                    NULL
            );
            auto ss = *pssss;
            linear_conditions.samplers_descs.push_back(
                ss ? &ss->get_desc() : NULL
            );
            *sss_inner = ss ? ss->get_inner() : NULL;
        }
        if (
            linear_conditions.alpha_discard ==
                PIXEL_SHADER_ALPHA_DISCARD::EQUAL &&
            linear_conditions.tex_has_lut
        ) {
            if (render_linear && linear_conditions.texs_descs.size()) {
                UINT width = linear_conditions.texs_descs[0]->Width;
                UINT height = linear_conditions.texs_descs[0]->Height;
                if (
                    !(
                        width == linear_test_width &&
                        height == linear_test_height
                    )
                ) {
                    inner->PSSetSamplers(0, MAX_SAMPLERS, sss);
                    linear_restore = true;
                }
            }
            if (LOG_STARTED && stream_out_available()) {
                inner->GSSetShader(cached_vs->get_sogs());
                UINT offset = 0;
                inner->SOSetTargets(1, &so_bt, &offset);
                so_q->Begin();
                stream_out = true;
            }
        }
    }

    void linear_conditions_end() {
        if (linear_restore)
            inner->PSSetSamplers(0, MAX_SAMPLERS, render_pssss);
        if (stream_out) {
            inner->GSSetShader(NULL);
            inner->SOSetTargets(0, NULL, NULL);
            inner->CopyResource(so_bs, so_bt);
            void *data = NULL;
            so_bs->Map(D3D10_MAP_READ, 0, &data);
            so_q->End();
            D3D10_QUERY_DATA_SO_STATISTICS stat;
            while (S_OK != so_q->GetData(&stat, sizeof(stat), 0));
            SOBuffer_Logger stream_out{
                .vs = cached_vs,
                .ps = cached_ps,
                .data = (const char *)data,
                .srvs = cached_pssrvs,
                .vp = cached_vp,
                .n = stat.NumPrimitivesWritten * 3
            };
            ConstantBuffer_Logger ps_constant_buffer{
                .uniform_list = cached_ps->get_uniform_list(),
                .pscbs = cached_pscbs,
            };
            LOG_FUN(_,
                LOG_ARG(stream_out),
                LOG_ARG(ps_constant_buffer)
            );
            so_bs->Unmap();
        }
    }

    struct Size {
        UINT sc_width;
        UINT sc_height;
        UINT render_width;
        UINT render_height;
        UINT render_3d_width;
        UINT render_3d_height;
        void resize(UINT width, UINT height) {
            sc_width = width;
            sc_height = height;
            render_width = sc_height * 4 / 3;
            render_height = sc_height;
            render_3d_width = sc_width * 3 / 4;
            render_3d_height = sc_height;
        }
    } cached_size = {}, render_size = {};

    ID3D10Buffer *so_bt = NULL, *so_bs = NULL;
    ID3D10Query *so_q = NULL;
    MyID3D10PixelShader *cached_ps = NULL;
    MyID3D10VertexShader *cached_vs = NULL;
    ID3D10GeometryShader *cached_gs = NULL;
    MyID3D10InputLayout *cached_il = NULL;
    MyID3D10DepthStencilState *cached_dss = NULL;
    MyID3D10SamplerState *cached_pssss[MAX_SAMPLERS] = {};
    ID3D10SamplerState *render_pssss[MAX_SAMPLERS] = {};
    MyID3D10RenderTargetView *cached_rtv = NULL;
    MyID3D10DepthStencilView *cached_dsv = NULL;
    MyID3D10ShaderResourceView *cached_pssrvs[MAX_SHADER_RESOURCES] = {};
    ID3D10ShaderResourceView *render_pssrvs[MAX_SHADER_RESOURCES] = {};
    D3D10_PRIMITIVE_TOPOLOGY cached_pt =
        D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED;
    struct BlendState {
        ID3D10BlendState *pBlendState;
        FLOAT BlendFactor[4];
        UINT SampleMask;
    } cached_bs = {};
    struct VertexBuffers {
        ID3D10Buffer *ppVertexBuffers[
            D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT
        ];
        MyID3D10Buffer *vertex_buffers[
            D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT
        ];
        UINT pStrides[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
        UINT pOffsets[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    } cached_vbs = {};
    MyID3D10Buffer *cached_ib = NULL;
    DXGI_FORMAT cached_ib_format = DXGI_FORMAT_UNKNOWN;
    UINT cached_ib_offset = 0;
    ID3D10Buffer *render_pscbs[MAX_CONSTANT_BUFFERS] = {};
    MyID3D10Buffer *cached_pscbs[MAX_CONSTANT_BUFFERS] = {};
    ID3D10Buffer *cached_vscbs[MAX_CONSTANT_BUFFERS] = {};
    UINT render_width = 0;
    UINT render_height = 0;
    UINT render_orig_width = 0;
    UINT render_orig_height = 0;
    D3D10_VIEWPORT render_vp = {};
    D3D10_VIEWPORT cached_vp = {};
    bool need_render_vp = false;
    bool is_render_vp = false;
    bool render_3d = false;
    UINT render_3d_width = 0;
    UINT render_3d_height = 0;

    void present() {
        clear_filter();
        can_filter = need_filter;
        need_filter = false;
        update_config();
        ++frame_count;
    }
    void resize_render_3d(UINT width, UINT height) {
        render_3d = width && height;
        if (render_3d) {
            render_3d_width = width;
            render_3d_height = height;
            overlay(
                "3D render resolution set to ",
                std::to_string(render_3d_width),
                "x",
                std::to_string(render_3d_height)
            );
        } else {
            render_3d_width = 0;
            render_3d_height = 0;
            overlay(
                "Restoring 3D render resolution to ",
                std::to_string(render_size.render_3d_width),
                "x",
                std::to_string(render_size.render_3d_height)
            );
        }
        filter_temp_init_x8();
    }
    void resize_buffers(UINT width, UINT height) {
        render_size.resize(width, height);
        clear_filter();
        can_filter = need_filter = false;
        update_config();
        filter_temp_init();
        frame_count = 0;
    }

    void set_render_vp() {
        if (cached_ps && cached_ps->get_has_repl()) {
#if 0
#define OV overlay
#else
#define OV(...)
#endif
            const auto &cached_pssrv = cached_pssrvs[0];
            if (cached_pssrv) {
                const auto &srv_desc = cached_pssrv->get_desc();
                if (srv_desc.ViewDimension != D3D10_SRV_DIMENSION_TEXTURE2D) {
                    OV("srv tex not 2d");
                    return;
                }
                const auto &tex_2d = (MyID3D10Texture2D *)cached_pssrv->get_resource();
                const auto &desc = tex_2d->get_desc();

                if (true || desc.Width != tex_dims.width || desc.Height != tex_dims.height) {
                    tex_dims.width = desc.Width;
                    tex_dims.height = desc.Height;

                    std::ostringstream oss;
                    oss << tex_dims.width << "x" << tex_dims.height;
                    OV(oss.str());

                    if (!tex_dims_cb) {
                        D3D10_BUFFER_DESC cb_desc = {};
                        cb_desc.ByteWidth = sizeof(tex_dims);
                        cb_desc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
                        D3D10_SUBRESOURCE_DATA cb_data = {};
                        cb_data.pSysMem = &tex_dims;
                        HRESULT ret = inner->CreateBuffer(&cb_desc, &cb_data, &tex_dims_cb);
                        if (ret != S_OK || !tex_dims_cb) {
                            std::ostringstream oss;
                            oss << "cb create failed: " << ret;
                            OV(oss.str());
                            return;
                        }
                        inner->PSSetConstantBuffers(TEX_DIMS_CB_INDEX, 1, &tex_dims_cb);
                        OV("cb set");
                    } else {
                        inner->UpdateSubresource(tex_dims_cb, 0, NULL, &tex_dims, 0, 0);
                        OV("cb updated");
                        inner->PSSetConstantBuffers(TEX_DIMS_CB_INDEX, 1, &tex_dims_cb);
                    }
                }
            } else {
                OV("no srv");
            }
        }
#undef OV

if constexpr (ENABLE_CUSTOM_RESOLUTION) {
        if (need_render_vp) {
            if (!is_render_vp) {
                if (cached_vp.Width && cached_vp.Height) {
                    render_vp = D3D10_VIEWPORT {
                        .TopLeftX = (INT)(
                            cached_vp.TopLeftX *
                                render_width /
                                render_orig_width
                        ),
                        .TopLeftY = (INT)(
                            cached_vp.TopLeftY *
                                render_height /
                                render_orig_height
                        ),
                        .Width = cached_vp.Width *
                            render_width /
                            render_orig_width,
                        .Height = cached_vp.Height *
                            render_height /
                            render_orig_height,
                        .MinDepth = cached_vp.MinDepth,
                        .MaxDepth = cached_vp.MaxDepth,
                    };
                    inner->RSSetViewports(1, &render_vp);
                }
                is_render_vp = true;
            }
        }
}

        if (!LOG_STARTED) return;

        size_t srvs_n = 0;
        auto srvs = cached_pssrvs;
        while (srvs_n < MAX_SHADER_RESOURCES && *srvs) {
            ++srvs, ++srvs_n;
        }
        LOG_MFUN(_,
            LOG_ARG(cached_rtv),
            LOG_ARG(cached_dsv),
            LOG_ARG_TYPE(cached_pssrvs, ArrayLoggerDeref, srvs_n),
            LogIf<5>{is_render_vp},
            LOG_ARG(render_vp),
            LOG_ARG(render_width),
            LOG_ARG(render_height),
            LOG_ARG(render_orig_width),
            LOG_ARG(render_orig_height)
        );
    }

    void reset_render_vp() {
if constexpr (ENABLE_CUSTOM_RESOLUTION) {
        if (need_render_vp && is_render_vp) {
            if (cached_vp.Width && cached_vp.Height) {
                render_vp = cached_vp;
                inner->RSSetViewports(1, &render_vp);
            }
            is_render_vp = false;
        }
}
    }

    Impl(
        ID3D10Device **inner,
        UINT width,
        UINT height
    ) : IUNKNOWN_INIT(*inner)
    {
        resize_buffers(width, height);

if constexpr (ENABLE_LOGGER) {
        D3D10_BUFFER_DESC bt_desc = {
            .ByteWidth = SO_B_LEN,
            .Usage = D3D10_USAGE_DEFAULT,
            .BindFlags = D3D10_BIND_STREAM_OUTPUT,
        };
        this->inner->CreateBuffer(&bt_desc, NULL, &so_bt);
        D3D10_BUFFER_DESC bs_desc = {
            .ByteWidth = SO_B_LEN,
            .Usage = D3D10_USAGE_STAGING,
            .CPUAccessFlags = D3D10_CPU_ACCESS_READ
        };
        this->inner->CreateBuffer(&bs_desc, NULL, &so_bs);
        D3D10_QUERY_DESC q_desc = {
            .Query = D3D10_QUERY_SO_STATISTICS
        };
        this->inner->CreateQuery(&q_desc, &so_q);
}
    }

    ~Impl() {
        filter_temp_shutdown();
        clear_filter();
        if (d3d10_2d) my_d3d10_gfx_free(d3d10_2d);
        if (d3d10_snes) my_d3d10_gfx_free(d3d10_snes);
        if (d3d10_psone) my_d3d10_gfx_free(d3d10_psone);
        if (d3d10_3d) my_d3d10_gfx_free(d3d10_3d);

        if (so_bt) so_bt->Release();
        if (so_bs) so_bs->Release();
        if (so_q) so_q->Release();

        if (tex_dims_cb) tex_dims_cb->Release();
    }

    void Draw(
        UINT VertexCount,
        UINT StartVertexLocation
    ) {
        set_render_vp();
        bool filter_next = false;
        bool filter_ss = false;
        bool filter_ss_snes = false;
        bool filter_ss_psone = false;
        bool filter_ss_3d = false;
if constexpr (ENABLE_SLANG_SHADER) {
        filter_ss = d3d10_2d && d3d10_2d->shader_preset;
        filter_ss_snes = d3d10_snes && d3d10_snes->shader_preset;
        filter_ss_psone = d3d10_psone && d3d10_psone->shader_preset;
        filter_ss_3d = d3d10_3d && d3d10_3d->shader_preset;
}
        auto set_filter_state_ps = [this] {
            inner->PSSetShader(filter_state.ps->get_inner());
        };
        auto restore_ps = [this] {
            inner->PSSetShader(cached_ps->get_inner());
        };
        auto restore_vps = [this] {
            inner->RSSetViewports(1, &render_vp);
        };
        auto restore_pscbs = [this] {
            inner->PSSetConstantBuffers(0, MAX_CONSTANT_BUFFERS, render_pscbs);
        };
        auto restore_rtvs = [this] {
            inner->OMSetRenderTargets(
                1,
                &cached_rtv->get_inner(),
                cached_dsv->get_inner()
            );
        };
        auto restore_pssrvs = [this] {
            inner->PSSetShaderResources(
                0,
                MAX_SHADER_RESOURCES,
                render_pssrvs
            );
        };
        auto restore_pssss = [this] {
            inner->PSSetSamplers(0, MAX_SAMPLERS, render_pssss);
        };
        auto set_viewport = [this](UINT width, UINT height) {
            D3D10_VIEWPORT viewport = {
                .TopLeftX = 0,
                .TopLeftY = 0,
                .Width = width,
                .Height = height,
                .MinDepth = 0,
                .MaxDepth = 1,
            };
            inner->RSSetViewports(1, &viewport);
        };
        auto set_srv = [this](ID3D10ShaderResourceView *srv) {
            inner->PSSetShaderResources(
                0,
                1,
                &srv
            );
        };
        auto set_rtv = [this](
            ID3D10RenderTargetView *rtv,
            ID3D10DepthStencilView *dsv = NULL
        ) {
            inner->OMSetRenderTargets(
                1,
                &rtv,
                dsv
            );
        };
        auto set_psss = [this](ID3D10SamplerState *psss) {
            inner->PSSetSamplers(0, 1, &psss);
        };
        auto srv = *cached_pssrvs;
    {
        bool x8 = filter_ss_3d ||
        (
            render_3d &&
            (
                render_3d_width != render_size.render_3d_width ||
                render_3d_height != render_size.render_3d_height
            )
        );
        if (
            !render_interp &&
            !render_linear &&
            !filter_ss &&
            !filter_ss_snes &&
            !filter_ss_psone &&
            !filter_ss_3d &&
            !x8
        ) goto end;

        if (VertexCount != 4) goto end;

        auto psss = *cached_pssss;
        if (!psss) goto end;
        filter_next = need_filter && psss == filter_state.psss;

        if (!srv) goto end;

        D3D10_SHADER_RESOURCE_VIEW_DESC &srv_desc = srv->get_desc();
        if (srv_desc.ViewDimension != D3D10_SRV_DIMENSION_TEXTURE2D)
            goto end;

        auto srv_tex = (MyID3D10Texture2D *)srv->get_resource();
        filter_next &= srv_tex == filter_state.rtv_tex;

        bool x4 = false;
        bool x6 = false;
        D3D10_TEXTURE2D_DESC &srv_tex_desc = srv_tex->get_desc();

        if (filter_next && !(
            render_vp.TopLeftY == 0.0f && render_vp.TopLeftX > 0.0f &&
            almost_equal(
                render_vp.TopLeftX * 2.0f + render_vp.Width,
                (FLOAT)render_size.sc_width) &&
            render_vp.Height == (FLOAT)render_size.sc_height
        )) {
            DWORD ps_hash = cached_ps ? cached_ps->get_bytecode_hash() : 0;
            if (ps_hash == 0x319c1f85) // x challenge double boss intro
                filter_next = need_filter = false;
            if (ps_hash == 0x4e589bcc); // x challenge double boss warning
        }

        filter_next &= can_filter;
        if (filter_next) {
            x8 = false;
        } else if (
            srv_tex_desc.Width == X1_WIDTH &&
            srv_tex_desc.Height == X1_HEIGHT
        ) { // SNES X1~X3
            x8 = false;
            filter_ss |= filter_ss_snes;
        } else if (
            srv_tex_desc.Width == X4_WIDTH &&
            srv_tex_desc.Height == X4_HEIGHT
        ) { // PlayStation X4~X5
            x8 = false;
            x4 = true;
            filter_ss |= filter_ss_psone;
        } else if (
            srv_tex_desc.Width == X6_WIDTH &&
            srv_tex_desc.Height == X6_HEIGHT
        ) { // PlayStation X6
            x8 = false;
            x6 = true;
            filter_ss |= filter_ss_psone;
        } else if (
            x8 &&
            almost_equal(
                srv_tex->get_orig_width(),
                cached_size.render_3d_width
            ) &&
            almost_equal(
                srv_tex->get_orig_height(),
                cached_size.render_3d_height
            )
        ) { // PlayStation 2 X7~X8
        } else {
            goto end;
        }

        auto rtv = cached_rtv;
        if (!rtv) goto end;

        D3D10_RENDER_TARGET_VIEW_DESC &rtv_desc = rtv->get_desc();
        if (rtv_desc.ViewDimension != D3D10_RTV_DIMENSION_TEXTURE2D)
            goto end;

        auto rtv_tex = (MyID3D10Texture2D *)rtv->get_resource();

        D3D10_TEXTURE2D_DESC &rtv_tex_desc = rtv_tex->get_desc();

        auto draw_enhanced = [&](
            std::vector<TextureViewsAndBuffer *> &v_v
        ) {
            auto v_it = v_v.begin();
            if (v_it == v_v.end()) {
                set_psss(filter_temp.sampler_linear);
                inner->Draw(VertexCount, StartVertexLocation);
                restore_pssss();
                return;
            }
            set_filter_state_ps();
            auto set_it_viewport = [&] {
                set_viewport((*v_it)->width, (*v_it)->height);
            };
            set_it_viewport();
            auto set_it_rtv = [&] {
                set_rtv((*v_it)->rtv);
            };
            set_it_rtv();
            auto set_it_pscbs = [&] {
                inner->PSSetConstantBuffers(
                    0,
                    1,
                    &(*v_it)->ps_cb
                );
            };
            set_it_pscbs();
            inner->Draw(
                VertexCount,
                filter_state.start_vertex_location
            );
            auto v_it_prev = v_it;
            auto set_it_prev_srv = [&] {
                set_srv((*v_it_prev)->srv);
            };
            while (++v_it != v_v.end()) {
                set_it_rtv();
                set_it_prev_srv();
                set_it_viewport();
                set_it_pscbs();
                inner->Draw(
                    VertexCount,
                    filter_state.start_vertex_location
                );
                v_it_prev = v_it;
            }
            restore_ps();
            restore_vps();
            restore_rtvs();
            restore_pscbs();
            set_it_prev_srv();
            set_psss(filter_temp.sampler_linear);
            inner->Draw(VertexCount, StartVertexLocation);
            restore_pssrvs();
            restore_pssss();
        };
        auto draw_nn = [&](TextureAndViews *v) {
            set_viewport(v->width, v->height);
            set_rtv(v->rtv);
            set_srv(filter_state.srv->get_inner());
            if (render_linear) set_psss(filter_temp.sampler_nn);
            inner->Draw(
                VertexCount,
                filter_state.start_vertex_location
            );
            restore_vps();
            restore_rtvs();
            set_srv(v->srv);
            set_psss(filter_temp.sampler_linear);
            inner->Draw(VertexCount, StartVertexLocation);
            restore_pssrvs();
            restore_pssss();
        };
        auto draw_ss = [&](
            d3d10_video_t *d3d10,
            MyID3D10ShaderResourceView *srv,
            D3D10_VIEWPORT *render_vp,
            TextureAndViews *cached_tex = NULL
        ) {
            if (cached_tex) {
                video_viewport_t vp = {
                    .x = 0,
                    .y = 0,
                    .width = cached_tex->width,
                    .height = cached_tex->height,
                    .full_width = cached_tex->width,
                    .full_height = cached_tex->height
                };
                my_d3d10_update_viewport(
                    d3d10,
                    cached_tex->rtv,
                    &vp
                );
            } else {
                video_viewport_t vp = {
                    .x = render_vp->TopLeftX,
                    .y = render_vp->TopLeftY,
                    .width = render_vp->Width,
                    .height = render_vp->Height,
                    .full_width = render_size.sc_width,
                    .full_height = render_size.sc_height
                };
                my_d3d10_update_viewport(
                    d3d10,
                    rtv->get_inner(),
                    &vp
                );
            }
            auto my_texture =
                (MyID3D10Texture2D *)srv->get_resource();
            d3d10_texture_t texture = {};
            texture.handle = my_texture->get_inner();
            texture.desc = my_texture->get_desc();
            my_d3d10_gfx_frame(d3d10, &texture, frame_count);

            inner->IASetPrimitiveTopology(cached_pt);
            inner->IASetVertexBuffers(
                0,
                D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT,
                cached_vbs.ppVertexBuffers,
                cached_vbs.pStrides,
                cached_vbs.pOffsets
            );
            inner->OMSetBlendState(
                cached_bs.pBlendState,
                cached_bs.BlendFactor,
                cached_bs.SampleMask
            );
            restore_pscbs();
            inner->VSSetConstantBuffers(
                0,
                MAX_CONSTANT_BUFFERS,
                cached_vscbs
            );
            inner->IASetInputLayout(cached_il->get_inner());
            restore_ps();
            inner->VSSetShader(cached_vs->get_inner());
            inner->GSSetShader(cached_gs);
            restore_vps();
            restore_rtvs();

            if (cached_tex) {
                set_psss(filter_temp.sampler_linear);
                set_srv(cached_tex->srv);
                inner->Draw(VertexCount, StartVertexLocation);
            }

            restore_pssss();
            restore_pssrvs();
        };

        if (filter_next || x8) {
            if (
                rtv_tex->get_orig_width() != cached_size.sc_width ||
                rtv_tex->get_orig_height() != cached_size.sc_height
            ) goto end;

            if (x8) {
                if (filter_ss_3d) {
                    draw_ss(
                        d3d10_3d,
                        srv,
                        NULL,
                        filter_temp.tex_x8
                    );
                } else {
                    set_psss(filter_temp.sampler_linear);
                    inner->Draw(VertexCount, StartVertexLocation);
                    restore_pssss();
                }
                goto done;
            } else if (filter_state.t1) {
                auto d3d10 = d3d10_2d;
                if (filter_state.x4) {
                    if (filter_ss_psone) {
                        d3d10 = d3d10_psone;
                        filter_ss = true;
                    }
                } else {
                    if (filter_ss_snes) {
                        d3d10 = d3d10_snes;
                        filter_ss = true;
                    }
                }
                if (filter_ss) {
                    draw_ss(
                        d3d10,
                        filter_state.srv,
                        &render_vp,
                        render_interp ?
                            filter_state.x4 ?
                                filter_state.x6 ?
                                    filter_temp.tex_nn_x6 :
                                    filter_temp.tex_nn_x4 :
                                filter_temp.tex_nn_x1 :
                            NULL
                    );
                } else if (render_enhanced) {
                    draw_enhanced(
                        filter_state.x4 ?
                            filter_state.x6?
                                filter_temp.tex_1_x6 :
                                filter_temp.tex_1_x4 :
                            filter_temp.tex_1_x1
                    );
                } else {
                    set_psss(filter_temp.sampler_linear);
                    inner->Draw(VertexCount, StartVertexLocation);
                    restore_pssss();
                }
            } else {
                draw_nn(
                    filter_state.x4 ?
                        filter_state.x6 ?
                            filter_temp.tex_nn_x6 :
                            filter_temp.tex_nn_x4 :
                        filter_temp.tex_nn_x1
                );
            }
        } else {
            if (
                rtv_tex_desc.Width != srv_tex_desc.Width * 2 ||
                rtv_tex_desc.Height != srv_tex_desc.Height * 2
            ) goto end;

            clear_filter();
            if (cached_ps) {
                filter_state.ps = cached_ps; cached_ps->AddRef();
                switch (cached_ps->get_bytecode_hash()) {
                    case PS_HASH_T1: filter_state.t1 = true; break;
                    case PS_HASH_T3: filter_state.t1 = false; break;
                    default:
                        filter_state.t1 =
                            cached_ps->get_bytecode_length() >=
                                PS_BYTECODE_LENGTH_T1_THRESHOLD;
                        break;
                }
            }
            filter_state.vs = cached_vs;
            if (cached_vs) cached_vs->AddRef();
            filter_state.psss = psss;
            if (psss) psss->AddRef();
            filter_state.x4 = x4 || x6;
            filter_state.x6 = x6;
            filter_state.start_vertex_location =
                StartVertexLocation;
            filter_state.vertex_buffer =
                *cached_vbs.ppVertexBuffers;
            if (filter_state.vertex_buffer)
                filter_state.vertex_buffer->AddRef();
            filter_state.vertex_stride = *cached_vbs.pStrides;
            filter_state.vertex_offset = *cached_vbs.pOffsets;
            filter_state.srv = srv; srv->AddRef();
            filter_state.rtv_tex = rtv_tex; rtv_tex->AddRef();
            if (filter_state.t1) {
                if (filter_ss) need_filter = filter_next = true;
                else if (render_interp) need_filter = true;
            } else {
                if (render_interp) need_filter = filter_next = true;
            }
        }
    }
end:
        if (can_filter) {
            if (filter_next) goto done;
        }
    {
        DWORD bytecode_hash = cached_ps ?
            cached_ps->get_bytecode_hash() : 0;
        switch (bytecode_hash) {
            case PS_HASH_T3:
                if (filter_state.srv != srv)
                    goto draw_default;
                /* fall-through */

            case PS_HASH_T1:
                if (!render_linear) goto draw_default;
                set_psss(filter_temp.sampler_nn);
                inner->Draw(
                    VertexCount,
                    StartVertexLocation
                );
                restore_pssss();
                break;

            case PS_HASH_T2: {
                if (!(
                    (render_interp || render_linear) &&
                    filter_state.ps && filter_state.vs &&
                    filter_state.vertex_buffer
                )) goto draw_default;
                if (render_interp) {
                    ID3D10SamplerState *sss[2] = {
                        filter_temp.sampler_wrap,
                        filter_temp.sampler_linear
                    };
                    inner->PSSetSamplers(0, 2, sss);
                } else {
                    ID3D10SamplerState *sss[2] = {
                        cached_pssss[0]->get_inner(),
                        cached_pssss[1]->get_inner()
                    };
                    inner->PSSetSamplers(0, 2, sss);
                }
                if (render_linear) {
                    set_viewport(
                        filter_temp.tex_t2->width,
                        filter_temp.tex_t2->height
                    );
                    set_rtv(
                        filter_temp.tex_t2->rtv,
                        filter_temp.tex_t2->dsv
                    );
                }
                inner->Draw(
                    VertexCount,
                    StartVertexLocation
                );
                if (render_linear) {
                    restore_rtvs();
                    restore_vps();
                    set_srv(filter_temp.tex_t2->srv);
                    set_psss(filter_temp.sampler_linear);
                    set_filter_state_ps();
                    inner->VSSetShader(filter_state.vs->get_inner());
                    inner->IASetVertexBuffers(
                        0,
                        1,
                        &filter_state.vertex_buffer,
                        &filter_state.vertex_stride,
                        &filter_state.vertex_offset
                    );
                    inner->Draw(
                        VertexCount,
                        filter_state.start_vertex_location
                    );
                    inner->IASetVertexBuffers(
                        0,
                        1,
                        cached_vbs.ppVertexBuffers,
                        cached_vbs.pStrides,
                        cached_vbs.pOffsets
                    );
                    inner->VSSetShader(cached_vs->get_inner());
                    restore_ps();
                    restore_pssrvs();
                }
                restore_pssss();
                break;
            }

            default: {
draw_default:
                linear_conditions_begin();
                MyVertexBuffer_Logger vertices = {
                    .input_layout = cached_il,
                    .vertex_buffer = cached_vbs.vertex_buffers[0],
                    .stride = cached_vbs.pStrides[0],
                    .offset = cached_vbs.pOffsets[0],
                    .VertexCount = VertexCount,
                    .StartVertexLocation = StartVertexLocation
                };
                LOG_MFUN(_,
                    LogIf<1>{linear_restore},
                    LOG_ARG(vertices),
                    LOG_ARG(VertexCount),
                    LOG_ARG(StartVertexLocation),
                    LOG_ARG(linear_conditions)
                );
                inner->Draw(VertexCount, StartVertexLocation);
                linear_conditions_end();
                return;
                break;
            }
        }
    }
done:
        LOG_MFUN(_,
            LOG_ARG(VertexCount),
            LOG_ARG(StartVertexLocation)
        );
    }
};

void MyID3D10Device::set_overlay(Overlay *overlay) {
    impl->overlay = {overlay};
}
void MyID3D10Device::set_config(Config *config) {
    impl->config = config;
}

void MyID3D10Device::present() {
    impl->present();
}

void MyID3D10Device::resize_render_3d(UINT width, UINT height) {
    impl->resize_render_3d(width, height);
}

void MyID3D10Device::resize_buffers(UINT width, UINT height) {
    impl->resize_buffers(width, height);
}

void MyID3D10Device::resize_orig_buffers(UINT width, UINT height) {
    impl->cached_size.resize(width, height);
}

IUNKNOWN_IMPL(MyID3D10Device, ID3D10Device)

MyID3D10Device::MyID3D10Device(
    ID3D10Device **inner,
    UINT width,
    UINT height
) :
    impl(new Impl(inner, width, height))
{
    if (!xorshift128p_state_init_status) {
        void *key[] = {this, impl};
        MurmurHash3_x86_128(
            &key,
            sizeof(key),
            (uint32_t)*inner,
            xorshift128p_state
        );
        xorshift128p_state_init_status = true;
    }
    LOG_MFUN();
    *inner = this;
}

MyID3D10Device::~MyID3D10Device() {
    LOG_MFUN();
    delete impl;
}

void STDMETHODCALLTYPE MyID3D10Device::VSSetConstantBuffers(
    UINT StartSlot,
    UINT NumBuffers,
    ID3D10Buffer *const *ppConstantBuffers
) {
    auto constant_buffers = (MyID3D10Buffer *const *)ppConstantBuffers;
    LOG_MFUN(_,
        LOG_ARG(StartSlot),
        LOG_ARG(NumBuffers),
        LOG_ARG_TYPE(constant_buffers, ArrayLoggerDeref, NumBuffers)
    );
    ID3D10Buffer *buffers[NumBuffers];
    for (UINT i = 0; i < NumBuffers; ++i) {
        buffers[i] = constant_buffers[i] ?
            constant_buffers[i]->get_inner() : NULL;
    }
    if (NumBuffers) {
        memcpy(
            impl->cached_vscbs + StartSlot,
            buffers,
            sizeof(void *) * NumBuffers
        );
    }
    impl->inner->VSSetConstantBuffers(
        StartSlot,
        NumBuffers,
        NumBuffers ? buffers : ppConstantBuffers
    );
}

void STDMETHODCALLTYPE MyID3D10Device::PSSetShaderResources(
    UINT StartSlot,
    UINT NumViews,
    ID3D10ShaderResourceView *const *ppShaderResourceViews
) {
    auto shader_resource_views =
        (MyID3D10ShaderResourceView *const *)ppShaderResourceViews;
    ID3D10ShaderResourceView *srvs[NumViews];
    int srv_types[NumViews] = {};
    for (UINT i = 0; i < NumViews; ++i) {
        auto my_srv = shader_resource_views[i];
        srvs[i] = my_srv ? my_srv->get_inner() : NULL;
        if (LOG_STARTED &&
            my_srv &&
            my_srv->get_desc().ViewDimension ==
                D3D_SRV_DIMENSION_TEXTURE2D
        ) {
            auto tex = (MyID3D10Texture2D *)my_srv->get_resource();
            if (
                tex->get_orig_width() == impl->cached_size.sc_width &&
                tex->get_orig_height() == impl->cached_size.sc_height
            )
                srv_types[i] = 1;
            else if (
                almost_equal(
                    tex->get_orig_width(),
                    impl->cached_size.render_3d_width
                ) &&
                almost_equal(
                    tex->get_orig_height(),
                    impl->cached_size.render_3d_height
                )
            )
                srv_types[i] = -1;
        }
    }
    LOG_MFUN(_,
        LOG_ARG(StartSlot),
        LOG_ARG(NumViews),
        LOG_ARG_TYPE(shader_resource_views, ArrayLoggerDeref, NumViews),
        LOG_ARG_TYPE(srv_types, ArrayLoggerDeref, NumViews)
    );
    if (NumViews) {
        memcpy(
            impl->render_pssrvs + StartSlot,
            srvs,
            sizeof(void *) * NumViews
        );
        memcpy(
            impl->cached_pssrvs + StartSlot,
            ppShaderResourceViews,
            sizeof(void *) * NumViews
        );
    } else {
        memset(
            impl->render_pssrvs,
            0,
            sizeof(void *) * MAX_SHADER_RESOURCES
        );
        memset(
            impl->cached_pssrvs,
            0,
            sizeof(void *) * MAX_SHADER_RESOURCES
        );
    }
    impl->inner->PSSetShaderResources(
        StartSlot,
        NumViews,
        NumViews ? srvs : ppShaderResourceViews
    );
}

void STDMETHODCALLTYPE MyID3D10Device::PSSetShader(
    ID3D10PixelShader *pPixelShader
) {
    LOG_MFUN(_,
        LOG_ARG(pPixelShader)
    );
    auto &cached_ps = impl->cached_ps;
    cached_ps = (MyID3D10PixelShader *)pPixelShader;
    impl->inner->PSSetShader(cached_ps ? cached_ps->get_repl(impl->inner, impl->overlay) : NULL);
}

void STDMETHODCALLTYPE MyID3D10Device::PSSetSamplers(
    UINT StartSlot,
    UINT NumSamplers,
    ID3D10SamplerState *const *ppSamplers
) {
    LOG_MFUN(_,
        LOG_ARG(StartSlot),
        LOG_ARG(NumSamplers),
        LOG_ARG_TYPE(ppSamplers, ArrayLoggerDeref, NumSamplers)
    );
    ID3D10SamplerState *sss[NumSamplers];
    for (UINT i = 0; i < NumSamplers; ++i) {
        auto sampler = (MyID3D10SamplerState *)ppSamplers[i];
        sss[i] = sampler ?
            impl->render_linear && sampler->get_linear() ?
                sampler->get_linear() : sampler->get_inner() :
                NULL;
    }
    if (NumSamplers) {
        memcpy(
            impl->render_pssss + StartSlot,
            sss,
            sizeof(void *) * NumSamplers
        );
        memcpy(
            impl->cached_pssss + StartSlot,
            ppSamplers,
            sizeof(void *) * NumSamplers
        );
    } else {
        memset(impl->render_pssss, 0, sizeof(void *) * MAX_SAMPLERS);
        memset(impl->cached_pssss, 0, sizeof(void *) * MAX_SAMPLERS);
    }
    impl->inner->PSSetSamplers(
        StartSlot,
        NumSamplers,
        NumSamplers ? sss : ppSamplers
    );
}

void STDMETHODCALLTYPE MyID3D10Device::VSSetShader(
    ID3D10VertexShader *pVertexShader
) {
    LOG_MFUN(_,
        LOG_ARG(pVertexShader)
    );
    auto vertex_shader = (MyID3D10VertexShader *)pVertexShader;
    impl->cached_vs = vertex_shader;
    impl->inner->VSSetShader(
        vertex_shader ? vertex_shader->get_inner() : NULL
    );
}

#define ENUM_CLASS PIXEL_SHADER_ALPHA_DISCARD
const ENUM_MAP(ENUM_CLASS) PIXEL_SHADER_ALPHA_DISCARD_ENUM_MAP = {
    ENUM_CLASS_MAP_ITEM(UNKNOWN),
    ENUM_CLASS_MAP_ITEM(NONE),
    ENUM_CLASS_MAP_ITEM(EQUAL),
    ENUM_CLASS_MAP_ITEM(LESS),
    ENUM_CLASS_MAP_ITEM(LESS_OR_EQUAL),
};
#undef ENUM_CLASS

template<>
struct LogItem<PIXEL_SHADER_ALPHA_DISCARD> {
    PIXEL_SHADER_ALPHA_DISCARD a;
    void log_item(Logger *l) const {
        l->log_enum(PIXEL_SHADER_ALPHA_DISCARD_ENUM_MAP, a);
    }
};

template<>
struct LogItem<MyID3D10Device::Impl::LinearFilterConditions> {
    const MyID3D10Device::Impl::LinearFilterConditions*
        linear_conditions;
    void log_item(Logger *l) const {
        l->log_struct_begin();
#define STRUCT linear_conditions
        l->log_struct_members_named(
            LOG_STRUCT_MEMBER(alpha_discard)
        );
        auto
            sd_it = STRUCT->samplers_descs.begin(),
            sd_it_end = STRUCT->samplers_descs.end();
        auto
            td_it = STRUCT->texs_descs.begin(),
            td_it_end = STRUCT->texs_descs.end();
        for (
            size_t i = 0;
            td_it != td_it_end && sd_it != sd_it_end;
            ++td_it, ++sd_it, ++i
        ) {
            auto sd = *sd_it;
            auto td = *td_it;
            if (td) {
#define LOG_DESC_MEMBER(m) do { \
    l->log_struct_sep(); \
    l->log_item(i); \
    l->log_struct_member_access(); \
    l->log_item(#m); \
    l->log_assign(); \
    l->log_item(DESC->m); \
} while (0)
#define DESC td
                LOG_DESC_MEMBER(Width);
                LOG_DESC_MEMBER(Height);
#undef DESC
            }
            if (sd) {
#define DESC sd
                LOG_DESC_MEMBER(Filter);
#undef DESC
            }
#undef LOG_DESC_MEMBER
        }
#undef STRUCT
        l->log_struct_end();
    }
};

void STDMETHODCALLTYPE MyID3D10Device::DrawIndexed(
    UINT IndexCount,
    UINT StartIndexLocation,
    INT BaseVertexLocation
) {
    impl->set_render_vp();
    impl->linear_conditions_begin();
    auto &linear_conditions = impl->linear_conditions;
    MyIndexedVertexBuffer_Logger vertices = {
        .input_layout = impl->cached_il,
        .vertex_buffer = impl->cached_vbs.vertex_buffers[0],
        .stride = impl->cached_vbs.pStrides[0],
        .offset = impl->cached_vbs.pOffsets[0],
        .index_buffer = impl->cached_ib,
        .index_format = impl->cached_ib_format,
        .index_offset = impl->cached_ib_offset,
        .IndexCount = IndexCount,
        .StartIndexLocation = StartIndexLocation,
        .BaseVertexLocation = BaseVertexLocation,
    };
    LOG_MFUN(_,
        LogIf<1>{impl->linear_restore},
        LOG_ARG(vertices),
        LOG_ARG(IndexCount),
        LOG_ARG(StartIndexLocation),
        LOG_ARG(BaseVertexLocation),
        LOG_ARG(linear_conditions)
    );
    impl->inner->DrawIndexed(
        IndexCount,
        StartIndexLocation,
        BaseVertexLocation
    );
    impl->linear_conditions_end();
}

void STDMETHODCALLTYPE MyID3D10Device::Draw(
    UINT VertexCount,
    UINT StartVertexLocation
) {
    impl->Draw(VertexCount, StartVertexLocation);
}

void STDMETHODCALLTYPE MyID3D10Device::PSSetConstantBuffers(
    UINT StartSlot,
    UINT NumBuffers,
    ID3D10Buffer *const *ppConstantBuffers
) {
    auto constant_buffers = (MyID3D10Buffer *const *)ppConstantBuffers;
    LOG_MFUN(_,
        LOG_ARG(StartSlot),
        LOG_ARG(NumBuffers),
        LOG_ARG_TYPE(constant_buffers, ArrayLoggerDeref, NumBuffers)
    );
    ID3D10Buffer *buffers[NumBuffers];
    for (UINT i = 0; i < NumBuffers; ++i) {
        buffers[i] = constant_buffers[i] ?
            constant_buffers[i]->get_inner() : NULL;
    }
    if (NumBuffers) {
        memcpy(
            impl->render_pscbs + StartSlot,
            buffers,
            sizeof(void *) * NumBuffers
        );
        memcpy(
            impl->cached_pscbs + StartSlot,
            constant_buffers,
            sizeof(void *) * NumBuffers
        );
    }
    impl->inner->PSSetConstantBuffers(
        StartSlot,
        NumBuffers,
        NumBuffers ? buffers : ppConstantBuffers
    );
}

void STDMETHODCALLTYPE MyID3D10Device::IASetInputLayout(
    ID3D10InputLayout *pInputLayout
) {
    auto input_layout = (MyID3D10InputLayout *)pInputLayout;
    LOG_MFUN(_,
        LOG_ARG(input_layout)
    );
    impl->cached_il = input_layout;
    impl->inner->IASetInputLayout(
        input_layout ? input_layout->get_inner() : NULL
    );
}

void STDMETHODCALLTYPE MyID3D10Device::IASetVertexBuffers(
    UINT StartSlot,
    UINT NumBuffers,
    ID3D10Buffer *const *ppVertexBuffers,
    const UINT *pStrides,
    const UINT *pOffsets
) {
    auto vertex_buffers = (MyID3D10Buffer *const *)ppVertexBuffers;
    LOG_MFUN(_,
        LOG_ARG(StartSlot),
        LOG_ARG(NumBuffers),
        LOG_ARG_TYPE(vertex_buffers, ArrayLoggerDeref, NumBuffers),
        LOG_ARG_TYPE(pStrides, ArrayLoggerDeref, NumBuffers),
        LOG_ARG_TYPE(pOffsets, ArrayLoggerDeref, NumBuffers)
    );
    ID3D10Buffer *buffers[NumBuffers];
    for (UINT i = 0; i < NumBuffers; ++i) {
        buffers[i] = vertex_buffers[i] ?
            vertex_buffers[i]->get_inner() : NULL;
    }
    if (NumBuffers) {
        memcpy(
            impl->cached_vbs.vertex_buffers + StartSlot,
            vertex_buffers,
            sizeof(void *) * NumBuffers
        );
        memcpy(
            impl->cached_vbs.ppVertexBuffers + StartSlot,
            buffers,
            sizeof(void *) * NumBuffers
        );
        memcpy(
            impl->cached_vbs.pStrides + StartSlot,
            pStrides,
            sizeof(UINT) * NumBuffers
        );
        memcpy(
            impl->cached_vbs.pOffsets + StartSlot,
            pOffsets,
            sizeof(UINT) * NumBuffers
        );
    }
    impl->inner->IASetVertexBuffers(
        StartSlot,
        NumBuffers,
        NumBuffers ? buffers : ppVertexBuffers,
        pStrides,
        pOffsets
    );
}

void STDMETHODCALLTYPE MyID3D10Device::IASetIndexBuffer(
    ID3D10Buffer *pIndexBuffer,
    DXGI_FORMAT Format,
    UINT Offset
) {
    auto index_buffer = (MyID3D10Buffer *)pIndexBuffer;
    impl->cached_ib = index_buffer;
    impl->cached_ib_format = Format;
    impl->cached_ib_offset = Offset;
    LOG_MFUN(_,
        LOG_ARG(index_buffer),
        LOG_ARG(Format),
        LOG_ARG(Offset)
    );
    impl->inner->IASetIndexBuffer(
        index_buffer ? index_buffer->get_inner() : NULL,
        Format,
        Offset
    );
}

void STDMETHODCALLTYPE MyID3D10Device::DrawIndexedInstanced(
    UINT IndexCountPerInstance,
    UINT InstanceCount,
    UINT StartIndexLocation,
    INT BaseVertexLocation,
    UINT StartInstanceLocation
) {
    impl->set_render_vp();
    impl->linear_conditions_begin();
    auto &linear_conditions = impl->linear_conditions;
    LOG_MFUN(_,
        LOG_ARG(IndexCountPerInstance),
        LOG_ARG(InstanceCount),
        LOG_ARG(StartIndexLocation),
        LOG_ARG(BaseVertexLocation),
        LOG_ARG(StartInstanceLocation),
        LOG_ARG(linear_conditions)
    );
    impl->inner->DrawIndexedInstanced(
        IndexCountPerInstance,
        InstanceCount,
        StartIndexLocation,
        BaseVertexLocation,
        StartInstanceLocation
    );
    impl->linear_conditions_end();
}

void STDMETHODCALLTYPE MyID3D10Device::DrawInstanced(
    UINT VertexCountPerInstance,
    UINT InstanceCount,
    UINT StartVertexLocation,
    UINT StartInstanceLocation
) {
    impl->set_render_vp();
    impl->linear_conditions_begin();
    auto &linear_conditions = impl->linear_conditions;
    LOG_MFUN(_,
        LOG_ARG(VertexCountPerInstance),
        LOG_ARG(InstanceCount),
        LOG_ARG(StartVertexLocation),
        LOG_ARG(StartInstanceLocation),
        LOG_ARG(linear_conditions)
    );
    impl->inner->DrawInstanced(
        VertexCountPerInstance,
        InstanceCount,
        StartVertexLocation,
        StartInstanceLocation
    );
    impl->linear_conditions_end();
}

void STDMETHODCALLTYPE MyID3D10Device::GSSetConstantBuffers(
    UINT StartSlot,
    UINT NumBuffers,
    ID3D10Buffer *const *ppConstantBuffers
) {
    auto constant_buffers = (MyID3D10Buffer *const *)ppConstantBuffers;
    LOG_MFUN(_,
        LOG_ARG(StartSlot),
        LOG_ARG(NumBuffers),
        LOG_ARG_TYPE(constant_buffers, ArrayLoggerDeref, NumBuffers)
    );
    ID3D10Buffer *buffers[NumBuffers];
    for (UINT i = 0; i < NumBuffers; ++i) {
        buffers[i] = constant_buffers[i] ?
            constant_buffers[i]->get_inner() : NULL;
    }
    impl->inner->GSSetConstantBuffers(
        StartSlot,
        NumBuffers,
        NumBuffers ? buffers : ppConstantBuffers
    );
}

void STDMETHODCALLTYPE MyID3D10Device::GSSetShader(
    ID3D10GeometryShader *pShader
) {
    LOG_MFUN(_,
        LOG_ARG(pShader)
    );
    impl->cached_gs = pShader;
    impl->inner->GSSetShader(pShader);
}

void STDMETHODCALLTYPE MyID3D10Device::IASetPrimitiveTopology(
    D3D10_PRIMITIVE_TOPOLOGY Topology
) {
    LOG_MFUN(_,
        LOG_ARG(Topology)
    );
    impl->inner->IASetPrimitiveTopology(Topology);
if constexpr (ENABLE_SLANG_SHADER) {
    impl->cached_pt = Topology;
}
}

void STDMETHODCALLTYPE MyID3D10Device::VSSetShaderResources(
    UINT StartSlot,
    UINT NumViews,
    ID3D10ShaderResourceView *const *ppShaderResourceViews
) {
    auto shader_resource_views =
        (MyID3D10ShaderResourceView *const *)ppShaderResourceViews;
    LOG_MFUN(_,
        LOG_ARG(StartSlot),
        LOG_ARG(NumViews),
        LOG_ARG_TYPE(shader_resource_views, ArrayLoggerDeref, NumViews)
    );
    ID3D10ShaderResourceView *srvs[NumViews];
    for (UINT i = 0; i < NumViews; ++i) {
        srvs[i] = shader_resource_views[i] ?
            shader_resource_views[i]->get_inner() : NULL;
    }
    impl->inner->VSSetShaderResources(
        StartSlot,
        NumViews,
        NumViews ? srvs : ppShaderResourceViews
    );
}

void STDMETHODCALLTYPE MyID3D10Device::VSSetSamplers(
    UINT StartSlot,
    UINT NumSamplers,
    ID3D10SamplerState *const *ppSamplers
) {
    LOG_MFUN();
    ID3D10SamplerState *sss[NumSamplers];
    for (UINT i = 0; i < NumSamplers; ++i) {
        sss[i] = ppSamplers[i] ?
            ((MyID3D10SamplerState *)ppSamplers[i])->get_inner() :
            NULL;
    }
    impl->inner->VSSetSamplers(
        StartSlot,
        NumSamplers,
        NumSamplers ? sss : ppSamplers
    );
}

void STDMETHODCALLTYPE MyID3D10Device::SetPredication(
    ID3D10Predicate *pPredicate,
    WINBOOL PredicateValue
) {
    LOG_MFUN();
    impl->inner->SetPredication(
        pPredicate,
        PredicateValue
    );
}

void STDMETHODCALLTYPE MyID3D10Device::GSSetShaderResources(
    UINT StartSlot,
    UINT NumViews,
    ID3D10ShaderResourceView *const *ppShaderResourceViews
) {
    auto shader_resource_views =
        (MyID3D10ShaderResourceView *const *)ppShaderResourceViews;
    LOG_MFUN(_,
        LOG_ARG(StartSlot),
        LOG_ARG(NumViews),
        LOG_ARG_TYPE(shader_resource_views, ArrayLoggerDeref, NumViews)
    );
    ID3D10ShaderResourceView *srvs[NumViews];
    for (UINT i = 0; i < NumViews; ++i) {
        srvs[i] = shader_resource_views[i] ?
            shader_resource_views[i]->get_inner() :
            NULL;
    }
    impl->inner->GSSetShaderResources(
        StartSlot,
        NumViews,
        NumViews ? srvs : ppShaderResourceViews
    );
}

void STDMETHODCALLTYPE MyID3D10Device::GSSetSamplers(
    UINT StartSlot,
    UINT NumSamplers,
    ID3D10SamplerState *const *ppSamplers
) {
    LOG_MFUN();
    ID3D10SamplerState *sss[NumSamplers];
    for (UINT i = 0; i < NumSamplers; ++i) {
        sss[i] = ppSamplers[i] ?
            ((MyID3D10SamplerState *)ppSamplers[i])->get_inner() :
            NULL;
    }
    impl->inner->GSSetSamplers(
        StartSlot,
        NumSamplers,
        NumSamplers ? sss : ppSamplers
    );
}

void STDMETHODCALLTYPE MyID3D10Device::OMSetRenderTargets(
    UINT NumViews,
    ID3D10RenderTargetView *const *ppRenderTargetViews,
    ID3D10DepthStencilView *pDepthStencilView
) {
    auto render_target_view =
        (MyID3D10RenderTargetView *const *)ppRenderTargetViews;
    auto depth_stencil_view =
        (MyID3D10DepthStencilView *)pDepthStencilView;
    LOG_MFUN(_,
        LOG_ARG(NumViews),
        LOG_ARG_TYPE(render_target_view, ArrayLoggerDeref, NumViews),
        LOG_ARG(depth_stencil_view)
    );
    impl->reset_render_vp();
    impl->render_width = 0;
    impl->render_height = 0;
    impl->render_orig_width = 0;
    impl->render_orig_height = 0;
    impl->need_render_vp = false;
    impl->cached_rtv =
        NumViews && render_target_view ?
            *render_target_view :
            NULL;
    auto dsv = impl->cached_dsv = depth_stencil_view;
    ID3D10RenderTargetView *rtvs[NumViews];
    for (UINT i = 0; i < NumViews; ++i) {
        auto my_rtv = render_target_view[i];
        if (
            my_rtv &&
            my_rtv->get_desc().ViewDimension ==
                D3D10_RTV_DIMENSION_TEXTURE2D
        ) {
            impl->set_render_tex_views_and_update(
                my_rtv->get_resource(),
                true
            );
        }
        rtvs[i] = my_rtv ? my_rtv->get_inner() : NULL;
    }
    if (
        dsv &&
        dsv->get_desc().ViewDimension ==
            D3D10_DSV_DIMENSION_TEXTURE2D
    ) {
        impl->set_render_tex_views_and_update(
            dsv->get_resource()
        );
    }
    impl->inner->OMSetRenderTargets(
        NumViews,
        NumViews ? rtvs : ppRenderTargetViews,
        dsv ? dsv->get_inner() : NULL
    );
}

void STDMETHODCALLTYPE MyID3D10Device::OMSetBlendState(
    ID3D10BlendState *pBlendState,
    const FLOAT BlendFactor[4],
    UINT SampleMask
) {
    LOG_MFUN(_,
        LOG_ARG(pBlendState),
        LOG_ARG_TYPE(BlendFactor, ArrayLoggerDeref, 4),
        LOG_ARG_TYPE(SampleMask, NumHexLogger)
    );
    impl->inner->OMSetBlendState(
        pBlendState,
        BlendFactor,
        SampleMask
    );
if constexpr (ENABLE_SLANG_SHADER) {
    impl->cached_bs.pBlendState = pBlendState;
    impl->cached_bs.BlendFactor[0] = BlendFactor[0];
    impl->cached_bs.BlendFactor[1] = BlendFactor[1];
    impl->cached_bs.BlendFactor[2] = BlendFactor[2];
    impl->cached_bs.BlendFactor[3] = BlendFactor[3];
    impl->cached_bs.SampleMask = SampleMask;
}
}

void STDMETHODCALLTYPE MyID3D10Device::OMSetDepthStencilState(
    ID3D10DepthStencilState *pDepthStencilState,
    UINT StencilRef
) {
    auto depth_stencil_state =
        (MyID3D10DepthStencilState *)pDepthStencilState;
    impl->cached_dss = depth_stencil_state;
    LOG_MFUN(_,
        LOG_ARG(depth_stencil_state),
        LOG_ARG_TYPE(StencilRef, NumHexLogger)
    );
    impl->inner->OMSetDepthStencilState(
        depth_stencil_state ?
            depth_stencil_state->get_inner() : NULL,
        StencilRef
    );
}

void STDMETHODCALLTYPE MyID3D10Device::SOSetTargets(
    UINT NumBuffers,
    ID3D10Buffer *const *ppSOTargets,
    const UINT *pOffsets
) {
    LOG_MFUN();
    ID3D10Buffer *buffers[NumBuffers];
    for (UINT i = 0; i < NumBuffers; ++i) {
        buffers[i] = ppSOTargets[i] ?
            ((MyID3D10Buffer *)ppSOTargets[i])->get_inner() :
            NULL;
    }
    impl->inner->SOSetTargets(
        NumBuffers,
        NumBuffers ? buffers : ppSOTargets,
        pOffsets
    );
}

void STDMETHODCALLTYPE MyID3D10Device::DrawAuto(
) {
    impl->set_render_vp();
    impl->linear_conditions_begin();
    auto &linear_conditions = impl->linear_conditions;
    LOG_MFUN(_,
        LOG_ARG(linear_conditions)
    );
    impl->inner->DrawAuto();
    impl->linear_conditions_end();
}

void STDMETHODCALLTYPE MyID3D10Device::RSSetState(
    ID3D10RasterizerState *pRasterizerState
) {
    LOG_MFUN();
    impl->inner->RSSetState(
        pRasterizerState
    );
}

void STDMETHODCALLTYPE MyID3D10Device::RSSetViewports(
    UINT NumViewports,
    const D3D10_VIEWPORT *pViewports
) {
    LOG_MFUN(_,
        LOG_ARG(NumViewports),
        LOG_ARG_TYPE(pViewports, ArrayLoggerRef, NumViewports)
    );
    if (NumViewports) {
        impl->cached_vp = *pViewports;
    } else {
        impl->cached_vp = {};
    }
    impl->render_vp = impl->cached_vp;
    impl->is_render_vp = false;
    impl->inner->RSSetViewports(
        NumViewports,
        pViewports
    );
}

void STDMETHODCALLTYPE MyID3D10Device::RSSetScissorRects(
    UINT NumRects,
    const D3D10_RECT *pRects
) {
    LOG_MFUN();
    impl->inner->RSSetScissorRects(
        NumRects,
        pRects
    );
}

void STDMETHODCALLTYPE MyID3D10Device::CopySubresourceRegion(
    ID3D10Resource *pDstResource,
    UINT DstSubresource,
    UINT DstX,
    UINT DstY,
    UINT DstZ,
    ID3D10Resource *pSrcResource,
    UINT SrcSubresource,
    const D3D10_BOX *pSrcBox
) {
    LOG_MFUN(_,
        LOG_ARG_TYPE(pDstResource, MyID3D10Resource_Logger),
        LOG_ARG(DstSubresource),
        LOG_ARG(DstX),
        LOG_ARG(DstY),
        LOG_ARG(DstZ),
        LOG_ARG_TYPE(pSrcResource, MyID3D10Resource_Logger),
        LOG_ARG(SrcSubresource),
        LOG_ARG(pSrcBox)
    );
    D3D10_RESOURCE_DIMENSION dstType;
    pDstResource->GetType(&dstType);
    D3D10_RESOURCE_DIMENSION srcType;
    pSrcResource->GetType(&srcType);
    if (dstType != srcType) return;
    auto box = pSrcBox ? *pSrcBox : D3D10_BOX{};
    switch (dstType) {
        case D3D10_RESOURCE_DIMENSION_BUFFER: {
            auto buffer = (MyID3D10Buffer *)pDstResource;
            auto buffer_src = (MyID3D10Buffer *)pSrcResource;
            if (buffer->get_cached()) {
                if (buffer_src->get_cached() && buffer_src->get_cached_state()) {
                    if (pSrcBox) {
                        if (
                            buffer->get_cached_state() &&
                            pSrcBox->left < pSrcBox->right &&
                            pSrcBox->top < pSrcBox->bottom &&
                            pSrcBox->front < pSrcBox->back
                        ) {
                            memcpy(
                                buffer->get_cached() + DstX,
                                buffer_src->get_cached() + pSrcBox->left,
                                std::min(
                                    pSrcBox->right,
                                    buffer_src->get_desc().ByteWidth
                                ) - pSrcBox->left
                            );
                        }
                    } else {
                        memcpy(
                            buffer->get_cached(),
                            buffer_src->get_cached(),
                            std::min(
                                buffer->get_desc().ByteWidth,
                                buffer_src->get_desc().ByteWidth
                            )
                        );
                        buffer->get_cached_state() = true;
                    }
                } else {
                    buffer->get_cached_state() = false;
                }
            }
            pDstResource =
                buffer->get_inner();
            pSrcResource =
                buffer_src->get_inner();
            break;
        }

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            pDstResource =
                ((MyID3D10Texture1D *)pDstResource)->get_inner();
            pSrcResource =
                ((MyID3D10Texture1D *)pSrcResource)->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D: {
            auto tex_dst =
                (MyID3D10Texture2D *)pDstResource;
            auto tex_src =
                (MyID3D10Texture2D *)pSrcResource;
            if (impl->set_render_tex_views_and_update(tex_dst)) {
                DstX =
                    DstX *
                        tex_dst->get_desc().Width /
                        tex_dst->get_orig_width();
                DstY =
                    DstY *
                        tex_dst->get_desc().Height /
                        tex_dst->get_orig_height();
            }
            if (
                impl->set_render_tex_views_and_update(tex_src) &&
                pSrcBox
            ) {
                box.left =
                    pSrcBox->left *
                        tex_src->get_desc().Width /
                        tex_src->get_orig_width();
                box.top =
                    pSrcBox->top *
                        tex_src->get_desc().Height /
                        tex_src->get_orig_height();
                box.right =
                    pSrcBox->right *
                        tex_src->get_desc().Width /
                        tex_src->get_orig_width();
                box.bottom =
                    pSrcBox->bottom *
                        tex_src->get_desc().Height /
                        tex_src->get_orig_height();
            }
            pDstResource = tex_dst->get_inner();
            pSrcResource = tex_src->get_inner();
            break;
        }

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            pDstResource =
                ((MyID3D10Texture3D *)pDstResource)->get_inner();
            pSrcResource =
                ((MyID3D10Texture3D *)pSrcResource)->get_inner();
            break;

        default:
            break;
    }
    impl->inner->CopySubresourceRegion(
        pDstResource,
        DstSubresource,
        DstX,
        DstY,
        DstZ,
        pSrcResource,
        SrcSubresource,
        pSrcBox ? &box : NULL
    );
}

void STDMETHODCALLTYPE MyID3D10Device::CopyResource(
    ID3D10Resource *pDstResource,
    ID3D10Resource *pSrcResource
) {
    LOG_MFUN(_,
        LOG_ARG_TYPE(pDstResource, MyID3D10Resource_Logger),
        LOG_ARG_TYPE(pSrcResource, MyID3D10Resource_Logger)
    );
    impl->set_render_tex_views_and_update(pDstResource);
    impl->set_render_tex_views_and_update(pSrcResource);
    D3D10_RESOURCE_DIMENSION dstType;
    pDstResource->GetType(&dstType);
    D3D10_RESOURCE_DIMENSION srcType;
    pSrcResource->GetType(&srcType);
    if (dstType != srcType) return;
    switch (dstType) {
        case D3D10_RESOURCE_DIMENSION_BUFFER: {
            auto buffer = (MyID3D10Buffer *)pDstResource;
            auto buffer_src = (MyID3D10Buffer *)pSrcResource;
            if (buffer->get_cached()) {
                if (
                    buffer_src->get_cached() && buffer_src->get_cached_state() &&
                    buffer->get_desc().ByteWidth == buffer_src->get_desc().ByteWidth
                ) {
                    memcpy(
                        buffer->get_cached(),
                        buffer_src->get_cached(),
                        buffer->get_desc().ByteWidth
                    );
                    buffer->get_cached_state() = true;
                } else {
                    buffer->get_cached_state() = false;
                }
            }
            pDstResource =
                buffer->get_inner();
            pSrcResource =
                buffer_src->get_inner();
            break;
        }

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            pDstResource =
                ((MyID3D10Texture1D *)pDstResource)->get_inner();
            pSrcResource =
                ((MyID3D10Texture1D *)pSrcResource)->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            pDstResource =
                ((MyID3D10Texture2D *)pDstResource)->get_inner();
            pSrcResource =
                ((MyID3D10Texture2D *)pSrcResource)->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            pDstResource =
                ((MyID3D10Texture3D *)pDstResource)->get_inner();
            pSrcResource =
                ((MyID3D10Texture3D *)pSrcResource)->get_inner();
            break;

        default:
            break;
    }
    impl->inner->CopyResource(
        pDstResource,
        pSrcResource
    );
}

void STDMETHODCALLTYPE MyID3D10Device::UpdateSubresource(
    ID3D10Resource *pDstResource,
    UINT DstSubresource,
    const D3D10_BOX *pDstBox,
    const void *pSrcData,
    UINT SrcRowPitch,
    UINT SrcDepthPitch
) {
    D3D10_RESOURCE_DIMENSION dstType;
    pDstResource->GetType(&dstType);
    ID3D10Resource *resource_inner;
    int tex_type = 0;
    switch (dstType) {
        case D3D10_RESOURCE_DIMENSION_BUFFER: {
            auto buffer = (MyID3D10Buffer *)pDstResource;
            if (buffer->get_cached()) {
                if (pDstBox) {
                    if (
                        buffer->get_cached_state() && pSrcData &&
                        pDstBox->left < pDstBox->right &&
                        pDstBox->top < pDstBox->bottom &&
                        pDstBox->front < pDstBox->back
                    ) {
                        memcpy(
                            buffer->get_cached() +
                                pDstBox->left,
                            pSrcData,
                            std::min(
                                pDstBox->right,
                                buffer->get_desc().ByteWidth
                            ) - pDstBox->left
                        );
                    }
                } else {
                    if (pSrcData) {
                        memcpy(
                            buffer->get_cached(),
                            pSrcData,
                            buffer->get_desc().ByteWidth
                        );
                        buffer->get_cached_state() = true;
                    }
                }
            }
            resource_inner = buffer->get_inner();
            break;
        }

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            resource_inner =
                ((MyID3D10Texture1D *)pDstResource)->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D: {
            auto tex = (MyID3D10Texture2D *)pDstResource;
            resource_inner = tex->get_inner();
            if (!LOG_STARTED) break;
            auto &cached_size = impl->cached_size;
            if (
                tex->get_orig_width() == cached_size.sc_width &&
                tex->get_orig_height() == cached_size.sc_height
            )
                tex_type = 1;
            else if (
                almost_equal(
                    tex->get_orig_width(),
                    cached_size.render_3d_width
                ) &&
                almost_equal(
                    tex->get_orig_height(),
                    cached_size.render_3d_height
                )
            )
                tex_type = -1;
            break;
        }

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            resource_inner =
                ((MyID3D10Texture3D *)pDstResource)->get_inner();
            break;

        default:
            resource_inner = NULL;
            break;
    }
    LOG_MFUN(_,
        LOG_ARG_TYPE(pDstResource, MyID3D10Resource_Logger),
        LogIf<1>{tex_type},
        LOG_ARG(tex_type),
        LOG_ARG(DstSubresource),
        LOG_ARG(pDstBox),
        LOG_ARG(SrcRowPitch),
        LOG_ARG(SrcDepthPitch)
    );
    impl->inner->UpdateSubresource(
        resource_inner,
        DstSubresource,
        pDstBox,
        pSrcData,
        SrcRowPitch,
        SrcDepthPitch
    );
}

void STDMETHODCALLTYPE MyID3D10Device::ClearRenderTargetView(
    ID3D10RenderTargetView *pRenderTargetView,
    const FLOAT ColorRGBA[4]
) {
    auto render_target_view =
        (MyID3D10RenderTargetView *)pRenderTargetView;
    LOG_MFUN(_,
        LOG_ARG(render_target_view),
        LOG_ARG_TYPE(ColorRGBA, ArrayLoggerDeref, 4)
    );
    impl->inner->ClearRenderTargetView(
        render_target_view ? render_target_view->get_inner() : NULL,
        ColorRGBA
    );
}

void STDMETHODCALLTYPE MyID3D10Device::ClearDepthStencilView(
    ID3D10DepthStencilView *pDepthStencilView,
    UINT ClearFlags,
    FLOAT Depth,
    UINT8 Stencil
) {
    auto depth_stencil_view =
        (MyID3D10DepthStencilView *)pDepthStencilView;
    LOG_MFUN(_,
        LOG_ARG(depth_stencil_view),
        LOG_ARG_TYPE(ClearFlags, D3D10_CLEAR_Logger),
        LogIf<1>{ClearFlags & D3D10_CLEAR_DEPTH},
        LOG_ARG(Depth),
        LogIf<1>{ClearFlags & D3D10_CLEAR_STENCIL},
        LOG_ARG(Stencil)
    );
    impl->inner->ClearDepthStencilView(
        depth_stencil_view ? depth_stencil_view->get_inner() : NULL,
        ClearFlags,
        Depth,
        Stencil
    );
}

void STDMETHODCALLTYPE MyID3D10Device::GenerateMips(
    ID3D10ShaderResourceView *pShaderResourceView
) {
    LOG_MFUN();
    impl->inner->GenerateMips(
        ((MyID3D10ShaderResourceView *)pShaderResourceView)->
            get_inner()
    );
}

void STDMETHODCALLTYPE MyID3D10Device::ResolveSubresource(
    ID3D10Resource *pDstResource,
    UINT DstSubresource,
    ID3D10Resource *pSrcResource,
    UINT SrcSubresource,
    DXGI_FORMAT Format
) {
    LOG_MFUN(_,
        LOG_ARG_TYPE(pDstResource, MyID3D10Resource_Logger),
        LOG_ARG(DstSubresource),
        LOG_ARG_TYPE(pSrcResource, MyID3D10Resource_Logger),
        LOG_ARG(SrcSubresource),
        LOG_ARG(Format)
    );
    D3D10_RESOURCE_DIMENSION dstType;
    pDstResource->GetType(&dstType);
    D3D10_RESOURCE_DIMENSION srcType;
    pSrcResource->GetType(&srcType);
    switch (dstType) {
        case D3D10_RESOURCE_DIMENSION_BUFFER:
            pDstResource =
                ((MyID3D10Buffer *)pDstResource)->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            pDstResource =
                ((MyID3D10Texture1D *)pDstResource)->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            pDstResource =
                ((MyID3D10Texture2D *)pDstResource)->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            pDstResource =
                ((MyID3D10Texture3D *)pDstResource)->get_inner();
            break;

        default:
            break;
    }
    switch (srcType) {
        case D3D10_RESOURCE_DIMENSION_BUFFER:
            pSrcResource =
                ((MyID3D10Buffer *)pSrcResource)->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            pSrcResource =
                ((MyID3D10Texture1D *)pSrcResource)->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            pSrcResource =
                ((MyID3D10Texture2D *)pSrcResource)->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            pSrcResource =
                ((MyID3D10Texture3D *)pSrcResource)->get_inner();
            break;

        default:
            break;
    }
    impl->inner->ResolveSubresource(
        pDstResource,
        DstSubresource,
        pSrcResource,
        SrcSubresource,
        Format
    );
}

void STDMETHODCALLTYPE MyID3D10Device::VSGetConstantBuffers(
    UINT StartSlot,
    UINT NumBuffers,
    ID3D10Buffer **ppConstantBuffers
) {
    LOG_MFUN();
    impl->inner->VSGetConstantBuffers(
        StartSlot,
        NumBuffers,
        ppConstantBuffers
    );
    for (UINT i = 0; i < NumBuffers; ++i) {
        ppConstantBuffers[i] = ppConstantBuffers[i] ?
            cached_bs_map.find(ppConstantBuffers[i])->second :
            NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::PSGetShaderResources(
    UINT StartSlot,
    UINT NumViews,
    ID3D10ShaderResourceView **ppShaderResourceViews
) {
    LOG_MFUN();
    impl->inner->PSGetShaderResources(
        StartSlot,
        NumViews,
        ppShaderResourceViews
    );
    for (UINT i = 0; i < NumViews; ++i) {
        ppShaderResourceViews[i] = ppShaderResourceViews[i] ?
            cached_srvs_map.find(ppShaderResourceViews[i])->second :
            NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::PSGetShader(
    ID3D10PixelShader **ppPixelShader
) {
    LOG_MFUN();
    impl->inner->PSGetShader(
        ppPixelShader
    );
    if (ppPixelShader)
        *ppPixelShader = *ppPixelShader ?
            cached_pss_map.find(*ppPixelShader)->second :
            NULL;
}

void STDMETHODCALLTYPE MyID3D10Device::PSGetSamplers(
    UINT StartSlot,
    UINT NumSamplers,
    ID3D10SamplerState **ppSamplers
) {
    LOG_MFUN();
    impl->inner->PSGetSamplers(
        StartSlot,
        NumSamplers,
        ppSamplers
    );
    for (UINT i = 0; i < NumSamplers; ++i) {
        ppSamplers[i] = ppSamplers[i] ?
            cached_sss_map.find(ppSamplers[i])->second :
            NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::VSGetShader(
    ID3D10VertexShader **ppVertexShader
) {
    LOG_MFUN();
    impl->inner->VSGetShader(
        ppVertexShader
    );
    if (ppVertexShader)
        *ppVertexShader = *ppVertexShader ?
            cached_vss_map.find(*ppVertexShader)->second :
            NULL;
}

void STDMETHODCALLTYPE MyID3D10Device::PSGetConstantBuffers(
    UINT StartSlot,
    UINT NumBuffers,
    ID3D10Buffer **ppConstantBuffers
) {
    LOG_MFUN();
    impl->inner->PSGetConstantBuffers(
        StartSlot,
        NumBuffers,
        ppConstantBuffers
    );
    for (UINT i = 0; i < NumBuffers; ++i) {
        ppConstantBuffers[i] = ppConstantBuffers[i] ?
            cached_bs_map.find(ppConstantBuffers[i])->second :
            NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::IAGetInputLayout(
    ID3D10InputLayout **ppInputLayout
) {
    LOG_MFUN();
    impl->inner->IAGetInputLayout(
        ppInputLayout
    );
    if (ppInputLayout)
        *ppInputLayout = *ppInputLayout ?
            cached_ils_map.find(*ppInputLayout)->second :
            NULL;
}

void STDMETHODCALLTYPE MyID3D10Device::IAGetVertexBuffers(
    UINT StartSlot,
    UINT NumBuffers,
    ID3D10Buffer **ppVertexBuffers,
    UINT *pStrides,
    UINT *pOffsets
) {
    LOG_MFUN();
    impl->inner->IAGetVertexBuffers(
        StartSlot,
        NumBuffers,
        ppVertexBuffers,
        pStrides,
        pOffsets
    );
    for (UINT i = 0; i < NumBuffers; ++i) {
        ppVertexBuffers[i] = ppVertexBuffers[i] ?
            cached_bs_map.find(ppVertexBuffers[i])->second :
            NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::IAGetIndexBuffer(
    ID3D10Buffer **pIndexBuffer,
    DXGI_FORMAT *Format,
    UINT *Offset
) {
    LOG_MFUN();
    impl->inner->IAGetIndexBuffer(
        pIndexBuffer,
        Format,
        Offset
    );
    if (pIndexBuffer)
        *pIndexBuffer = *pIndexBuffer ?
            cached_bs_map.find(*pIndexBuffer)->second :
            NULL;
}

void STDMETHODCALLTYPE MyID3D10Device::GSGetConstantBuffers(
    UINT StartSlot,
    UINT NumBuffers,
    ID3D10Buffer **ppConstantBuffers
) {
    LOG_MFUN();
    impl->inner->GSGetConstantBuffers(
        StartSlot,
        NumBuffers,
        ppConstantBuffers
    );
    for (UINT i = 0; i < NumBuffers; ++i) {
        ppConstantBuffers[i] = ppConstantBuffers[i] ?
            cached_bs_map.find(ppConstantBuffers[i])->second :
            NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::GSGetShader(
    ID3D10GeometryShader **ppGeometryShader
) {
    LOG_MFUN();
    impl->inner->GSGetShader(
        ppGeometryShader
    );
}

void STDMETHODCALLTYPE MyID3D10Device::IAGetPrimitiveTopology(
    D3D10_PRIMITIVE_TOPOLOGY *pTopology
) {
    LOG_MFUN();
    impl->inner->IAGetPrimitiveTopology(
        pTopology
    );
}

void STDMETHODCALLTYPE MyID3D10Device::VSGetShaderResources(
    UINT StartSlot,
    UINT NumViews,
    ID3D10ShaderResourceView **ppShaderResourceViews
) {
    LOG_MFUN();
    impl->inner->VSGetShaderResources(
        StartSlot,
        NumViews,
        ppShaderResourceViews
    );
    for (UINT i = 0; i < NumViews; ++i) {
        ppShaderResourceViews[i] = ppShaderResourceViews[i] ?
            cached_srvs_map.find(ppShaderResourceViews[i])->second :
            NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::VSGetSamplers(
    UINT StartSlot,
    UINT NumSamplers,
    ID3D10SamplerState **ppSamplers
) {
    LOG_MFUN();
    impl->inner->VSGetSamplers(
        StartSlot,
        NumSamplers,
        ppSamplers
    );
    for (UINT i = 0; i < NumSamplers; ++i) {
        ppSamplers[i] = ppSamplers[i] ?
            cached_sss_map.find(ppSamplers[i])->second :
            NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::GetPredication(
    ID3D10Predicate **ppPredicate,
    WINBOOL *pPredicateValue
) {
    LOG_MFUN();
    impl->inner->GetPredication(
        ppPredicate,
        pPredicateValue
    );
}

void STDMETHODCALLTYPE MyID3D10Device::GSGetShaderResources(
    UINT StartSlot,
    UINT NumViews,
    ID3D10ShaderResourceView **ppShaderResourceViews
) {
    LOG_MFUN();
    impl->inner->GSGetShaderResources(
        StartSlot,
        NumViews,
        ppShaderResourceViews
    );
    for (UINT i = 0; i < NumViews; ++i) {
        ppShaderResourceViews[i] = ppShaderResourceViews[i] ?
            cached_srvs_map.find(ppShaderResourceViews[i])->second :
            NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::GSGetSamplers(
    UINT StartSlot,
    UINT NumSamplers,
    ID3D10SamplerState **ppSamplers
) {
    LOG_MFUN();
    impl->inner->GSGetSamplers(
        StartSlot,
        NumSamplers,
        ppSamplers
    );
    for (UINT i = 0; i < NumSamplers; ++i) {
        ppSamplers[i] =
            ppSamplers[i] ?
                cached_sss_map.find(ppSamplers[i])->second :
                NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::OMGetRenderTargets(
    UINT NumViews,
    ID3D10RenderTargetView **ppRenderTargetViews,
    ID3D10DepthStencilView **ppDepthStencilView
) {
    LOG_MFUN();
    impl->inner->OMGetRenderTargets(
        NumViews,
        ppRenderTargetViews,
        ppDepthStencilView
    );
    for (UINT i = 0; i < NumViews; ++i) {
        ppRenderTargetViews[i] = ppRenderTargetViews[i] ?
            cached_rtvs_map.find(ppRenderTargetViews[i])->second :
            NULL;
    }
    if (ppDepthStencilView)
        *ppDepthStencilView =
            *ppDepthStencilView ?
                cached_dsvs_map.find(*ppDepthStencilView)->second :
                NULL;
}

void STDMETHODCALLTYPE MyID3D10Device::OMGetBlendState(
    ID3D10BlendState **ppBlendState,
    FLOAT BlendFactor[4],
    UINT *pSampleMask
) {
    LOG_MFUN();
    impl->inner->OMGetBlendState(
        ppBlendState,
        BlendFactor,
        pSampleMask
    );
}

void STDMETHODCALLTYPE MyID3D10Device::OMGetDepthStencilState(
    ID3D10DepthStencilState **ppDepthStencilState,
    UINT *pStencilRef
) {
    impl->inner->OMGetDepthStencilState(
        ppDepthStencilState,
        pStencilRef
    );
    if (ppDepthStencilState)
        *ppDepthStencilState = *ppDepthStencilState ?
            cached_dsss_map.find(*ppDepthStencilState)->second :
            NULL;
    LOG_MFUN(_,
        LOG_ARG(*ppDepthStencilState),
        LOG_ARG_TYPE(*pStencilRef, NumHexLogger)
    );
}

void STDMETHODCALLTYPE MyID3D10Device::SOGetTargets(
    UINT NumBuffers,
    ID3D10Buffer **ppSOTargets,
    UINT *pOffsets
) {
    LOG_MFUN();
    impl->inner->SOGetTargets(
        NumBuffers,
        ppSOTargets,
        pOffsets
    );
    for (UINT i = 0; i < NumBuffers; ++i) {
        ppSOTargets[i] = ppSOTargets[i] ?
            cached_bs_map.find(ppSOTargets[i])->second :
            NULL;
    }
}

void STDMETHODCALLTYPE MyID3D10Device::RSGetState(
    ID3D10RasterizerState **ppRasterizerState
) {
    LOG_MFUN();
    impl->inner->RSGetState(
        ppRasterizerState
    );
}

void STDMETHODCALLTYPE MyID3D10Device::RSGetViewports(
    UINT *NumViewports,
    D3D10_VIEWPORT *pViewports
) {
    LOG_MFUN();
    impl->inner->RSGetViewports(
        NumViewports,
        pViewports
    );
}

void STDMETHODCALLTYPE MyID3D10Device::RSGetScissorRects(
    UINT *NumRects,
    D3D10_RECT *pRects
) {
    LOG_MFUN();
    impl->inner->RSGetScissorRects(
        NumRects,
        pRects
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::GetDeviceRemovedReason(
) {
    LOG_MFUN();
    return impl->inner->GetDeviceRemovedReason(
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::SetExceptionMode(
    UINT RaiseFlags
) {
    LOG_MFUN();
    return impl->inner->SetExceptionMode(
        RaiseFlags
    );
}

UINT STDMETHODCALLTYPE MyID3D10Device::GetExceptionMode(
) {
    LOG_MFUN();
    return impl->inner->GetExceptionMode(
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::GetPrivateData(
    REFGUID guid,
    UINT *pDataSize,
    void *pData
) {
    LOG_MFUN();
    return impl->inner->GetPrivateData(
        guid,
        pDataSize,
        pData
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::SetPrivateData(
    REFGUID guid,
    UINT DataSize,
    const void *pData
) {
    LOG_MFUN();
    return impl->inner->SetPrivateData(
        guid,
        DataSize,
        pData
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::SetPrivateDataInterface(
    REFGUID guid,
    const IUnknown *pData
) {
    LOG_MFUN();
    return impl->inner->SetPrivateDataInterface(
        guid,
        pData
    );
}

void STDMETHODCALLTYPE MyID3D10Device::ClearState(
) {
    LOG_MFUN();
    impl->inner->ClearState(
    );
}

void STDMETHODCALLTYPE MyID3D10Device::Flush(
) {
    LOG_MFUN();
    impl->inner->Flush(
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateBuffer(
    const D3D10_BUFFER_DESC *pDesc,
    const D3D10_SUBRESOURCE_DATA *pInitialData,
    ID3D10Buffer **ppBuffer
) {
    HRESULT ret = impl->inner->CreateBuffer(
        pDesc,
        pInitialData,
        ppBuffer
    );
    if (ret == S_OK) {
        new MyID3D10Buffer(
            ppBuffer,
            pDesc,
            xorshift128p(),
            pInitialData
        );
        LOG_MFUN(_,
            LOG_ARG(pDesc),
            LOG_ARG(*ppBuffer),
            ret
        );
    } else {
        LOG_MFUN(_,
            LOG_ARG(pDesc),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateTexture1D(
    const D3D10_TEXTURE1D_DESC *pDesc,
    const D3D10_SUBRESOURCE_DATA *pInitialData,
    ID3D10Texture1D **ppTexture1D
) {
    HRESULT ret = impl->inner->CreateTexture1D(
        pDesc,
        pInitialData,
        ppTexture1D
    );
    if (ret == S_OK) {
        new MyID3D10Texture1D(ppTexture1D, pDesc, xorshift128p());
        LOG_MFUN(_,
            LOG_ARG(pDesc),
            LOG_ARG(*ppTexture1D),
            ret
        );
    } else {
        LOG_MFUN(_,
            LOG_ARG(pDesc),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateTexture2D(
    const D3D10_TEXTURE2D_DESC *pDesc,
    const D3D10_SUBRESOURCE_DATA *pInitialData,
    ID3D10Texture2D **ppTexture2D
) {
    HRESULT ret = impl->inner->CreateTexture2D(
        pDesc,
        pInitialData,
        ppTexture2D
    );
    if (ret == S_OK) {
        new MyID3D10Texture2D(ppTexture2D, pDesc, xorshift128p());
        LOG_MFUN(_,
            LOG_ARG(pDesc),
            LOG_ARG(*ppTexture2D),
            ret
        );
    } else {
        LOG_MFUN(_,
            LOG_ARG(pDesc),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateTexture3D(
    const D3D10_TEXTURE3D_DESC *pDesc,
    const D3D10_SUBRESOURCE_DATA *pInitialData,
    ID3D10Texture3D **ppTexture3D
) {
    HRESULT ret = impl->inner->CreateTexture3D(
        pDesc,
        pInitialData,
        ppTexture3D
    );
    if (ret == S_OK) {
        new MyID3D10Texture3D(ppTexture3D, pDesc, xorshift128p());
        LOG_MFUN(_,
            LOG_ARG(pDesc),
            LOG_ARG(*ppTexture3D),
            ret
        );
    } else {
        LOG_MFUN(_,
            LOG_ARG(pDesc),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateShaderResourceView(
    ID3D10Resource *pResource,
    const D3D10_SHADER_RESOURCE_VIEW_DESC *pDesc,
    ID3D10ShaderResourceView **ppSRView
) {
    D3D10_RESOURCE_DIMENSION type;
    pResource->GetType(&type);
    ID3D10Resource *resource_inner = NULL;
    MyID3D10Texture2D *texture_2d = NULL;
    switch (type) {
        case D3D10_RESOURCE_DIMENSION_BUFFER:
            resource_inner =
                ((MyID3D10Buffer *)pResource)->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            resource_inner =
                ((MyID3D10Texture1D *)pResource)->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            texture_2d = (MyID3D10Texture2D *)pResource;
            resource_inner = texture_2d->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            resource_inner =
                ((MyID3D10Texture3D *)pResource)->get_inner();
            break;

        default:
            break;
    }
    HRESULT ret = impl->inner->CreateShaderResourceView(
        resource_inner,
        pDesc,
        ppSRView
    );
    if (ret == S_OK) {
        MyID3D10ShaderResourceView *srv =
            new MyID3D10ShaderResourceView(
                ppSRView,
                pDesc,
                pResource
            );
        LOG_MFUN(_,
            LOG_ARG_TYPE(pResource, MyID3D10Resource_Logger),
            LOG_ARG(pDesc),
            LOG_ARG(srv),
            ret
        );
        if (texture_2d) {
            texture_2d->get_srvs().insert(srv);
        }
    } else {
        LOG_MFUN(_,
            LOG_ARG_TYPE(pResource, MyID3D10Resource_Logger),
            LOG_ARG(pDesc),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateRenderTargetView(
    ID3D10Resource *pResource,
    const D3D10_RENDER_TARGET_VIEW_DESC *pDesc,
    ID3D10RenderTargetView **ppRTView
) {
    D3D10_RESOURCE_DIMENSION type;
    pResource->GetType(&type);
    ID3D10Resource *resource_inner = NULL;
    MyID3D10Texture2D *texture_2d = NULL;
    switch (type) {
        case D3D10_RESOURCE_DIMENSION_BUFFER:
            resource_inner =
                ((MyID3D10Buffer *)pResource)->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            resource_inner =
                ((MyID3D10Texture1D *)pResource)->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            texture_2d = (MyID3D10Texture2D *)pResource;
            resource_inner = texture_2d->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            resource_inner =
                ((MyID3D10Texture3D *)pResource)->get_inner();
            break;

        default:
            break;
    }
    HRESULT ret = impl->inner->CreateRenderTargetView(
        resource_inner,
        pDesc,
        ppRTView
    );
    if (ret == S_OK) {
        MyID3D10RenderTargetView *rtv =
            new MyID3D10RenderTargetView(
                ppRTView,
                pDesc,
                pResource
            );
        LOG_MFUN(_,
            LOG_ARG_TYPE(pResource, MyID3D10Resource_Logger),
            LOG_ARG(pDesc),
            LOG_ARG(rtv),
            ret
        );
        if (texture_2d) {
            texture_2d->get_rtvs().insert(rtv);
        }
    } else {
        LOG_MFUN(_,
            LOG_ARG_TYPE(pResource, MyID3D10Resource_Logger),
            LOG_ARG(pDesc),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateDepthStencilView(
    ID3D10Resource *pResource,
    const D3D10_DEPTH_STENCIL_VIEW_DESC *pDesc,
    ID3D10DepthStencilView **ppDepthStencilView
) {
    D3D10_RESOURCE_DIMENSION type;
    pResource->GetType(&type);
    ID3D10Resource *resource_inner = NULL;
    MyID3D10Texture2D *texture_2d = NULL;
    switch (type) {
        case D3D10_RESOURCE_DIMENSION_BUFFER:
            resource_inner =
                ((MyID3D10Buffer *)pResource)->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE1D:
            resource_inner =
                ((MyID3D10Texture1D *)pResource)->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            texture_2d = (MyID3D10Texture2D *)pResource;
            resource_inner = texture_2d->get_inner();
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            resource_inner =
                ((MyID3D10Texture3D *)pResource)->get_inner();
            break;

        default:
            break;
    }
    HRESULT ret = impl->inner->CreateDepthStencilView(
        resource_inner,
        pDesc,
        ppDepthStencilView
    );
    if (ret == S_OK) {
        MyID3D10DepthStencilView *dsv =
            new MyID3D10DepthStencilView(
                ppDepthStencilView,
                pDesc,
                pResource
            );
        LOG_MFUN(_,
            LOG_ARG_TYPE(pResource, MyID3D10Resource_Logger),
            LOG_ARG(pDesc),
            LOG_ARG(dsv),
            ret
        );
        if (texture_2d) {
            texture_2d->get_dsvs().insert(dsv);
        }
    } else {
        LOG_MFUN(_,
            LOG_ARG_TYPE(pResource, MyID3D10Resource_Logger),
            LOG_ARG(pDesc),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateInputLayout(
    const D3D10_INPUT_ELEMENT_DESC *pInputElementDescs,
    UINT NumElements,
    const void *pShaderBytecodeWithInputSignature,
    SIZE_T BytecodeLength,
    ID3D10InputLayout **ppInputLayout
) {
    HRESULT ret = impl->inner->CreateInputLayout(
        pInputElementDescs,
        NumElements,
        pShaderBytecodeWithInputSignature,
        BytecodeLength,
        ppInputLayout
    );
    if (ret == S_OK) {
        new MyID3D10InputLayout(
            ppInputLayout,
            pInputElementDescs,
            NumElements
        );
    }
    LOG_MFUN(_,
        LOG_ARG_TYPE(
            pInputElementDescs,
            ArrayLoggerRef,
            NumElements
        ),
        LogIf<1>{ret == S_OK},
        LOG_ARG_TYPE(
            ppInputLayout,
            ArrayLoggerDeref,
            NumElements
        ),
        ret
    );
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateVertexShader(
    const void *pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D10VertexShader **ppVertexShader
) {
    HRESULT ret = impl->inner->CreateVertexShader(
        pShaderBytecode,
        BytecodeLength,
        ppVertexShader
    );
    if (ret == S_OK) {
        ShaderLogger shader_source{pShaderBytecode};
        DWORD hash;
        MurmurHash3_x86_32(
            pShaderBytecode,
            BytecodeLength,
            0,
            &hash
        );
        ID3D10GeometryShader *pGeometryShader = NULL;
        std::vector<D3D10_SO_DECLARATION_ENTRY> decl_entries;
        HRESULT sogs_ret = 0;
if constexpr (ENABLE_LOGGER) {
        decl_entries.push_back(D3D10_SO_DECLARATION_ENTRY{
            .SemanticName = "SV_Position",
            .ComponentCount = 4,
        });
        std::string source = shader_source.source;
        std::regex texcoord_regex(R"(out\s+vec4\s+vs_TEXCOORD(\d+);)");
        std::smatch texcoord_sm;
        while (std::regex_search(
            source,
            texcoord_sm,
            texcoord_regex
        )) {
            decl_entries.push_back(D3D10_SO_DECLARATION_ENTRY{
                .SemanticName = "TEXCOORD",
                .SemanticIndex = std::stoul(texcoord_sm[1]),
                .ComponentCount = 4,
            });
            source = texcoord_sm.suffix();
        }
        sogs_ret = impl->inner->CreateGeometryShaderWithStreamOutput(
            pShaderBytecode,
            BytecodeLength,
            decl_entries.data(),
            decl_entries.size(),
            sizeof(float) * 4 * decl_entries.size(),
            &pGeometryShader
        );
        if (sogs_ret != S_OK)
            pGeometryShader = NULL;
}
        new MyID3D10VertexShader(
            ppVertexShader,
            hash,
            BytecodeLength,
            shader_source.source,
            std::move(decl_entries),
            pGeometryShader
        );
        LOG_MFUN(_,
            LOG_ARG(sogs_ret),
            LOG_ARG(shader_source),
            LOG_ARG(BytecodeLength),
            LOG_ARG(*ppVertexShader),
            ret
        );
    } else {
        LOG_MFUN(_,
            LOG_ARG(BytecodeLength),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateGeometryShader(
    const void *pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D10GeometryShader **ppGeometryShader
) {
    HRESULT ret = impl->inner->CreateGeometryShader(
        pShaderBytecode,
        BytecodeLength,
        ppGeometryShader
    );
    if (ret == S_OK) {
        LOG_MFUN(_,
            LOG_ARG_TYPE(pShaderBytecode, ShaderLogger),
            LOG_ARG(BytecodeLength),
            LOG_ARG(*ppGeometryShader),
            ret
        );
    } else {
        LOG_MFUN(_,
            LOG_ARG(BytecodeLength),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::
CreateGeometryShaderWithStreamOutput(
    const void *pShaderBytecode,
    SIZE_T BytecodeLength,
    const D3D10_SO_DECLARATION_ENTRY *pSODeclaration,
    UINT NumEntries,
    UINT OutputStreamStride,
    ID3D10GeometryShader **ppGeometryShader
) {
    LOG_MFUN();
    return impl->inner->CreateGeometryShaderWithStreamOutput(
        pShaderBytecode,
        BytecodeLength,
        pSODeclaration,
        NumEntries,
        OutputStreamStride,
        ppGeometryShader
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreatePixelShader(
    const void *pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D10PixelShader **ppPixelShader
) {
    HRESULT ret = impl->inner->CreatePixelShader(
        pShaderBytecode,
        BytecodeLength,
        ppPixelShader
    );
    if (ret == S_OK) {
        ShaderLogger shader_source{pShaderBytecode};
        DWORD hash;
        MurmurHash3_x86_32(
            pShaderBytecode,
            BytecodeLength,
            0,
            &hash
        );
        new MyID3D10PixelShader(
            ppPixelShader,
            hash,
            BytecodeLength,
            shader_source.source
        );
        LOG_MFUN(_,
            LOG_ARG(shader_source),
            LOG_ARG_TYPE(hash, NumHexLogger),
            LOG_ARG(BytecodeLength),
            LOG_ARG(*ppPixelShader),
            ret
        );
    } else {
        LOG_MFUN(_,
            LOG_ARG(BytecodeLength),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateBlendState(
    const D3D10_BLEND_DESC *pBlendStateDesc,
    ID3D10BlendState **ppBlendState
) {
    HRESULT ret = impl->inner->CreateBlendState(
        pBlendStateDesc,
        ppBlendState
    );
    if (ret == S_OK) {
        LOG_MFUN(_,
            LOG_ARG(pBlendStateDesc),
            LOG_ARG(*ppBlendState)
        );
    } else {
        LOG_MFUN(_,
            LOG_ARG(pBlendStateDesc)
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateDepthStencilState(
    const D3D10_DEPTH_STENCIL_DESC *pDepthStencilDesc,
    ID3D10DepthStencilState **ppDepthStencilState
) {
    HRESULT ret = impl->inner->CreateDepthStencilState(
        pDepthStencilDesc,
        ppDepthStencilState
    );
    if (ret == S_OK) {
        new MyID3D10DepthStencilState(
            ppDepthStencilState,
            pDepthStencilDesc
        );
        LOG_MFUN(_,
            LOG_ARG(pDepthStencilDesc),
            LOG_ARG(*ppDepthStencilState)
        );
    } else {
        LOG_MFUN(_,
            LOG_ARG(pDepthStencilDesc)
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateRasterizerState(
    const D3D10_RASTERIZER_DESC *pRasterizerDesc,
    ID3D10RasterizerState **ppRasterizerState
) {
    LOG_MFUN();
    return impl->inner->CreateRasterizerState(
        pRasterizerDesc,
        ppRasterizerState
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateSamplerState(
    const D3D10_SAMPLER_DESC *pSamplerDesc,
    ID3D10SamplerState **ppSamplerState
) {
    HRESULT ret = impl->inner->CreateSamplerState(
        pSamplerDesc,
        ppSamplerState
    );
    if (ret == S_OK) {
        ID3D10SamplerState *linear;
        if (
            pSamplerDesc->Filter ==
                D3D10_FILTER_MIN_MAG_MIP_LINEAR
        ) {
            linear = *ppSamplerState;
            linear->AddRef();
        } else {
            D3D10_SAMPLER_DESC desc = *pSamplerDesc;
            desc.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
            if (S_OK != impl->inner->CreateSamplerState(
                &desc,
                &linear
            )) {
                linear = NULL;
            }
        }
        new MyID3D10SamplerState(
            ppSamplerState,
            pSamplerDesc,
            linear
        );
        LOG_MFUN(_,
            LOG_ARG(pSamplerDesc),
            LOG_ARG(*ppSamplerState),
            ret
        );
    } else {
        LOG_MFUN(_,
            LOG_ARG(pSamplerDesc),
            ret
        );
    }
    return ret;
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateQuery(
    const D3D10_QUERY_DESC *pQueryDesc,
    ID3D10Query **ppQuery
) {
    LOG_MFUN();
    return impl->inner->CreateQuery(
        pQueryDesc,
        ppQuery
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreatePredicate(
    const D3D10_QUERY_DESC *pPredicateDesc,
    ID3D10Predicate **ppPredicate
) {
    LOG_MFUN();
    return impl->inner->CreatePredicate(
        pPredicateDesc,
        ppPredicate
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CreateCounter(
    const D3D10_COUNTER_DESC *pCounterDesc,
    ID3D10Counter **ppCounter
) {
    LOG_MFUN();
    return impl->inner->CreateCounter(
        pCounterDesc,
        ppCounter
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CheckFormatSupport(
    DXGI_FORMAT Format,
    UINT *pFormatSupport
) {
    LOG_MFUN();
    return impl->inner->CheckFormatSupport(
        Format,
        pFormatSupport
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::
CheckMultisampleQualityLevels(
    DXGI_FORMAT Format,
    UINT SampleCount,
    UINT *pNumQualityLevels
) {
    LOG_MFUN();
    return impl->inner->CheckMultisampleQualityLevels(
        Format,
        SampleCount,
        pNumQualityLevels
    );
}

void STDMETHODCALLTYPE MyID3D10Device::CheckCounterInfo(
    D3D10_COUNTER_INFO *pCounterInfo
) {
    LOG_MFUN();
    impl->inner->CheckCounterInfo(
        pCounterInfo
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::CheckCounter(
    const D3D10_COUNTER_DESC *pDesc,
    D3D10_COUNTER_TYPE *pType,
    UINT *pActiveCounters,
    char *name,
    UINT *pNameLength,
    char *units,
    UINT *pUnitsLength,
    char *description,
    UINT *pDescriptionLength
) {
    LOG_MFUN();
    return impl->inner->CheckCounter(
        pDesc,
        pType,
        pActiveCounters,
        name,
        pNameLength,
        units,
        pUnitsLength,
        description,
        pDescriptionLength
    );
}

UINT STDMETHODCALLTYPE MyID3D10Device::GetCreationFlags(
) {
    LOG_MFUN();
    return impl->inner->GetCreationFlags(
    );
}

HRESULT STDMETHODCALLTYPE MyID3D10Device::OpenSharedResource(
    HANDLE hResource,
    REFIID ReturnedInterface,
    void **ppResource
) {
    LOG_MFUN();
    return impl->inner->OpenSharedResource(
        hResource,
        ReturnedInterface,
        ppResource
    );
}

void STDMETHODCALLTYPE MyID3D10Device::SetTextFilterSize(
    UINT Width,
    UINT Height
) {
    LOG_MFUN();
    impl->inner->SetTextFilterSize(
        Width,
        Height
    );
}

void STDMETHODCALLTYPE MyID3D10Device::GetTextFilterSize(
    UINT *pWidth,
    UINT *pHeight
) {
    LOG_MFUN();
    impl->inner->GetTextFilterSize(
        pWidth,
        pHeight
    );
}
