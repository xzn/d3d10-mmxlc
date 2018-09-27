#ifndef DINPUT8_DLL_H
#define DINPUT8_DLL_H

#include "main.h"

void base_dll_init(HINSTANCE hinstDLL);
void base_dll_shutdown();
void minhook_init();
void minhook_shutdown();

#endif
