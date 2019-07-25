#ifndef D3D10PIXELSHADER_H
#define D3D10PIXELSHADER_H

#include "main.h"
#include "unknown.h"
#include "d3d10devicechild.h"

class MyID3D10PixelShader : public ID3D10PixelShader {
public:
    const DWORD bytecode_hash;
    const SIZE_T bytecode_length;
    const std::string source;
    const bool alpha_discard;

    MyID3D10PixelShader(
        ID3D10PixelShader **inner,
        DWORD bytecode_hash,
        SIZE_T bytecode_length,
        std::string source
    );

    virtual ~MyID3D10PixelShader();

    IUNKNOWN_DECL(MyID3D10PixelShader, ID3D10PixelShader)

    ID3D10DEVICECHILD_DECL
};

extern std::unordered_map<ID3D10PixelShader *, MyID3D10PixelShader *> cached_pss_map;

#endif
