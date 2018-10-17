#ifndef CONF_H
#define CONF_H

#include "main.h"

struct Config {
    bool logging = false;
    std::vector<BYTE> log_toggle_hotkey;
    std::vector<BYTE> log_frame_hotkey;
    bool interp = false;
    bool enhanced = false;
    std::string slang_shader;
    bool slang_shader_updated = false;
    std::string slang_shader_3d;
    bool slang_shader_3d_updated = false;
    UINT render_3d_width = 0;
    UINT render_3d_height = 0;
    UINT display_width = 0;
    UINT display_height = 0;
    bool render_display_updated = false;

    CRITICAL_SECTION cs;
    Config();
    ~Config();
};
extern Config *default_config;

#endif
