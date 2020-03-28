#ifndef DIRECTINPUT8A_H
#define DIRECTINPUT8A_H

#include "main.h"
#include "unknown.h"

class MyIDirectInput8A : public IDirectInput8A {
    class Impl;
    Impl *impl;

public:
    MyIDirectInput8A(
        IDirectInput8A **inner
    );

    virtual ~MyIDirectInput8A();

    IUNKNOWN_DECL(IDirectInput8A)

    // IDirectInput8A

    virtual HRESULT STDMETHODCALLTYPE CreateDevice(
        REFGUID rguid,
        LPDIRECTINPUTDEVICE8A *lplpDirectInputDevice,
        LPUNKNOWN pUnkOuter
    );

    virtual HRESULT STDMETHODCALLTYPE EnumDevices(
        DWORD dwDevType,
        LPDIENUMDEVICESCALLBACKA lpCallback,
        LPVOID pvRef,
        DWORD dwFlags
    );

    virtual HRESULT STDMETHODCALLTYPE GetDeviceStatus(
        REFGUID rguidInstance
    );

    virtual HRESULT STDMETHODCALLTYPE RunControlPanel(
        HWND hwndOwner,
        DWORD dwFlags
    );

    virtual HRESULT STDMETHODCALLTYPE Initialize(
        HINSTANCE hinst,
        DWORD dwVersion
    );

    virtual HRESULT STDMETHODCALLTYPE FindDevice(
        REFGUID rguid,
        LPCSTR pszName,
        LPGUID pguidInstance
    );

    virtual HRESULT STDMETHODCALLTYPE EnumDevicesBySemantics(
        LPCSTR ptszUserName,
        LPDIACTIONFORMATA lpdiActionFormat,
        LPDIENUMDEVICESBYSEMANTICSCBA lpCallback,
        LPVOID pvRef,
        DWORD dwFlags
    );

    virtual HRESULT STDMETHODCALLTYPE ConfigureDevices(
        LPDICONFIGUREDEVICESCALLBACK lpdiCallback,
        LPDICONFIGUREDEVICESPARAMSA lpdiCDParams,
        DWORD dwFlags,
        LPVOID pvRefData
    );
};

#endif
