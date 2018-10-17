#ifndef TEX_H
#define TEX_H

#include "main.h"

struct TextureAndViews {
    ID3D10Texture2D *tex;
    ID3D10ShaderResourceView *srv;
    ID3D10RenderTargetView *rtv;
    UINT width;
    UINT height;
    TextureAndViews();
    ~TextureAndViews();
};

struct TextureViewsAndBuffer : TextureAndViews {
    ID3D10Buffer *ps_cb;
    TextureViewsAndBuffer();
    ~TextureViewsAndBuffer();
};

#endif
