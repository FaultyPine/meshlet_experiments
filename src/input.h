#pragma once

#include "common.h"

constexpr u32 const_ceil(float num)
{
    return (static_cast<float>(static_cast<u32>(num)) == num)
        ? static_cast<u32>(num)
        : static_cast<u32>(num) + ((num > 0) ? 1 : 0);
}

constexpr u32 SG_NUM_KEYCODES = 349;// SAPP_KEYCODE_MENU+1;
struct InputState
{
    u32 pressedKeys[const_ceil(((double)SG_NUM_KEYCODES) / 32.0)] = {0}; // 1 bit for every keycode
};

struct sapp_event;
void InputSystemOnEvent(const sapp_event* event);
bool IsKeyDown(int sapp_keycode);

