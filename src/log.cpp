#include "log.h"
#include "overlay.h"
#include "conf.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#include "../HLSLcc/include/hlslcc.h"
#pragma GCC diagnostic pop

StringLogger::StringLogger(LPCSTR a) : a(a) {}

RawStringLogger::RawStringLogger(LPCSTR a, LPCSTR p) : a(a), p(p) {}

CharLogger::CharLogger(CHAR a) : a(a) {}

ShaderLogger::ShaderLogger(const void *a, SIZE_T n) : a(a), n(n) {}

D3D10_CLEAR_Logger::D3D10_CLEAR_Logger(UINT a) : a(a) {}

D3D10_BIND_Logger::D3D10_BIND_Logger(UINT a) : a(a) {}

D3D10_CPU_ACCESS_Logger::D3D10_CPU_ACCESS_Logger(UINT a) : a(a) {}

D3D10_RESOURCE_MISC_Logger::D3D10_RESOURCE_MISC_Logger(UINT a) : a(a) {}

bool Logger::file_init() {
    if (file != INVALID_HANDLE_VALUE) return true;
    file = CreateFile(
        file_name,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (file == INVALID_HANDLE_VALUE) {
        Overlay::push_text("Failed to create log file ", file_name);
        return false;
    }
    return true;
}

void Logger::file_shutdown() {
    if (file != INVALID_HANDLE_VALUE) {
        CloseHandle(file);
        file = INVALID_HANDLE_VALUE;
    }
}

bool Logger::log_begin() {
    if (!started) goto e_started;
    EnterCriticalSection(&oss_cs);
    if (file == INVALID_HANDLE_VALUE && !file_init()) goto e_file_init;
    oss.clear();
    oss.seekp(0);
    log_items_base("(", frame_count, ")(", GetCurrentThreadId(), ")");
    return true;

e_file_init:
    LeaveCriticalSection(&oss_cs);
e_started:
    return false;
}

void Logger::log_end() {
    log_item('\n');
    DWORD written = 0;
    std::ostringstream::pos_type len = oss.tellp();
    if (len != -1) {
        WriteFile(file, oss.str().c_str(), len, &written, NULL);
    }
    LeaveCriticalSection(&oss_cs);
}

void Logger::log_assign() {
    log_item(" = ");
}

void Logger::log_sep() {
    log_item(", ");
}

void Logger::log_fun_name(LPCSTR n) {
    log_item(n);
}

void Logger::log_fun_begin() {
    log_item('(');
}

void Logger::log_fun_args_next() {
    log_fun_end();
}

void Logger::log_fun_sep() {
    log_sep();
}

void Logger::log_fun_end() {
    log_item(')');
}

void Logger::log_struct_begin() {
    log_item('{');
}

void Logger::log_struct_sep() {
    log_sep();
}

void Logger::log_struct_end() {
    log_item('}');
}

void Logger::log_array_begin() {
    log_item('[');
}

void Logger::log_array_sep() {
    log_sep();
}

void Logger::log_array_end() {
    log_item(']');
}

void Logger::log_string_begin() {
    log_item('"');
}

void Logger::log_string_end() {
    log_item('"');
}

void Logger::log_char_begin() {
    log_item('\'');
}

void Logger::log_char_end() {
    log_item('\'');
}

void Logger::log_null() {
    log_item("NULL");
}

void Logger::log_item(LPCWSTR a) {
    int len = WideCharToMultiByte(CP_UTF8, 0, a, -1, NULL, 0, NULL, NULL);
    CHAR b[len] = {};
    WideCharToMultiByte(CP_UTF8, 0, a, -1, b, len, NULL, NULL);
    log_item(b);
}

void Logger::log_flag_sep() {
    log_item(" | ");
}

void Logger::log_item(StringLogger a) {
    log_string_begin();
    for (LPCSTR s = a.a; *s; ++s) {
        if (*s == '\\') log_item("\\\\");
        else if (*s == '"') log_item("\\\"");
        else log_item(*s);
    }
    log_string_end();
}

void Logger::log_item(RawStringLogger a) {
    log_item("R\"");
    log_item(a.p);
    log_item('(');
    log_item(a.a);
    log_item(')');
    log_item(a.p);
    log_item('"');
}

void Logger::log_item(CharLogger a) {
    log_char_begin();
    if (a.a == '\\') log_item("\\\\");
    else if (a.a == '\'') log_item("\\'");
    else log_item(a.a);
    log_char_end();
}

const ENUM_MAP(GLLang) GLLang_ENUM_MAP = {
    ENUM_MAP_ITEM(LANG_DEFAULT)
    ENUM_MAP_ITEM(LANG_ES_100)
    ENUM_MAP_ITEM(LANG_ES_300)
    ENUM_MAP_ITEM(LANG_ES_310)
    ENUM_MAP_ITEM(LANG_120)
    ENUM_MAP_ITEM(LANG_130)
    ENUM_MAP_ITEM(LANG_140)
    ENUM_MAP_ITEM(LANG_150)
    ENUM_MAP_ITEM(LANG_330)
    ENUM_MAP_ITEM(LANG_400)
    ENUM_MAP_ITEM(LANG_410)
    ENUM_MAP_ITEM(LANG_420)
    ENUM_MAP_ITEM(LANG_430)
    ENUM_MAP_ITEM(LANG_440)
    ENUM_MAP_ITEM(LANG_METAL)
};

const char *hlslcc_prefix = "HLSLcc_";
#define LOG_SHADER_DELIM() do { \
    log_item(hlslcc_prefix); \
    log_enum(GLLang_ENUM_MAP, result.GLSLLanguage); \
} while (0)
void Logger::log_item(ShaderLogger a) {
    char shader[a.n + 1] = {};
    memcpy(shader, a.a, a.n);
    shader[a.n] = 0;

    GlExtensions extensions{};
    HLSLccSamplerPrecisionInfo samplerPrecisions{};
    HLSLccReflection reflectionCallbacks{};
    GLSLShader result{};

    TranslateHLSLFromMem(
        shader,
        0,
        LANG_DEFAULT,
        &extensions,
        NULL,
        samplerPrecisions,
        reflectionCallbacks,
        &result
    );

    log_item("R\"");
    LOG_SHADER_DELIM();
    log_item('(');
    log_item(result.sourceCode);
    log_item(')');
    LOG_SHADER_DELIM();
    log_item('"');
}

const FLAG_MAP(UINT) D3D10_CLEAR_FLAG_MAP = {
    ENUM_MAP_ITEM(D3D10_CLEAR_DEPTH)
    ENUM_MAP_ITEM(D3D10_CLEAR_STENCIL)
};

void Logger::log_item(D3D10_CLEAR_Logger a) {
    log_flag(D3D10_CLEAR_FLAG_MAP, a.a);
}

const FLAG_MAP(UINT) D3D10_BIND_FLAG_MAP = {
    ENUM_MAP_ITEM(D3D10_BIND_VERTEX_BUFFER)
    ENUM_MAP_ITEM(D3D10_BIND_INDEX_BUFFER)
    ENUM_MAP_ITEM(D3D10_BIND_CONSTANT_BUFFER)
    ENUM_MAP_ITEM(D3D10_BIND_SHADER_RESOURCE)
    ENUM_MAP_ITEM(D3D10_BIND_STREAM_OUTPUT)
    ENUM_MAP_ITEM(D3D10_BIND_RENDER_TARGET)
    ENUM_MAP_ITEM(D3D10_BIND_DEPTH_STENCIL)
};

void Logger::log_item(D3D10_BIND_Logger a) {
    log_flag(D3D10_BIND_FLAG_MAP, a.a);
}

const FLAG_MAP(UINT) D3D10_CPU_ACCESS_FLAG_MAP = {
    ENUM_MAP_ITEM(D3D10_CPU_ACCESS_WRITE)
    ENUM_MAP_ITEM(D3D10_CPU_ACCESS_READ)
};

void Logger::log_item(D3D10_CPU_ACCESS_Logger a) {
    log_flag(D3D10_CPU_ACCESS_FLAG_MAP, a.a);
}

const FLAG_MAP(UINT) D3D10_RESOURCE_MISC_FLAG_MAP = {
    ENUM_MAP_ITEM(D3D10_RESOURCE_MISC_GENERATE_MIPS)
    ENUM_MAP_ITEM(D3D10_RESOURCE_MISC_SHARED)
    ENUM_MAP_ITEM(D3D10_RESOURCE_MISC_TEXTURECUBE)
    ENUM_MAP_ITEM(D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX)
    ENUM_MAP_ITEM(D3D10_RESOURCE_MISC_GDI_COMPATIBLE)
};

void Logger::log_item(D3D10_RESOURCE_MISC_Logger a) {
    log_flag(D3D10_RESOURCE_MISC_FLAG_MAP, a.a);
}

void Logger::log_item(HotkeyLogger a) {
    if (a.a.size()) {
        oss << std::hex << std::showbase;
        bool first = true;
        for (BYTE vk : a.a) {
            if (first) {
                first = false;
            } else {
                log_item('+');
            }
            oss << +vk;
        }
        oss.flags(std::ios::fmtflags{});
    } else {
        log_item("None");
    }
}

void Logger::log_item(const GUID *guid) {
    if (!guid) { log_null(); return; }
    log_struct(
        NumHexLogger(guid->Data1),
        NumHexLogger(guid->Data2),
        NumHexLogger(guid->Data3),
        NumHexLogger(guid->Data4[0]),
        NumHexLogger(guid->Data4[1]),
        NumHexLogger(guid->Data4[2]),
        NumHexLogger(guid->Data4[3]),
        NumHexLogger(guid->Data4[4]),
        NumHexLogger(guid->Data4[5]),
        NumHexLogger(guid->Data4[6]),
        NumHexLogger(guid->Data4[7])
    );
}

void Logger::log_item(const D3D10_SAMPLER_DESC *sampler_desc) {
    if (!sampler_desc) { log_null(); return; }
    log_struct_begin();
#define STRUCT sampler_desc
    log_struct_members_named(
        LOG_STRUCT_MEMBER(Filter),
        LOG_STRUCT_MEMBER(AddressU),
        LOG_STRUCT_MEMBER(AddressV),
        LOG_STRUCT_MEMBER(AddressW),
        LOG_STRUCT_MEMBER(MipLODBias),
        LOG_STRUCT_MEMBER(MaxAnisotropy),
        LOG_STRUCT_MEMBER(ComparisonFunc)
    );
    if (
        sampler_desc->AddressU == D3D10_TEXTURE_ADDRESS_BORDER ||
        sampler_desc->AddressV == D3D10_TEXTURE_ADDRESS_BORDER ||
        sampler_desc->AddressW == D3D10_TEXTURE_ADDRESS_BORDER
    ) {
    }
    log_struct_sep();
    log_struct_members_named(
        LOG_STRUCT_MEMBER(MinLOD),
        LOG_STRUCT_MEMBER(MaxLOD)
    );
#undef STRUCT
    log_struct_end();
}

const ENUM_MAP(D3D10_FILTER) D3D10_FILTER_ENUM_MAP = {
    ENUM_MAP_ITEM(D3D10_FILTER_MIN_MAG_MIP_POINT)
    ENUM_MAP_ITEM(D3D10_FILTER_MIN_MAG_POINT_MIP_LINEAR)
    ENUM_MAP_ITEM(D3D10_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT)
    ENUM_MAP_ITEM(D3D10_FILTER_MIN_POINT_MAG_MIP_LINEAR)
    ENUM_MAP_ITEM(D3D10_FILTER_MIN_LINEAR_MAG_MIP_POINT)
    ENUM_MAP_ITEM(D3D10_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR)
    ENUM_MAP_ITEM(D3D10_FILTER_MIN_MAG_LINEAR_MIP_POINT)
    ENUM_MAP_ITEM(D3D10_FILTER_MIN_MAG_MIP_LINEAR)
    ENUM_MAP_ITEM(D3D10_FILTER_ANISOTROPIC)
    ENUM_MAP_ITEM(D3D10_FILTER_COMPARISON_MIN_MAG_MIP_POINT)
    ENUM_MAP_ITEM(D3D10_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR)
    ENUM_MAP_ITEM(D3D10_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT)
    ENUM_MAP_ITEM(D3D10_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR)
    ENUM_MAP_ITEM(D3D10_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT)
    ENUM_MAP_ITEM(D3D10_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR)
    ENUM_MAP_ITEM(D3D10_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT)
    ENUM_MAP_ITEM(D3D10_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR)
    ENUM_MAP_ITEM(D3D10_FILTER_COMPARISON_ANISOTROPIC)
    ENUM_MAP_ITEM(D3D10_FILTER_TEXT_1BIT)
};

void Logger::log_item(D3D10_FILTER a) {
    log_enum(D3D10_FILTER_ENUM_MAP, a, true);
}

const ENUM_MAP(D3D10_TEXTURE_ADDRESS_MODE) D3D10_TEXTURE_ADDRESS_ENUM_MAP = {
    ENUM_MAP_ITEM(D3D10_TEXTURE_ADDRESS_WRAP)
    ENUM_MAP_ITEM(D3D10_TEXTURE_ADDRESS_MIRROR)
    ENUM_MAP_ITEM(D3D10_TEXTURE_ADDRESS_CLAMP)
    ENUM_MAP_ITEM(D3D10_TEXTURE_ADDRESS_BORDER)
    ENUM_MAP_ITEM(D3D10_TEXTURE_ADDRESS_MIRROR_ONCE)
};

void Logger::log_item(D3D10_TEXTURE_ADDRESS_MODE a) {
    log_enum(D3D10_TEXTURE_ADDRESS_ENUM_MAP, a);
}

const ENUM_MAP(D3D10_COMPARISON_FUNC) D3D10_COMPARISON_ENUM_MAP = {
    ENUM_MAP_ITEM(D3D10_COMPARISON_NEVER)
    ENUM_MAP_ITEM(D3D10_COMPARISON_LESS)
    ENUM_MAP_ITEM(D3D10_COMPARISON_EQUAL)
    ENUM_MAP_ITEM(D3D10_COMPARISON_LESS_EQUAL)
    ENUM_MAP_ITEM(D3D10_COMPARISON_GREATER)
    ENUM_MAP_ITEM(D3D10_COMPARISON_NOT_EQUAL)
    ENUM_MAP_ITEM(D3D10_COMPARISON_GREATER_EQUAL)
    ENUM_MAP_ITEM(D3D10_COMPARISON_ALWAYS)
};

void Logger::log_item(D3D10_COMPARISON_FUNC a) {
    log_enum(D3D10_COMPARISON_ENUM_MAP, a);
}

void Logger::log_item(const D3D10_INPUT_ELEMENT_DESC *input_element_descs) {
    if (!input_element_descs) { log_null(); return; }
    log_struct_named(
#define STRUCT input_element_descs
        LOG_STRUCT_MEMBER_TYPE(SemanticName, StringLogger),
        LOG_STRUCT_MEMBER(SemanticIndex),
        LOG_STRUCT_MEMBER(Format),
        LOG_STRUCT_MEMBER(InputSlot),
        LOG_STRUCT_MEMBER(AlignedByteOffset),
        LOG_STRUCT_MEMBER(InputSlotClass),
        LOG_STRUCT_MEMBER(InstanceDataStepRate)
#undef STRUCT
    );
}

const ENUM_MAP(DXGI_FORMAT) DXGI_FORMAT_ENUM_MAP = {
    ENUM_MAP_ITEM(DXGI_FORMAT_UNKNOWN)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32G32B32A32_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32G32B32A32_FLOAT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32G32B32A32_UINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32G32B32A32_SINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32G32B32_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32G32B32_FLOAT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32G32B32_UINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32G32B32_SINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16G16B16A16_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16G16B16A16_FLOAT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16G16B16A16_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16G16B16A16_UINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16G16B16A16_SNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16G16B16A16_SINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32G32_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32G32_FLOAT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32G32_UINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32G32_SINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32G8X24_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_X32_TYPELESS_G8X24_UINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R10G10B10A2_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_R10G10B10A2_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R10G10B10A2_UINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R11G11B10_FLOAT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8G8B8A8_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8G8B8A8_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8G8B8A8_UINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8G8B8A8_SNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8G8B8A8_SINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16G16_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16G16_FLOAT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16G16_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16G16_UINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16G16_SNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16G16_SINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_D32_FLOAT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32_FLOAT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32_UINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R32_SINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R24G8_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_D24_UNORM_S8_UINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R24_UNORM_X8_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_X24_TYPELESS_G8_UINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8G8_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8G8_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8G8_UINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8G8_SNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8G8_SINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16_FLOAT)
    ENUM_MAP_ITEM(DXGI_FORMAT_D16_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16_UINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16_SNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R16_SINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8_UINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8_SNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8_SINT)
    ENUM_MAP_ITEM(DXGI_FORMAT_A8_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R1_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R9G9B9E5_SHAREDEXP)
    ENUM_MAP_ITEM(DXGI_FORMAT_R8G8_B8G8_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_G8R8_G8B8_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC1_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC1_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC1_UNORM_SRGB)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC2_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC2_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC2_UNORM_SRGB)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC3_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC3_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC3_UNORM_SRGB)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC4_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC4_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC4_SNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC5_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC5_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC5_SNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_B5G6R5_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_B5G5R5A1_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_B8G8R8A8_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_B8G8R8X8_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_B8G8R8A8_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
    ENUM_MAP_ITEM(DXGI_FORMAT_B8G8R8X8_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC6H_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC6H_UF16)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC6H_SF16)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC7_TYPELESS)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC7_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_BC7_UNORM_SRGB)
    ENUM_MAP_ITEM(DXGI_FORMAT_AYUV)
    ENUM_MAP_ITEM(DXGI_FORMAT_Y410)
    ENUM_MAP_ITEM(DXGI_FORMAT_Y416)
    ENUM_MAP_ITEM(DXGI_FORMAT_NV12)
    ENUM_MAP_ITEM(DXGI_FORMAT_P010)
    ENUM_MAP_ITEM(DXGI_FORMAT_P016)
    ENUM_MAP_ITEM(DXGI_FORMAT_420_OPAQUE)
    ENUM_MAP_ITEM(DXGI_FORMAT_YUY2)
    ENUM_MAP_ITEM(DXGI_FORMAT_Y210)
    ENUM_MAP_ITEM(DXGI_FORMAT_Y216)
    ENUM_MAP_ITEM(DXGI_FORMAT_NV11)
    ENUM_MAP_ITEM(DXGI_FORMAT_AI44)
    ENUM_MAP_ITEM(DXGI_FORMAT_IA44)
    ENUM_MAP_ITEM(DXGI_FORMAT_P8)
    ENUM_MAP_ITEM(DXGI_FORMAT_A8P8)
    ENUM_MAP_ITEM(DXGI_FORMAT_B4G4R4A4_UNORM)
    ENUM_MAP_ITEM(DXGI_FORMAT_P208)
    ENUM_MAP_ITEM(DXGI_FORMAT_V208)
    ENUM_MAP_ITEM(DXGI_FORMAT_V408)
    ENUM_MAP_ITEM(DXGI_FORMAT_FORCE_UINT)
};

void Logger::log_item(DXGI_FORMAT a) {
    log_enum(DXGI_FORMAT_ENUM_MAP, a, true);
}

const ENUM_MAP(D3D10_INPUT_CLASSIFICATION) D3D10_INPUT_ENUM_MAP = {
    ENUM_MAP_ITEM(D3D10_INPUT_PER_VERTEX_DATA)
    ENUM_MAP_ITEM(D3D10_INPUT_PER_INSTANCE_DATA)
};

void Logger::log_item(D3D10_INPUT_CLASSIFICATION a) {
    log_enum(D3D10_INPUT_ENUM_MAP, a);
}

void Logger::log_item(const D3D10_BOX *box) {
    if (!box) { log_null(); return; }
    log_struct_named(
#define STRUCT box
        LOG_STRUCT_MEMBER(left),
        LOG_STRUCT_MEMBER(top),
        LOG_STRUCT_MEMBER(front),
        LOG_STRUCT_MEMBER(right),
        LOG_STRUCT_MEMBER(bottom),
        LOG_STRUCT_MEMBER(back)
#undef STRUCT
    );
}

void Logger::log_item(const D3D10_BUFFER_DESC *buffer_desc) {
    log_struct_named(
#define STRUCT buffer_desc
        LOG_STRUCT_MEMBER(ByteWidth),
        LOG_STRUCT_MEMBER(Usage),
        LOG_STRUCT_MEMBER_TYPE(BindFlags, D3D10_BIND_Logger),
        LOG_STRUCT_MEMBER_TYPE(CPUAccessFlags, D3D10_CPU_ACCESS_Logger),
        LOG_STRUCT_MEMBER_TYPE(MiscFlags, D3D10_RESOURCE_MISC_Logger)
#undef STRUCT
    );
}

const ENUM_MAP(D3D10_USAGE) D3D10_USAGE_ENUM_MAP = {
    ENUM_MAP_ITEM(D3D10_USAGE_DEFAULT)
    ENUM_MAP_ITEM(D3D10_USAGE_IMMUTABLE)
    ENUM_MAP_ITEM(D3D10_USAGE_DYNAMIC)
    ENUM_MAP_ITEM(D3D10_USAGE_STAGING)
};

void Logger::log_item(const D3D10_TEXTURE1D_DESC *desc) {
    log_struct_named(
#define STRUCT desc
        LOG_STRUCT_MEMBER(Width),
        LOG_STRUCT_MEMBER(MipLevels),
        LOG_STRUCT_MEMBER(ArraySize),
        LOG_STRUCT_MEMBER(Format),
        LOG_STRUCT_MEMBER(Usage),
        LOG_STRUCT_MEMBER_TYPE(BindFlags, D3D10_BIND_Logger),
        LOG_STRUCT_MEMBER_TYPE(CPUAccessFlags, D3D10_CPU_ACCESS_Logger),
        LOG_STRUCT_MEMBER_TYPE(MiscFlags, D3D10_RESOURCE_MISC_Logger)
#undef STRUCT
    );
}
void Logger::log_item(const D3D10_TEXTURE2D_DESC *desc) {
    log_struct_named(
#define STRUCT desc
        LOG_STRUCT_MEMBER(Width),
        LOG_STRUCT_MEMBER(Height),
        LOG_STRUCT_MEMBER(MipLevels),
        LOG_STRUCT_MEMBER(ArraySize),
        LOG_STRUCT_MEMBER(Format),
        LOG_STRUCT_MEMBER(SampleDesc),
        LOG_STRUCT_MEMBER(Usage),
        LOG_STRUCT_MEMBER_TYPE(BindFlags, D3D10_BIND_Logger),
        LOG_STRUCT_MEMBER_TYPE(CPUAccessFlags, D3D10_CPU_ACCESS_Logger),
        LOG_STRUCT_MEMBER_TYPE(MiscFlags, D3D10_RESOURCE_MISC_Logger)
#undef STRUCT
    );
}

void Logger::log_item(const DXGI_SAMPLE_DESC a) {
    log_item(&a);
}

void Logger::log_item(const DXGI_SAMPLE_DESC *a) {
    log_struct_named(
#define STRUCT a
        LOG_STRUCT_MEMBER(Count),
        LOG_STRUCT_MEMBER(Quality)
#undef STRUCT
    );
}

void Logger::log_item(const D3D10_SHADER_RESOURCE_VIEW_DESC *a) {
    log_struct_named();
}

void Logger::log_item(const D3D10_RENDER_TARGET_VIEW_DESC *a) {
    log_struct_named();
}

void Logger::log_item(const D3D10_DEPTH_STENCIL_VIEW_DESC *a) {
    log_struct_named();
}

const ENUM_MAP(D3D10_PRIMITIVE_TOPOLOGY) D3D10_PRIMITIVE_TOPOLOGY_ENUM_MAP = {
    ENUM_MAP_ITEM(D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED)
    ENUM_MAP_ITEM(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST)
    ENUM_MAP_ITEM(D3D10_PRIMITIVE_TOPOLOGY_LINELIST)
    ENUM_MAP_ITEM(D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP)
    ENUM_MAP_ITEM(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
    ENUM_MAP_ITEM(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP)
    ENUM_MAP_ITEM(D3D10_PRIMITIVE_TOPOLOGY_LINELIST_ADJ)
    ENUM_MAP_ITEM(D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ)
    ENUM_MAP_ITEM(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ)
    ENUM_MAP_ITEM(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ)
};

void Logger::log_item(D3D10_PRIMITIVE_TOPOLOGY a) {
    log_enum(D3D10_PRIMITIVE_TOPOLOGY_ENUM_MAP, a);
}

void Logger::log_item(const D3D10_VIEWPORT *a) {
    log_struct_named(
#define STRUCT a
        LOG_STRUCT_MEMBER(TopLeftX),
        LOG_STRUCT_MEMBER(TopLeftY),
        LOG_STRUCT_MEMBER(Width),
        LOG_STRUCT_MEMBER(Height),
        LOG_STRUCT_MEMBER(MinDepth),
        LOG_STRUCT_MEMBER(MaxDepth)
#undef STRUCT
    );
}

void Logger::log_item(const D3D10_TEXTURE3D_DESC *desc) {
    log_struct_named(
#define STRUCT desc
        LOG_STRUCT_MEMBER(Width),
        LOG_STRUCT_MEMBER(Height),
        LOG_STRUCT_MEMBER(Depth),
        LOG_STRUCT_MEMBER(MipLevels),
        LOG_STRUCT_MEMBER(Format),
        LOG_STRUCT_MEMBER(Usage),
        LOG_STRUCT_MEMBER_TYPE(BindFlags, D3D10_BIND_Logger),
        LOG_STRUCT_MEMBER_TYPE(CPUAccessFlags, D3D10_CPU_ACCESS_Logger),
        LOG_STRUCT_MEMBER_TYPE(MiscFlags, D3D10_RESOURCE_MISC_Logger)
#undef STRUCT
    );
}

void Logger::log_item(D3D10_USAGE a) {
    log_enum(D3D10_USAGE_ENUM_MAP, a);
}

void Logger::log_items_base() {
}

void Logger::log_struct_members() {
}

void Logger::log_struct_members_named() {
}

Logger::Logger(LPCTSTR file_name) :
    file_name(file_name),
    started(false),
    start_count(0),
    frame_count(0),
    file(INVALID_HANDLE_VALUE),
    oss{},
    oss_cs{}
{
    InitializeCriticalSection(&oss_cs);
    Config *config = default_config;
    if (config && config->logging) {
        started = true;
        Overlay::push_text("Logging to ", file_name, " enabled");
    }
}

Logger::~Logger() {
    DeleteCriticalSection(&oss_cs);
}

LPCTSTR Logger::get_file_name() {
    return file_name;
}

bool Logger::get_started() {
    return started;
}

bool Logger::start() {
    if (!file_init()) return false;
    ++start_count;
    started = true;
    return true;
}

void Logger::stop() {
    started = false;
}

void Logger::next_frame() {
    ++frame_count;
}

void Logger::log_fun_custom_args() {}

void Logger::log_fun_custom_end() {
    log_fun_end();
    log_end();
}

Logger *default_logger;
