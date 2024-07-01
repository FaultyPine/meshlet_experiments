#include "input.h"

#include "common.h"
#include "external/sokol/sokol_app.h"

static InputState inputState;

bool IsKeyDown(int sapp_keycode)
{
    int idx = sapp_keycode / 32;
    int bit_idx = sapp_keycode - idx;
    SOKOL_ASSERT(idx < SG_NUM_KEYCODES);
    return inputState.pressedKeys[idx] & (1 << bit_idx);
}


void InputSystemOnEvent(const sapp_event* event)
{
    if (event->type == SAPP_EVENTTYPE_KEY_DOWN || event->type == SAPP_EVENTTYPE_CHAR)
    {
        int pressed_key = event->key_code;
        if (event->type == SAPP_EVENTTYPE_CHAR)
        {
            pressed_key = event->char_code;
        }
        int idx = pressed_key / 32;
        SOKOL_ASSERT(idx < SG_NUM_KEYCODES);
        int bit_idx = pressed_key - idx;
        inputState.pressedKeys[idx] |= (1 << bit_idx);
    }
    else if (event->type == SAPP_EVENTTYPE_KEY_UP)
    {
        int pressed_key = event->key_code;
        int idx = pressed_key / 32;
        SOKOL_ASSERT(idx < SG_NUM_KEYCODES);
        int bit_idx = pressed_key - idx;
        inputState.pressedKeys[idx] &= (0 << bit_idx);
    }
}