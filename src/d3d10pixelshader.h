#ifndef D3D10PIXELSHADER_H
#define D3D10PIXELSHADER_H

#include "main.h"
#include "d3d10devicechild.h"

enum class PIXEL_SHADER_ALPHA_DISCARD {
    UNKNOWN,
    NONE,
    EQUAL,
    LESS,
    LESS_OR_EQUAL
};

class MyID3D10PixelShader : public ID3D10PixelShader {
    class Impl;
    Impl *impl;

public:
    ID3D10DEVICECHILD_DECL(ID3D10PixelShader)

    DWORD get_bytecode_hash() const;
    SIZE_T get_bytecode_length() const;
    const std::string &get_source() const;
    PIXEL_SHADER_ALPHA_DISCARD get_alpha_discard() const;
    const std::vector<UINT> &get_texcoord_sampler_map() const;
    const std::vector<std::tuple<std::string, std::string>> &get_uniform_list() const;

    MyID3D10PixelShader(
        ID3D10PixelShader **inner,
        DWORD bytecode_hash,
        SIZE_T bytecode_length,
        const std::string &source
    );

    virtual ~MyID3D10PixelShader();
};

extern std::unordered_map<ID3D10PixelShader *, MyID3D10PixelShader *> cached_pss_map;

#endif
