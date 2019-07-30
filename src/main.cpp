#include "main.h"

bool _tstring_view_icmp::operator()(const _tstring_view &a, const _tstring_view &b) const {
    int ret = _tcsnicmp(a.data(), b.data(), std::min(a.size(), b.size()));
    if (ret == 0) return a.size() < b.size();
    return ret < 0;
}

#include "dinput8_dll.h"
#include "overlay.h"
#include "conf.h"
#include "ini.h"
#include "log.h"
#include "../RetroArch/retroarch.h"

static Logger dummy_logger = {NULL};
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL, // handle to DLL module
    DWORD fdwReason,    // reason for calling function
    LPVOID lpReserved   // reserved
) {
    // Perform actions based on the reason for calling.
    switch( fdwReason )
    {
        case DLL_PROCESS_ATTACH:
        // Initialize once for each new process.
        // Return FALSE to fail DLL load.
        {
            DisableThreadLibraryCalls(hinstDLL);
            my_config_init();

            default_overlay = new Overlay();
            default_overlay(MOD_NAME " loaded");

            default_config = new Config();

            default_ini = new Ini(INI_FILE_NAME);
            default_ini->set_overlay(default_overlay);
            default_ini->set_config(default_config);

            default_logger = new Logger(LOG_FILE_NAME);
            default_logger->set_overlay(default_overlay);
            default_logger->set_config(default_config);

            base_dll_init(hinstDLL);
            break;
        }

        case DLL_THREAD_ATTACH:
        // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
        // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
        // Perform any necessary cleanup.
        {
            // May need to add synchronizations
            // in this block.
            base_dll_shutdown();

            delete default_logger;
            default_logger = &dummy_logger;
            delete default_ini;
            delete default_config;
            delete default_overlay;

            my_config_free();
            break;
        }
    }
    return TRUE; // Successful DLL_PROCESS_ATTACH.
}

class cs_wrapper::Impl {
    CRITICAL_SECTION cs;
    friend class cs_wrapper;
};

cs_wrapper::cs_wrapper() : impl(new Impl()) {
    InitializeCriticalSection(&impl->cs);
}
cs_wrapper::~cs_wrapper() {
    DeleteCriticalSection(&impl->cs);
    delete impl;
}

void cs_wrapper::begin_cs() {
    EnterCriticalSection(&impl->cs);
}

bool cs_wrapper::try_begin_cs() {
    return TryEnterCriticalSection(&impl->cs);
}

void cs_wrapper::end_cs() {
    LeaveCriticalSection(&impl->cs);
}
