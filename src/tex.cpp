#include "tex.h"

TextureAndViews::TextureAndViews() :
    tex(NULL),
    srv(NULL),
    rtv(NULL),
    width(0),
    height(0)
{}

TextureAndViews::~TextureAndViews() {
    if (tex) tex->Release();
    if (srv) tex->Release();
    if (rtv) tex->Release();
}

TextureViewsAndBuffer::TextureViewsAndBuffer() :
    ps_cb(NULL)
{}

TextureViewsAndBuffer::~TextureViewsAndBuffer() {
    if (ps_cb) ps_cb->Release();
}
