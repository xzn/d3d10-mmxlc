#ifndef INI_H
#define INI_H

#include "main.h"

class Ini {
    struct ThreadState {
        LPCTSTR file_name;
        HANDLE file;
        UINT64 time;
        bool thread_running;
    };
    ThreadState *thread_state;
    friend DWORD WINAPI ini_ThreadProc(LPVOID lpParameter);

public:
    Ini(LPCTSTR file_name);
    ~Ini();
};
extern Ini *default_ini;

#endif
