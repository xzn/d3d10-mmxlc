#include "dinput8_dll.h"
#include "dxgiswapchain.h"
#include "directinput8a.h"
#include "overlay.h"
#include "conf.h"
#include "log.h"
#include "../minhook/include/MinHook.h"

#define LOGGER default_logger

#define DEFINE_PROC(r, n, v) \
    typedef r (__stdcall *n ## _t) v; \
    n ## _t p ## n; \
    extern "C" r __stdcall n v

// dinput8.dll

DEFINE_PROC(HRESULT, DirectInput8Create, (
    HINSTANCE hinst,
    DWORD dwVersion,
    REFIID riidltf,
    LPVOID *ppvOut,
    LPUNKNOWN punkOuter
)) {
    HRESULT ret = pDirectInput8Create ?
        pDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter) :
        E_NOTIMPL;
    LOG_FUN(_,
        LOG_ARG(hinst),
        LOG_ARG(dwVersion),
        LOG_ARG(riidltf),
        ret
    );

    if (ret == S_OK) {
        new MyIDirectInput8A(
            (IDirectInput8A **)ppvOut
        );
    }
    return ret;
}

DEFINE_PROC(HRESULT, DllCanUnloadNow, ()) {
    LOG_FUN();
    if (pDllCanUnloadNow) {
        return pDllCanUnloadNow();
    } else {
        return E_NOTIMPL;
    }
}

DEFINE_PROC(HRESULT, DllGetClassObject, (
    REFCLSID rclsid,
    REFIID   riid,
    LPVOID   *ppv
)) {
    LOG_FUN();
    if (pDllGetClassObject) {
        return pDllGetClassObject(rclsid, riid, ppv);
    } else {
        return E_NOTIMPL;
    }
}

DEFINE_PROC(HRESULT, DllRegisterServer, ()) {
    LOG_FUN();
    if (pDllRegisterServer) {
        return pDllRegisterServer();
    } else {
        return E_NOTIMPL;
    }
}

DEFINE_PROC(HRESULT, DllUnregisterServer, ()) {
    LOG_FUN();
    if (pDllUnregisterServer) {
        return pDllUnregisterServer();
    } else {
        return E_NOTIMPL;
    }
}

DEFINE_PROC(LPCDIDATAFORMAT, GetdfDIJoystick, ()) {
    LOG_FUN();
    if (pGetdfDIJoystick) {
        return pGetdfDIJoystick();
    } else {
        return NULL;
    }
}

// d3d10.dll

DEFINE_PROC(HRESULT, D3D10CreateDeviceAndSwapChain, (
    IDXGIAdapter         *pAdapter,
    D3D10_DRIVER_TYPE    DriverType,
    HMODULE              Software,
    UINT                 Flags,
    UINT                 SDKVersion,
    DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
    IDXGISwapChain       **ppSwapChain,
    ID3D10Device         **ppDevice
)) {
    HRESULT ret = E_NOTIMPL;
    if (pD3D10CreateDeviceAndSwapChain) {
        ret = pD3D10CreateDeviceAndSwapChain(
            pAdapter,
            DriverType,
            Software,
            Flags,
            SDKVersion,
            pSwapChainDesc,
            ppSwapChain,
            ppDevice
        );
        if (ret == S_OK) {
            auto sc = new MyIDXGISwapChain(
                pSwapChainDesc,
                ppSwapChain,
                ppDevice
            );
            default_config->hwnd = pSwapChainDesc->OutputWindow;
            sc->set_overlay(default_overlay);
            sc->set_config(default_config);
        }
    }
    LOG_FUN(_, ret);
    return ret;
}

namespace {

HMODULE base_dll;

bool MinHook_Initialized;

void minhook_init() {
    if (MH_Initialize() != MH_OK) {
    } else {
        MinHook_Initialized = true;
#define HOOK_PROC(m, n) do { \
    LPVOID pTarget; \
    if (MH_CreateHookApiEx( \
        L ## #m, #n, \
        (LPVOID)&n, \
        (LPVOID *)&p ## n, \
        &pTarget \
    ) != MH_OK) { \
    } else { \
        if (MH_EnableHook(pTarget) != MH_OK) { \
        } \
    } \
} while (0)
        HOOK_PROC(d3d10, D3D10CreateDeviceAndSwapChain);
    }
}

void minhook_shutdown() {
    if (MinHook_Initialized) {
        if (MH_Uninitialize() != MH_OK) {
        }
        MinHook_Initialized = false;
    }
}

}

void base_dll_init(HINSTANCE hinstDLL) {
    UINT len = GetSystemDirectory(NULL, 0);
    len += _tcslen(BASE_DLL_NAME);
    TCHAR BASE_DLL_NAME_FULL[len] = {};
    GetSystemDirectory(BASE_DLL_NAME_FULL, len);
    _tcscat(BASE_DLL_NAME_FULL, BASE_DLL_NAME);

    base_dll = LoadLibrary(BASE_DLL_NAME_FULL);
    if (!base_dll) {
    } else if (base_dll == hinstDLL) {
        FreeLibrary(base_dll);
        base_dll = NULL;
    } else {
#define LOAD_PROC(n) do { \
    p ## n = (n ## _t)GetProcAddress(base_dll, #n); \
} while (0)
        LOAD_PROC(DirectInput8Create);
        LOAD_PROC(DllCanUnloadNow);
        LOAD_PROC(DllGetClassObject);
        LOAD_PROC(DllRegisterServer);
        LOAD_PROC(DllUnregisterServer);
        LOAD_PROC(GetdfDIJoystick);

        minhook_init();
    }
}

void base_dll_shutdown() {
    if (base_dll) {
        minhook_shutdown();

        pDirectInput8Create = NULL;
        pDllCanUnloadNow = NULL;
        pDllGetClassObject = NULL;
        pDllRegisterServer = NULL;
        pDllUnregisterServer = NULL;
        pGetdfDIJoystick = NULL;
        FreeLibrary(base_dll);
        base_dll = NULL;
    }
}
