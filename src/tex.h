#ifndef TEX_H
#define TEX_H

#include "main.h"

struct TextureAndViews {
    ID3D10Texture2D *tex = NULL;
    ID3D10ShaderResourceView *srv = NULL;
    ID3D10RenderTargetView *rtv = NULL;
    UINT width = 0;
    UINT height = 0;
    TextureAndViews();
    ~TextureAndViews();
};

struct TextureAndDepthViews : TextureAndViews {
    ID3D10Texture2D *tex_ds = NULL;
    ID3D10DepthStencilView *dsv = NULL;
    TextureAndDepthViews();
    ~TextureAndDepthViews();
};

struct TextureViewsAndBuffer : TextureAndViews {
    ID3D10Buffer *ps_cb = NULL;
    TextureViewsAndBuffer();
    ~TextureViewsAndBuffer();
};

#endif
