#ifndef D3D10VERTEXSHADER_H
#define D3D10VERTEXSHADER_H

#include "main.h"
#include "d3d10devicechild.h"

class MyID3D10VertexShader : public ID3D10VertexShader {
    class Impl;
    Impl *impl;

public:
    ID3D10DEVICECHILD_DECL(ID3D10VertexShader)

    DWORD get_bytecode_hash() const;
    SIZE_T get_bytecode_length() const;
    const std::string &get_source() const;
    const std::vector<D3D10_SO_DECLARATION_ENTRY> &get_decl_entries() const;
    ID3D10GeometryShader *get_sogs() const;

    MyID3D10VertexShader(
        ID3D10VertexShader **inner,
        DWORD bytecode_hash,
        SIZE_T bytecode_length,
        const std::string &source,
        std::vector<D3D10_SO_DECLARATION_ENTRY> &&decl_entries,
        ID3D10GeometryShader *pGeometryShader
    );

    virtual ~MyID3D10VertexShader();
};

extern std::unordered_map<ID3D10VertexShader *, MyID3D10VertexShader *> cached_vss_map;

#endif
