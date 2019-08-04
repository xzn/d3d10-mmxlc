#ifndef D3D10INPUTLAYOUT_H
#define D3D10INPUTLAYOUT_H

#include "main.h"
#include "d3d10devicechild.h"

class MyID3D10InputLayout : public ID3D10InputLayout {
    class Impl;
    Impl *impl;

public:
    ID3D10DEVICECHILD_DECL(ID3D10InputLayout);

    MyID3D10InputLayout(
        ID3D10InputLayout **inner,
        const D3D10_INPUT_ELEMENT_DESC *pInputElementDescs,
        UINT NumElements
    );

    virtual ~MyID3D10InputLayout();
    UINT &get_descs_num();
    UINT get_descs_num() const;
    D3D10_INPUT_ELEMENT_DESC *&get_descs();
    D3D10_INPUT_ELEMENT_DESC *get_descs() const;
};

extern std::unordered_map<ID3D10InputLayout *, MyID3D10InputLayout *> cached_ils_map;

#endif
