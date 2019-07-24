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
            InitializeCriticalSection(&Overlay::texts_cs);
            Overlay::push_text(MOD_NAME " loaded");
            default_overlays = new std::unordered_set<Overlay *>{};
            default_config = new Config{};
            default_ini = new Ini(INI_FILE_NAME);
            default_logger = new Logger(LOG_FILE_NAME);

            base_dll_init(hinstDLL);
            minhook_init();
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
            minhook_shutdown();
            base_dll_shutdown();

            delete default_logger;
            delete default_ini;
            delete default_config;
            delete default_overlays;
            DeleteCriticalSection(&Overlay::texts_cs);
            my_config_free();

            break;
        }
    }
    return TRUE; // Successful DLL_PROCESS_ATTACH.
}
