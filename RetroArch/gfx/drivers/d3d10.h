#ifndef RETROARCH_D3D10_H
#define RETROARCH_D3D10_H

#include "../common/d3d10_common.h"

#ifdef __cplusplus
extern "C" {
#endif

d3d10_video_t *my_d3d10_gfx_init(ID3D10Device *device);
void my_d3d10_gfx_free(d3d10_video_t *d3d10);
bool my_d3d10_gfx_set_shader(d3d10_video_t *d3d10, const char* path);
bool my_d3d10_gfx_frame(d3d10_video_t *d3d10, d3d10_texture_t *texture, UINT64 frame_count);
void my_d3d10_update_viewport(d3d10_video_t *d3d10, D3D10RenderTargetView renderTargetView, video_viewport_t *viewport);

#ifdef __cplusplus
}
#endif

#endif
