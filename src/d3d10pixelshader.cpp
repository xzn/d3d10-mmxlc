#include "d3d10pixelshader.h"
#include "d3d10devicechild_impl.h"
#include "log.h"

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
        )
    {}

    ~Impl() {}
};

ID3D10DEVICECHILD_IMPL(MyID3D10PixelShader, ID3D10PixelShader)

DWORD MyID3D10PixelShader::get_bytecode_hash() { return impl->bytecode_hash; }
SIZE_T MyID3D10PixelShader::get_bytecode_length() { return impl->bytecode_length; }
const std::string &MyID3D10PixelShader::get_source() { return impl->source; }
PIXEL_SHADER_ALPHA_DISCARD MyID3D10PixelShader::get_alpha_discard() { return impl->alpha_discard; }

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
    *inner = this;
}

MyID3D10PixelShader::~MyID3D10PixelShader() {
    LOG_MFUN();
    cached_pss_map.erase(impl->inner);
    delete impl;
}

std::unordered_map<ID3D10PixelShader *, MyID3D10PixelShader *> cached_pss_map;
