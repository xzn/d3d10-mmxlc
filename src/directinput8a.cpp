#include "directinput8a.h"
#include "unknown_impl.h"
#include "log.h"

#define LOGGER default_logger
#define LOG_MFUN(_, ...) LOG_MFUN_DEF(MyIDirectInput8A, ## __VA_ARGS__)

class MyIDirectInput8A::Impl {
    friend class MyIDirectInput8A;

    IUNKNOWN_PRIV(IDirectInput8A)

    Impl(
        IDirectInput8A **inner
    ) :
        IUNKNOWN_INIT(*inner)
    {}

    ~Impl() {}
};

IUNKNOWN_IMPL(MyIDirectInput8A, IDirectInput8A)

MyIDirectInput8A::MyIDirectInput8A(
    IDirectInput8A **inner
) :
    impl(new Impl(inner))
{
    LOG_MFUN(_,
        LOG_ARG(*inner)
    );
    *inner = this;
}

MyIDirectInput8A::~MyIDirectInput8A() {
    LOG_MFUN();
    delete impl;
}

// IDirectInput8A

HRESULT STDMETHODCALLTYPE MyIDirectInput8A::CreateDevice(
    REFGUID rguid,
    LPDIRECTINPUTDEVICE8A *lplpDirectInputDevice,
    LPUNKNOWN pUnkOuter
) {
    LOG_MFUN();
    return impl->inner->CreateDevice(
        rguid,
        lplpDirectInputDevice,
        pUnkOuter
    );
}

HRESULT STDMETHODCALLTYPE MyIDirectInput8A::EnumDevices(
    DWORD dwDevType,
    LPDIENUMDEVICESCALLBACKA lpCallback,
    LPVOID pvRef,
    DWORD dwFlags
) {
    HRESULT ret = impl->inner->EnumDevices(
        dwDevType,
        lpCallback,
        pvRef,
        dwFlags
    );
    LOG_MFUN(_,
        LOG_ARG_TYPE(dwDevType, DI8DEVCLASS_Logger),
        LOG_ARG_TYPE(dwFlags, DIEDFL_Logger),
        ret
    );
    return ret;
}

HRESULT STDMETHODCALLTYPE MyIDirectInput8A::GetDeviceStatus(
    REFGUID rguidInstance
) {
    LOG_MFUN();
    return impl->inner->GetDeviceStatus(
        rguidInstance
    );
}

HRESULT STDMETHODCALLTYPE MyIDirectInput8A::RunControlPanel(
    HWND hwndOwner,
    DWORD dwFlags
) {
    LOG_MFUN();
    return impl->inner->RunControlPanel(
        hwndOwner,
        dwFlags
    );
}

HRESULT STDMETHODCALLTYPE MyIDirectInput8A::Initialize(
    HINSTANCE hinst,
    DWORD dwVersion
) {
    LOG_MFUN();
    return impl->inner->Initialize(
        hinst,
        dwVersion
    );
}

HRESULT STDMETHODCALLTYPE MyIDirectInput8A::FindDevice(
    REFGUID rguid,
    LPCSTR pszName,
    LPGUID pguidInstance
) {
    LOG_MFUN();
    return impl->inner->FindDevice(
        rguid,
        pszName,
        pguidInstance
    );
}

HRESULT STDMETHODCALLTYPE MyIDirectInput8A::EnumDevicesBySemantics(
    LPCSTR ptszUserName,
    LPDIACTIONFORMATA lpdiActionFormat,
    LPDIENUMDEVICESBYSEMANTICSCBA lpCallback,
    LPVOID pvRef,
    DWORD dwFlags
) {
    LOG_MFUN();
    return impl->inner->EnumDevicesBySemantics(
        ptszUserName,
        lpdiActionFormat,
        lpCallback,
        pvRef,
        dwFlags
    );
}

HRESULT STDMETHODCALLTYPE MyIDirectInput8A::ConfigureDevices(
    LPDICONFIGUREDEVICESCALLBACK lpdiCallback,
    LPDICONFIGUREDEVICESPARAMSA lpdiCDParams,
    DWORD dwFlags,
    LPVOID pvRefData
) {
    LOG_MFUN();
    return impl->inner->ConfigureDevices(
        lpdiCallback,
        lpdiCDParams,
        dwFlags,
        pvRefData
    );
}
