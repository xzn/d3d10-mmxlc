#include "d3d10inputlayout.h"
#include "d3d10devicechild_impl.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyID3D10InputLayout, ## __VA_ARGS__)

class MyID3D10InputLayout::Impl {
    friend class MyID3D10InputLayout;

    IUNKNOWN_PRIV(ID3D10InputLayout)
    D3D10_INPUT_ELEMENT_DESC *descs = NULL;
    UINT descs_num = 0;

    Impl(
        ID3D10InputLayout **inner,
        const D3D10_INPUT_ELEMENT_DESC *pInputElementDescs,
        UINT NumElements
    ) :
        IUNKNOWN_INIT(*inner),
        descs(new D3D10_INPUT_ELEMENT_DESC[NumElements]),
        descs_num(NumElements)
    {
        std::copy_n(pInputElementDescs, NumElements, descs);
    }

    ~Impl() { if (descs) delete[] descs; }
};

ID3D10DEVICECHILD_IMPL(MyID3D10InputLayout, ID3D10InputLayout)

UINT &MyID3D10InputLayout::get_descs_num() {
    return impl->descs_num;
}

UINT MyID3D10InputLayout::get_descs_num() const {
    return impl->descs_num;
}

D3D10_INPUT_ELEMENT_DESC *&MyID3D10InputLayout::get_descs() {
    return impl->descs;
}

D3D10_INPUT_ELEMENT_DESC *MyID3D10InputLayout::get_descs() const {
    return impl->descs;
}

MyID3D10InputLayout::MyID3D10InputLayout(
    ID3D10InputLayout **inner,
    const D3D10_INPUT_ELEMENT_DESC *pInputElementDescs,
    UINT NumElements
) :
    impl(new Impl(inner, pInputElementDescs, NumElements))
{
    LOG_MFUN(_,
        LOG_ARG(*inner)
    );
    cached_ils_map.emplace(*inner, this);
    *inner = this;
}

MyID3D10InputLayout::~MyID3D10InputLayout() {
    LOG_MFUN();
    cached_ils_map.erase(impl->inner);
    delete impl;
}

std::unordered_map<ID3D10InputLayout *, MyID3D10InputLayout *> cached_ils_map;
