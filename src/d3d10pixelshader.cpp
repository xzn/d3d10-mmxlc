#include "d3d10pixelshader.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10PixelShader, ## __VA_ARGS__)

IUNKNOWN_IMPL(MyID3D10PixelShader)

MyID3D10PixelShader::MyID3D10PixelShader(
    ID3D10PixelShader **inner,
    DWORD bytecode_hash,
    SIZE_T bytecode_length
) :
    bytecode_hash(bytecode_hash),
    bytecode_length(bytecode_length),
    IUNKNOWN_INIT(*inner)
{
    LOG_MFUN(_,
        LOG_ARG(*inner),
        LOG_ARG_TYPE(bytecode_hash, NumHexLogger),
        LOG_ARG(bytecode_length)
    );
    current_pss_map.emplace(*inner, this);
    *inner = this;
}

MyID3D10PixelShader::~MyID3D10PixelShader() {
    LOG_MFUN();
    current_pss_map.erase(inner);
}

ID3D10DEVICECHILD_IMPL(MyID3D10PixelShader)

std::unordered_map<ID3D10PixelShader *, MyID3D10PixelShader *> current_pss_map;
