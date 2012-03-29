#ifndef BIFURCATE_CONTROLLER_H
#define BIFURCATE_CONTROLLER_H

#include "Config.h"

namespace bc
{
    struct Controller
    {
        float mLeftX;
        float mLeftY;
        float mRightX;
        float mRightY;
        uint8_t mButtons[16];
    };

    enum ControllerId
    {
        CONTROLLER_KEYBOARD_MOUSE,
        CONTROLLER_GAMEPAD_1,
        CONTROLLER_NUM
    };

    bool ControllerInitialize();
    void ControllerShutdown();
    void ControllerUpdate();
    const Controller *ControllerGetState(ControllerId controllerId);
}

#endif