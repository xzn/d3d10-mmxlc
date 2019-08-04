#include "d3d10vertexshader.h"
#include "d3d10devicechild_impl.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10VertexShader, ## __VA_ARGS__)

class MyID3D10VertexShader::Impl {
    friend class MyID3D10VertexShader;

    IUNKNOWN_PRIV(ID3D10VertexShader)
    DWORD bytecode_hash = 0;
    SIZE_T bytecode_length = 0;
    std::string source;
    std::vector<D3D10_SO_DECLARATION_ENTRY> decl_entries;
    ID3D10GeometryShader *sogs = NULL;

    Impl(
        ID3D10VertexShader **inner,
        DWORD bytecode_hash,
        SIZE_T bytecode_length,
        const std::string &source,
        std::vector<D3D10_SO_DECLARATION_ENTRY> &&decl_entries,
        ID3D10GeometryShader *pGeometryShader
    ) :
        IUNKNOWN_INIT(*inner),
        bytecode_hash(bytecode_hash),
        bytecode_length(bytecode_length),
        source(source),
        decl_entries(std::move(decl_entries)),
        sogs(pGeometryShader)
    {}

    ~Impl() {
        if (sogs) sogs->Release();
    }
};

ID3D10DEVICECHILD_IMPL(MyID3D10VertexShader, ID3D10VertexShader)

DWORD MyID3D10VertexShader::get_bytecode_hash() const { return impl->bytecode_hash; }
SIZE_T MyID3D10VertexShader::get_bytecode_length() const { return impl->bytecode_length; }
const std::string &MyID3D10VertexShader::get_source() const { return impl->source; }
const std::vector<D3D10_SO_DECLARATION_ENTRY> &
MyID3D10VertexShader::get_decl_entries() const { return impl->decl_entries; }
ID3D10GeometryShader *MyID3D10VertexShader::get_sogs() const { return impl->sogs; }

MyID3D10VertexShader::MyID3D10VertexShader(
    ID3D10VertexShader **inner,
    DWORD bytecode_hash,
    SIZE_T bytecode_length,
    const std::string &source,
    std::vector<D3D10_SO_DECLARATION_ENTRY> &&decl_entries,
    ID3D10GeometryShader *pGeometryShader
) :
    impl(new Impl(
        inner,
        bytecode_hash,
        bytecode_length,
        source,
        std::move(decl_entries),
        pGeometryShader
    ))
{
    LOG_MFUN(_,
        LOG_ARG(*inner),
        LOG_ARG_TYPE(bytecode_hash, NumHexLogger),
        LOG_ARG(bytecode_length)
    );
    cached_vss_map.emplace(*inner, this);
    *inner = this;
}

MyID3D10VertexShader::~MyID3D10VertexShader() {
    LOG_MFUN();
    cached_vss_map.erase(impl->inner);
    delete impl;
}

std::unordered_map<ID3D10VertexShader *, MyID3D10VertexShader *> cached_vss_map;
