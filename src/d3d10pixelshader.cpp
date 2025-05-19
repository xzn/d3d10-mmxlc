#include "d3d10pixelshader.h"
#include "d3d10devicechild_impl.h"
#include "log.h"

#include <shlwapi.h>

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10PixelShader, ## __VA_ARGS__)

class MyID3D10PixelShader::Impl {
    friend class MyID3D10PixelShader;

    IUNKNOWN_PRIV(ID3D10PixelShader)
    DWORD bytecode_hash = 0;
    SIZE_T bytecode_length = 0;
    std::string source;
    PIXEL_SHADER_ALPHA_DISCARD alpha_discard =
        PIXEL_SHADER_ALPHA_DISCARD::UNKNOWN;
    std::vector<UINT> texcoord_sampler_map;
    std::vector<std::tuple<std::string, std::string>> uniform_list;
    bool tex_has_lut = false;
    ID3D10PixelShader *orig = NULL;
    ID3D10PixelShader *repl = NULL;
    std::string repl_file_name;
    DWORD repl_file_time = 0;
    DWORD repl_file_size = 0;

    Impl(
        ID3D10PixelShader **inner,
        DWORD bytecode_hash,
        SIZE_T bytecode_length,
        const std::string &source
    ) :
        IUNKNOWN_INIT(*inner),
        bytecode_hash(bytecode_hash),
        bytecode_length(bytecode_length),
        source(source),
        alpha_discard(
            source.find("discard") == std::string::npos ||
            source.find("fAlphaRef") == std::string::npos ?
                PIXEL_SHADER_ALPHA_DISCARD::NONE :
            source.find("==CBROPTestPS.fAlphaRef") != std::string::npos ?
                PIXEL_SHADER_ALPHA_DISCARD::EQUAL :
            source.find("<CBROPTestPS.fAlphaRef") != std::string::npos ?
                PIXEL_SHADER_ALPHA_DISCARD::LESS :
            source.find("CBROPTestPS.fAlphaRef>=") != std::string::npos ?
                PIXEL_SHADER_ALPHA_DISCARD::LESS_OR_EQUAL :
            PIXEL_SHADER_ALPHA_DISCARD::NONE
        ),
        tex_has_lut(
            source.find("tClutMap") != std::string::npos
        )
    {
if constexpr (ENABLE_LOGGER) {
        std::unordered_map<std::string, UINT> tex_is;
        {
            std::string source_next = source;
            std::regex uniform_regex(R"(uniform\s+(\w+)\s+(\w+);)");
            std::smatch uniform_sm;
            UINT i = 0;
            while (std::regex_search(
                source_next,
                uniform_sm,
                uniform_regex
            )) {
                uniform_list.emplace_back(uniform_sm[1], uniform_sm[2]);

                std::regex sampler_regex(R"(\w*sampler\w+)");
                std::smatch sampler_sm;
                std::string type = uniform_sm[1];
                if (std::regex_search(
                    type,
                    sampler_sm,
                    sampler_regex
                )) {
                    tex_is.emplace(
                        uniform_sm[2],
                        sampler_sm[0] == "sampler2D" ? i : -1
                    );
                    ++i;
                }

                source_next = uniform_sm.suffix();
            }
        }
        for (UINT i = 0;; ++i) {
            std::string texcoord_var_name = "vs_TEXCOORD" + std::to_string(i);
            {
                std::regex texcoord_regex(R"(in\s+vec4\s+)" + texcoord_var_name + ";");
                std::smatch texcoord_sm;
                if (!std::regex_search(
                    source,
                    texcoord_sm,
                    texcoord_regex
                )) break;
            }
            {
                std::regex texture_regex(R"(texture\((\w+),\s*)" + texcoord_var_name);
                std::smatch texture_sm;
                if (std::regex_search(
                    source,
                    texture_sm,
                    texture_regex
                )) {
                    auto it = tex_is.find(texture_sm[1]);
                    texcoord_sampler_map.push_back(it != tex_is.end() ? it->second : -1);
                } else {
                    texcoord_sampler_map.push_back(-1);
                }
            }
        }
}
        std::ostringstream oss;
        oss << "0x" << std::setw(8) << std::setfill('0') << std::hex << bytecode_hash << ".psh";
        repl_file_name = oss.str();
    }

    ~Impl() {}

    bool clear_repl() {
        if (repl) {
            cached_pss_map.erase(repl);
            repl->Release();
            repl = NULL;
            repl_file_size = 0;
            repl_file_time = 0;

            inner = orig;
            return true;
        }
        return false;
    }
};

ID3D10DEVICECHILD_IMPL(MyID3D10PixelShader, ID3D10PixelShader)

DWORD MyID3D10PixelShader::get_bytecode_hash() const { return impl->bytecode_hash; }
SIZE_T MyID3D10PixelShader::get_bytecode_length() const { return impl->bytecode_length; }
const std::string &MyID3D10PixelShader::get_source() const { return impl->source; }
PIXEL_SHADER_ALPHA_DISCARD MyID3D10PixelShader::get_alpha_discard() const { return impl->alpha_discard; }
bool MyID3D10PixelShader::get_tex_has_lut() const { return impl->tex_has_lut; }
const std::vector<UINT> &
MyID3D10PixelShader::get_texcoord_sampler_map() const { return impl->texcoord_sampler_map; }
const std::vector<std::tuple<std::string, std::string>> &
MyID3D10PixelShader::get_uniform_list() const { return impl->uniform_list; }

MyID3D10PixelShader::MyID3D10PixelShader(
    ID3D10PixelShader **inner,
    DWORD bytecode_hash,
    SIZE_T bytecode_length,
    const std::string &source
) :
    impl(new Impl(inner, bytecode_hash, bytecode_length, source))
{
    LOG_MFUN(_,
        LOG_ARG(*inner),
        LOG_ARG_TYPE(bytecode_hash, NumHexLogger),
        LOG_ARG(bytecode_length)
    );
    cached_pss_map.emplace(*inner, this);
    impl->orig = *inner;
    *inner = this;
}

MyID3D10PixelShader::~MyID3D10PixelShader() {
    LOG_MFUN();
    cached_pss_map.erase(impl->inner);
    delete impl;
}

ID3D10PixelShader *MyID3D10PixelShader::get_repl(ID3D10Device *dev, OverlayPtr ov) {
    if (PathFileExistsA(impl->repl_file_name.c_str())) {
        HANDLE file = CreateFileA(
            impl->repl_file_name.c_str(),
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );
        if (file != INVALID_HANDLE_VALUE) {
            DWORD size = GetFileSize(file, NULL);
            FILETIME write_time = {};
            BOOL ret = GetFileTime(file, NULL, NULL, &write_time);
            if (!size || !ret) {
                CloseHandle(file);
                goto reset_shader;
            }

            if (size != impl->repl_file_size || write_time.dwLowDateTime != impl->repl_file_time) {
                // close old repl shader
                impl->clear_repl();
                impl->repl_file_size = size;
                impl->repl_file_time = write_time.dwLowDateTime;
                // open new repl shader
                std::string source;
                source.resize(size);
                ReadFile(file, source.data(), size, &size, NULL);
                CloseHandle(file);
                // compile new repl shader
                ID3D10Blob *shader_blob = NULL;
                ID3D10Blob *err_msg = NULL;
                HRESULT res = D3D10CompileShader(
                    source.c_str(),
                    size,
                    NULL,
                    NULL,
                    NULL,
                    "main",
                    "ps_4_0",
                    D3D10_SHADER_OPTIMIZATION_LEVEL3 | D3D10_SHADER_WARNINGS_ARE_ERRORS,
                    &shader_blob,
                    &err_msg
                );
                if (err_msg) {
                    std::string err_str;
                    SIZE_T err_size = err_msg->GetBufferSize();
                    err_str.resize(err_size + 1);
                    memcpy(err_str.data(), err_msg->GetBufferPointer(), err_size);
                    err_str[err_size] = 0;
                    err_msg->Release();
                    err_msg = NULL;
                    ov(impl->repl_file_name, " compile: ", err_str);
                }
                if (res != S_OK) {
                    std::ostringstream oss;
                    oss << "0x" << std::setw(8) << std::setfill('0') << std::hex << res;
                    ov(impl->repl_file_name, " compile failed: ", oss.str());
                    goto reset_shader;
                }
                // create new repl shader
                ID3D10PixelShader *shader;
                res = dev->CreatePixelShader(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), &shader);
                shader_blob->Release();
                shader_blob = NULL;
                if (res != S_OK) {
                    std::ostringstream oss;
                    oss << "0x" << std::setw(8) << std::setfill('0') << std::hex << res;
                    ov(impl->repl_file_name, " create failed: ", oss.str());
                    goto reset_shader;
                }
                impl->inner = impl->repl = shader;
                cached_pss_map.emplace(impl->repl, this);
                ov(impl->repl_file_name, " replaced.");
            } else {
                // keep old repl shader
                CloseHandle(file);
            }
        } else {
            goto reset_shader;
        }
    } else {
reset_shader:
        // reset shader
        if (impl->clear_repl()) {
            ov(impl->repl_file_name, " restored.");
        }
    }
    return get_inner();
}

bool MyID3D10PixelShader::get_has_repl() const {
    return impl->inner == impl->repl;
}

std::unordered_map<ID3D10PixelShader *, MyID3D10PixelShader *> cached_pss_map;
