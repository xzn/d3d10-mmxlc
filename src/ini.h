#ifndef INI_H
#define INI_H

#include "main.h"

class Config;
class Overlay;
class Ini {
    class Impl;
    Impl *impl;

public:
    Ini(LPCTSTR file_name);
    ~Ini();

    void set_config(Config *config);
    void set_overlay(Overlay *overlay);
};
extern Ini *default_ini;

#endif
