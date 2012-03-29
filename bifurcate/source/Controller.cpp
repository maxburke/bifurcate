#include "Controller.h"
#include "Gfx.h"

#pragma warning(push, 0)
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <Xinput.h>
#pragma warning(pop)

namespace bc
{
    static LPDIRECTINPUT8 gDirectInput;
    static LPDIRECTINPUTDEVICE8 gKeyboard;
    static LPDIRECTINPUTDEVICE8 gMouse;

    static BYTE gKeyState[256];
    static DIMOUSESTATE2 gMouseState;

    Controller gControllers[CONTROLLER_NUM];

    bool ControllerInitialize()
    {
        if (FAILED(
            DirectInput8Create(GetModuleHandle(NULL),
            DIRECTINPUT_VERSION,
            IID_IDirectInput8,
            (LPVOID *)&gDirectInput,
            NULL)))
            return false;

        if (FAILED(gDirectInput->CreateDevice(GUID_SysKeyboard, &gKeyboard, NULL)))
            return false;

        if (FAILED(gKeyboard->SetDataFormat(&c_dfDIKeyboard)))
            return false;

        if (FAILED(gKeyboard->SetCooperativeLevel((HWND)bg::GfxGetWindowHandle(), DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)))
            return false;

        if (FAILED(gKeyboard->Acquire()))
            return false;

        if (FAILED(gDirectInput->CreateDevice(GUID_SysMouse, &gMouse, NULL)))
            return false;

        if (FAILED(gMouse->SetDataFormat(&c_dfDIMouse2)))
            return false;

        if (FAILED(gMouse->SetCooperativeLevel((HWND)bg::GfxGetWindowHandle(), DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)))
            return false;

        if (FAILED(gMouse->Acquire()))
            return false;
            
        return true;
    }

    void ControllerUpdate()
    {
        Controller *kbMouse = &gControllers[CONTROLLER_KEYBOARD_MOUSE];

        HRESULT kb = gKeyboard->GetDeviceState(256, gKeyState);
        if (FAILED(kb))
            gKeyboard->Acquire();
//        assert(SUCCEEDED(kb));
        UNUSED(kb);

        HRESULT m = gMouse->GetDeviceState(sizeof(DIMOUSESTATE2), &gMouseState);
        if (FAILED(m))
            gMouse->Acquire();
//        assert(SUCCEEDED(m));
        UNUSED(m);

        float leftY = 0;
        leftY += gKeyState[DIK_W] == 0 ? 0 : 1.0f;
        leftY -= gKeyState[DIK_S] == 0 ? 0 : 1.0f;
        kbMouse->mLeftY = leftY; 

        float leftX = 0;
        leftX += gKeyState[DIK_D] == 0 ? 0 : 1.0f;
        leftX -= gKeyState[DIK_A] == 0 ? 0 : 1.0f;
        kbMouse->mLeftX = leftX;

        const float invMaxDisplacement = 1.0f / 55.0f;
        kbMouse->mRightX = static_cast<float>(gMouseState.lX) * invMaxDisplacement;
        kbMouse->mRightY = static_cast<float>(gMouseState.lY) * invMaxDisplacement;
    }

    void ControllerShutdown()
    {
        if (gKeyboard)
            gKeyboard->Release();

        if (gMouse)
            gMouse->Release();

        if (gDirectInput)
            gDirectInput->Release();
    }

    const Controller *ControllerGetState(ControllerId controllerId)
    {
        return &gControllers[controllerId];
    }
}